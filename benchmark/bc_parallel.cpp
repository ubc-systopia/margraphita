#include <chrono>
#include <iostream>
#include <vector>

#include "benchmark_definitions.h"
#include "bitmap.h"
#include "command_line.h"
#include "csv_log.h"
#include "graph_engine.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "times.h"

typedef float ScoreT;
typedef double CountT;

const int THREAD_NUM = omp_get_max_threads();

void PBFS(GraphEngine &graph_engine,
          node_id_t source,
          node_id_t maxNodeID,
          adjlist &nbd,
          pvector<CountT> &path_counts,
          Bitmap &succ,
          vector<SlidingQueue<node_id_t>::iterator> &depth_index,
          SlidingQueue<node_id_t> &queue)
{
    pvector<node_id_t> depths(maxNodeID, -1);
    depths[source] = 0;
    path_counts[source] = 1;
    queue.push_back(source);
    depth_index.push_back(queue.begin());
    queue.slide_window();
    node_id_t g_out_start = nbd.node_id;
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
                GraphBase *g_ = graph_engine.create_graph_handle();
                node_id_t u = *q_iter;
                for (node_id_t &v : g_->get_out_nodes_id(u))
                {
                    if ((depths[v] == -1) &&
                        (compare_and_swap(
                            depths[v], static_cast<node_id_t>(-1), depth)))
                    {
                        lqueue.push_back(v);
                    }
                    if (depths[v] == depth)
                    {
                        succ.set_bit_atomic(v - g_out_start);
#pragma omp atomic
                        path_counts[v] += path_counts[u];
                    }
                }
                g_->close();
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
                        std::vector<node_id_t> &random_nodes,
                        node_id_t maxNodeID,
                        node_id_t num_nodes,
                        edge_id_t num_edges,
                        int num_iters)
{
    Times t;
    t.start();
    GraphBase *g = graph_engine.create_graph_handle();
    pvector<ScoreT> scores(maxNodeID, 0);
    pvector<CountT> path_counts(maxNodeID);
    Bitmap succ(num_edges);
    vector<SlidingQueue<node_id_t>::iterator> depth_index;
    SlidingQueue<node_id_t> queue(maxNodeID);
    t.stop();
    PrintStep("a", t.t_secs());
    OutCursor *out_cur = g->get_outnbd_iter();
    out_cur->set_key_range({0, num_nodes});
    adjlist nbd;
    out_cur->next(&nbd);
    node_id_t g_out_start = nbd.node_id;

    for (int iter = 0; iter < num_iters; iter++)
    {
        node_id_t source = random_nodes.at(iter);
        t.start();
        path_counts.fill(0);
        depth_index.resize(0);
        queue.reset();
        succ.reset();
        PBFS(graph_engine,
             source,
             maxNodeID,
             nbd,
             path_counts,
             succ,
             depth_index,
             queue);
        t.stop();
        PrintStep("b", t.t_secs());
        pvector<ScoreT> deltas(maxNodeID, 0);
        t.start();
        for (int d = depth_index.size() - 2; d >= 0; d--)
        {
#pragma omp parallel for schedule(dynamic, 64)
            for (auto it = depth_index[d]; it < depth_index[d + 1]; it++)
            {
                GraphBase *g_ = graph_engine.create_graph_handle();
                node_id_t u = *it;
                ScoreT delta_u = 0;
                for (node_id_t v : g_->get_out_nodes_id(u))
                {
                    if (succ.get_bit(v - g_out_start))
                    {
                        delta_u +=
                            (path_counts[u] / path_counts[v]) * (1 + deltas[v]);
                    }
                }
                deltas[u] = delta_u;
                scores[u] += delta_u;
                g_->close();
            }
        }
        t.stop();
        PrintStep("p", t.t_secs());
    }

    // normalize scores
    ScoreT biggest_score = 0;
#pragma omp parallel for reduction(max : biggest_score)
    for (node_id_t n = 0; n < maxNodeID; n++)
        biggest_score = max(biggest_score, scores[n]);

#pragma omp parallel for
    for (node_id_t n = 0; n < maxNodeID; n++)
        scores[n] = scores[n] / biggest_score;

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
            std::sort(top_k.begin(),
                      top_k.end(),
                      std::greater<std::pair<ValT, KeyT>>());
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

    node found = {0};
    node_cursor->next(&found);
    while (found.id != UINT32_MAX)
    {
        score_pairs.emplace_back(found.id, scores[found.id]);
        node_cursor->next(&found);
    }
    node_id_t k = 100;
    vector<pair<ScoreT, node_id_t>> top_k = TopK(score_pairs, k);
    node_id_t it = 0;
    std::cout << "Top " << k << " nodes by PageRank:" << std::endl;
    for (auto kvp : top_k)
    {
        it++;
        if (kvp.first > 1e-4)
            std::cout << "(" << it << ") " << kvp.second << ":" << kvp.first
                      << std::endl;

        std::cout << kvp.second << ":" << kvp.first << std::endl;
    }
}

int main(int argc, char *argv[])
{
    std::cout << "Running SSSP" << std::endl;
    CmdLineApp cli(argc, argv);
    if (!cli.parse_args())
    {
        return -1;
    }

    cmdline_opts opts = cli.get_parsed_opts();
    opts.stat_log += "/" + opts.db_name;
    std::vector<node_id_t> random_nodes;
    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    graphEngine.calculate_thread_offsets();
    GraphBase *graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_secs() << std::endl;

    if (opts.start_vertex == -1)
    {
        graph->get_random_node_ids(random_nodes, opts.num_trials);
    }
    else
    {
        random_nodes.push_back(opts.start_vertex);
        opts.num_trials = 1;
    }

    node_id_t maxNodeID = graph->get_max_node_id();
    node_id_t num_edges = graph->get_num_edges();
    graph->close();

    long double total_time = 0;
    sssp_info info(0);
    for (int i = 0; i < opts.num_trials; i++)
    {
        t.start();
        pvector<ScoreT> scores = Brandes(graphEngine,
                                         random_nodes,
                                         maxNodeID,
                                         num_nodes,
                                         num_edges,
                                         opts.iterations);

        t.stop();

        info.time_taken = t.t_secs();
        total_time += info.time_taken;
        std::cout << "Single-Source Shortest Path completed in : "
                  << info.time_taken << std::endl;
        print_top_scores(graph, maxNodeID, scores);
        print_csv_info(opts.db_name, info, opts.stat_log);
    }
    std::cout << "Average time taken for " << opts.num_trials
              << " trials: " << total_time / opts.num_trials << std::endl;

    graphEngine.close_graph();
    return 0;
}