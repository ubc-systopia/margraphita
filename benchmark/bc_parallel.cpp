#include <chrono>
#include <iostream>
#include <vector>

#include "benchmark_definitions.h"
#include "bitmap.h"
#include "command_line.h"
#include "csv_log.h"
#include "graph_engine.h"
#include "mem_usage.h"
#include "omp.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "times.h"

/*
Betweenness Centrality (BC) - Parallel Implementation

This implementation is based on the GAPBS benchmark suite BC kernel, adapted
to work with the Margraphita graph database interface (GraphEngine/GraphBase).

Algorithm:
Returns approximate betweenness centrality scores for each vertex using the
Brandes algorithm [1] with parallel optimizations from Madduri et al. [2].

The algorithm has two main phases:
1. PBFS (Parallel BFS): Computes shortest path counts from a source vertex
2. Backpropagation: Accumulates dependency scores by traversing BFS tree
   in reverse level order

The implementation is approximate because it samples from a subset of source
vertices rather than computing paths from all vertices. Scores are normalized
to the range [0,1] by dividing by the maximum score.

Key Implementation Differences from GAPBS:
- Adapted to use GraphEngine/GraphBase API instead of CSR graph format
- Graph traversal uses get_out_nodes_id() instead of direct neighbor arrays
- Each thread maintains its own read-only graph handle for parallel access
- Successor checking uses depth comparison (depths[v] == depths[u] + 1) rather
  than edge-indexed bitmap, as the graph interface doesn't expose edge indices
- The depths array is now passed to backpropagation to enable proper successor
  identification in the BFS tree

References:
[1] Ulrik Brandes. "A faster algorithm for betweenness centrality." Journal of
    Mathematical Sociology, 25(2):163–177, 2001.

[2] Kamesh Madduri, David Ediger, Karl Jiang, David A Bader, and Daniel
    Chavarria-Miranda. "A faster parallel algorithm and efficient multithreaded
    implementations for evaluating betweenness centrality on massive datasets."
    International Symposium on Parallel & Distributed Processing (IPDPS), 2009.
*/

typedef float ScoreT;
typedef double CountT;

const int THREAD_NUM = omp_get_max_threads();
std::vector<GraphBase *> graph_handles;

void PBFS(GraphEngine &graph_engine,
          node_id_t source,
          node_id_t maxNodeID,
          node_id_t minNodeID,
          pvector<CountT> &path_counts,
          Bitmap &succ,
          vector<SlidingQueue<node_id_t>::iterator> &depth_index,
          SlidingQueue<node_id_t> &queue,
          pvector<node_id_t> &depths)
{
  depths[source] = 0;
  path_counts[source] = 1;
  queue.push_back(source);
  depth_index.push_back(queue.begin());
  queue.slide_window();
#pragma omp parallel
  {
    node_id_t depth = 0;
    QueueBuffer<node_id_t> lqueue(queue);
    while (!queue.empty())
    {
      depth++;

#pragma omp for schedule(dynamic, 64) nowait
      for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++)
      {
        GraphBase *g_ = graph_handles[omp_get_thread_num()];
        node_id_t u = *q_iter;
        for (node_id_t &v : g_->get_out_nodes_id(u))
        {
          if ((depths[v] == OutOfBand_ID_MAX) &&
              (compare_and_swap(
                  depths[v], static_cast<node_id_t>(OutOfBand_ID_MAX), depth)))
          {
            lqueue.push_back(v);
          }
          if (depths[v] == depth)
          {
            succ.set_bit_atomic(v - minNodeID);
#pragma omp atomic
            path_counts[v] += path_counts[u];
          }
        }
        // g_->close(false);
      }
      lqueue.flush();
#pragma omp barrier
#pragma omp single
      {
        depth_index.push_back(queue.begin());
        queue.slide_window();
      }
    }
  }
  depth_index.push_back(queue.begin());
}

void PrintStep(const std::string &s, double seconds, int64_t count = -1)
{
  if (count != -1)
    printf("%5s%11" PRId64 "  %10.5lf\n", s.c_str(), count, seconds);
  else
    printf("%5s%23.5lf\n", s.c_str(), seconds);
}

pvector<ScoreT> Brandes(GraphEngine &graph_engine,
                        node_id_t start_vertex,
                        node_id_t maxNodeID,
                        node_id_t minNodeID,
                        node_id_t num_nodes,
                        edge_id_t num_edges,
                        int num_iters)
{
  Times t;
  t.start();
  pvector<ScoreT> scores(maxNodeID, 0);
  pvector<CountT> path_counts(maxNodeID);
  Bitmap succ(maxNodeID - minNodeID);
  vector<SlidingQueue<node_id_t>::iterator> depth_index;
  SlidingQueue<node_id_t> queue(maxNodeID);
  t.stop();
  PrintStep("a", t.t_secs());

  pvector<node_id_t> depths(maxNodeID, OutOfBand_ID_MAX);

  for (int iter = 0; iter < num_iters; iter++)
  {
    t.start();
    path_counts.fill(0);
    depth_index.resize(0);
    queue.reset();
    succ.reset();
    depths.fill(OutOfBand_ID_MAX);
    PBFS(graph_engine,
         start_vertex,
         maxNodeID,
         minNodeID,
         path_counts,
         succ,
         depth_index,
         queue,
         depths);
    t.stop();
    PrintStep("b", t.t_secs());

#ifdef DEBUG
    // Debug: Check BFS results
    std::cout << "DEBUG: BFS reached " << depth_index.size() - 1 << " levels"
              << std::endl;
    node_id_t nodes_reached = 0;
    CountT total_paths = 0;
    for (node_id_t i = 0; i < maxNodeID; i++)
    {
      if (path_counts[i] > 0)
      {
        nodes_reached++;
        total_paths += path_counts[i];
      }
    }
    std::cout << "DEBUG: BFS reached " << nodes_reached
              << " nodes, total paths: " << total_paths << std::endl;
#endif

    pvector<ScoreT> deltas(maxNodeID, 0);
    t.start();
    for (int d = depth_index.size() - 2; d >= 0; d--)
    {
#pragma omp parallel for schedule(dynamic, 64)
      for (auto it = depth_index[d]; it < depth_index[d + 1]; it++)
      {
        GraphBase *g_ =
            graph_handles[omp_get_thread_num()];  //.create_graph_handle();
        node_id_t u = *it;
        ScoreT delta_u = 0;
        for (node_id_t v : g_->get_out_nodes_id(u))
        {
          // Check if v is a successor of u in BFS tree (v is one level deeper)
          if (depths[v] == depths[u] + 1)
          {
            delta_u += (path_counts[u] / path_counts[v]) * (1 + deltas[v]);
          }
        }
        deltas[u] = delta_u;
        scores[u] += delta_u;
        // g_->close(false);
      }
    }
    t.stop();
    PrintStep("p", t.t_secs());

#ifdef DEBUG
    // Debug: Check delta computation
    ScoreT total_delta = 0;
    for (node_id_t i = 0; i < maxNodeID; i++)
    {
      total_delta += deltas[i];
    }
    std::cout << "DEBUG: Total delta after backprop: " << total_delta
              << std::endl;
#endif
  }

#ifdef DEBUG
  // Debug: Check scores before normalization
  ScoreT total_score = 0;
  node_id_t nonzero_scores = 0;
  for (node_id_t i = 0; i < maxNodeID; i++)
  {
    if (scores[i] != 0)
    {
      nonzero_scores++;
      total_score += scores[i];
    }
  }
  std::cout << "DEBUG: Before normalization - " << nonzero_scores
            << " non-zero scores, total: " << total_score << std::endl;
#endif

  // normalize scores
  ScoreT biggest_score = 0;
#pragma omp parallel for reduction(max : biggest_score)
  for (node_id_t n = 0; n < maxNodeID; n++)
    biggest_score = max(biggest_score, scores[n]);

#ifdef DEBUG
  std::cout << "DEBUG: Biggest score before normalization: " << biggest_score
            << std::endl;
#endif

  if (biggest_score > 0)
  {
#pragma omp parallel for
    for (node_id_t n = 0; n < maxNodeID; n++)
      scores[n] = scores[n] / biggest_score;
  }
  else
  {
#ifdef DEBUG
    std::cout
        << "WARNING: biggest_score is 0 or negative, skipping normalization"
        << std::endl;
#endif
  }

  return scores;
}

// Returns k pairs with largest values from list of key-value pairs
template <typename KeyT, typename ValT>
std::vector<std::pair<ValT, KeyT>> TopK(
    const std::vector<std::pair<KeyT, ValT>> &to_sort, size_t k)
{
  std::vector<std::pair<ValT, KeyT>> top_k;
  ValT min_so_far = 0;
  for (auto kvp : to_sort)
  {
    if ((top_k.size() < k) || (kvp.second > min_so_far))
    {
      top_k.push_back(std::make_pair(kvp.second, kvp.first));
      std::sort(
          top_k.begin(), top_k.end(), std::greater<std::pair<ValT, KeyT>>());
      if (top_k.size() > k) top_k.resize(k);
      min_so_far = top_k.back().first;
    }
  }
  return top_k;
}

void print_top_scores(GraphBase *g,
                      node_id_t maxNodeID,
                      const pvector<ScoreT> &scores)
{
  vector<pair<node_id_t, ScoreT>> score_pairs(maxNodeID);
  NodeCursor *node_cursor = g->get_node_iter();

  // Populate score_pairs with (node_id, score) pairs
  node found = {0};
  node_cursor->next(&found);
  while (found.id != OutOfBand_ID_MAX)
  {
    score_pairs[found.id] = make_pair(found.id, scores[found.id]);
    node_cursor->next(&found);
  }

  node_id_t k = 5;
  vector<pair<ScoreT, node_id_t>> top_k = TopK(score_pairs, k);
  node_id_t it = 0;
  std::cout << "Top " << k << " nodes by BC score:" << std::endl;
  for (auto kvp : top_k)
  {
    it++;
    std::cout << kvp.second << ":" << kvp.first << std::endl;
  }
}

void create_graph_handles(GraphEngine &graph_engine,
                          std::string &checkpoint_name,
                          int thread_num)
{
  graph_handles.resize(thread_num);
#pragma omp parallel for num_threads(thread_num)
  for (int i = 0; i < thread_num; i++)
  {
    GraphBase *graph = graph_engine.create_ro_graph_handle(checkpoint_name);
    graph_handles[i] = graph;
  }
}

int main(int argc, char *argv[])
{
  std::cout << "Running Betweenness Centrality" << std::endl;
  mem_util::MemoryCounter memory_usage;

  CmdLineApp cli(argc, argv);
  if (!cli.parse_args())
  {
    return -1;
  }

  cmdline_opts opts = cli.get_parsed_opts();
  opts.stat_log += "/" + opts.db_name;
  opts.create_new = false;  // we will work on a checkpoint
  opts.read_only = true;

  graph_handles.reserve(THREAD_NUM);  // preallocate graph handles vector.
  Times t;
  t.start();
  GraphEngine graph_engine(THREAD_NUM, opts);
  std::string chkpt = graph_engine.make_checkpoint();
  graph_engine.set_partition_strategy(PartitionStrategy::NODE_COUNT);
  graph_engine.calculate_thread_offsets();

  t.stop();
  std::cout << "Graph loaded in " << t.t_secs() << std::endl;

  create_graph_handles(graph_engine, chkpt, THREAD_NUM);
  GraphBase *graph_stat = graph_handles[0];

  if (opts.start_vertex == OutOfBand_ID_MAX)
  {
    throw std::runtime_error(
        "BC requires a start vertex. Provide one using -v option.");
  }

  node_id_t maxNodeID = graph_stat->get_max_node_id();
  node_id_t minNodeID = graph_stat->get_min_node_id();
  node_id_t num_nodes = graph_stat->get_num_nodes();
  node_id_t num_edges = graph_stat->get_num_edges();

  long double total_time = 0;
  sssp_info info(0);
  for (int i = 0; i < opts.num_trials; i++)
  {
    t.start();
    pvector<ScoreT> scores = Brandes(graph_engine,
                                     opts.start_vertex,
                                     maxNodeID,
                                     minNodeID,
                                     num_nodes,
                                     num_edges,
                                     opts.iterations);

    t.stop();

    info.time_taken = t.t_secs();
    total_time += info.time_taken;
    std::cout << "Trial time: " << t.t_secs() << std::endl;
    print_top_scores(graph_stat, maxNodeID, scores);
    print_csv_info(opts.db_name, info, opts.stat_log);
  }
  // Clean up graph handles
  for (int i = 0; i < THREAD_NUM; i++)
  {
    graph_handles[i]->close(false);
  }
  std::cout << "Average time taken for " << opts.num_trials
            << " trials: " << total_time / opts.num_trials << std::endl;

  graph_engine.close_graph();
  return 0;
}