#include <algorithm>
#include <cinttypes>
#include <deque>
#include <iostream>
#include <set>
#include <vector>

#include "benchmark_definitions.h"
#include "command_line.h"
#include "common_util.h"
#include "csv_log.h"
#include "graph_engine.h"
#include "mem_usage.h"
#include "omp.h"
#include "times.h"

/*
Modified from original GAP Benchmark Suite:

We never relabel the graph and always use the ordered counting method.
--------------------------------------------------------------------------
*/

using namespace std;
vector<GraphBase *> graph_handles;
const int THREAD_NUM = omp_get_max_threads();

size_t OrderedCount(GraphEngine &graph_engine)
{
  size_t total = 0;
  int total_work_chunks = graph_engine.get_total_partitions();
  atomic<size_t> total_nodes_processed = 0;
#pragma omp parallel for num_threads(THREAD_NUM) reduction(+ : total) \
    schedule(dynamic, 1000)
  for (int i = 0; i < total_work_chunks; i++)
  {
    Times chunk_timer;
    chunk_timer.start();

    GraphBase *graph = graph_handles[omp_get_thread_num()];
    OutCursor *out_cursor = graph->get_outnbd_iter();
    out_cursor->set_key_range(graph_engine.get_key_range(i));

    // Create a separate cursor for neighbor lookups
    // OutCursor *lookup_cursor = graph->get_outnbd_iter();
    WT_CURSOR *lookup_wt_cursor =
        dynamic_cast<AdjList *>(graph)->get_out_adjlist_cursor();

    adjlist v_adjlist;  // Reusable buffer for v's neighbors

    size_t local_nodes_processed = 0;
    size_t local_edges_processed = 0;
    size_t local_triangles = 0;

    LOG_MSG("Thread {} starting chunk {}/{} with key range [{}, {}]",
            i,
            i + 1,
            total_work_chunks,
            graph_engine.get_key_range(i).start,
            graph_engine.get_key_range(i).end);

    adjlist found;
    out_cursor->next(&found);
    while (found.node_id != OutOfBand_ID_MAX)
    {
      local_nodes_processed++;
      ++total_nodes_processed;
      local_edges_processed += found.edgelist.size();

      // Log progress every 1000000 nodes
      if (local_nodes_processed % 1'000'000 == 0)
      {
        LOG_MSG(
            "Thread {} processed {} nodes, {} edges, found {} triangles so far",
            i,
            local_nodes_processed,
            local_edges_processed,
            local_triangles);
      }

      for (node_id_t v : found.edgelist)
      {
        if (v > found.node_id) break;

        // Use cursor to fetch v's adjacency list directly
        // lookup_cursor->set_key_range({v, v});  // Seek to node v
        CommonUtil::set_key(lookup_wt_cursor, v);
        int ret = lookup_wt_cursor->search(lookup_wt_cursor);
        if (ret == WT_NOTFOUND)
        {
          LOG_MSG("Node ID {} does not have an adjacency list", v);
          continue;
        }
        else
        {
          CommonUtil::record_to_adjlist(lookup_wt_cursor, &v_adjlist);
        }
        lookup_wt_cursor->reset(lookup_wt_cursor);

        // Now v_adjlist.edgelist contains v's neighbors (already sorted)
        auto it = v_adjlist.edgelist.begin();
        for (node_id_t w : found.edgelist)
        {
          if (w > v) break;
          while (it != v_adjlist.edgelist.end() && *it < w) it++;
          if (it != v_adjlist.edgelist.end() && w == *it)
          {
            total++;
            local_triangles++;
          }
        }
      }
      out_cursor->next(&found);
    }
    chunk_timer.stop();

    // Log completion of this chunk with detailed metrics
    LOG_MSG(
        "Thread {} completed chunk {}/{} [{}, {}]: {} nodes, {} edges, {} "
        "triangles in {:.3f}s (total progress: {}/? nodes)",
        i,
        i + 1,
        total_work_chunks,
        graph_engine.get_key_range(i).start,
        graph_engine.get_key_range(i).end,
        local_nodes_processed,
        local_edges_processed,
        local_triangles,
        chunk_timer.t_secs(),
        total_nodes_processed.load());

    lookup_wt_cursor->close(lookup_wt_cursor);
    out_cursor->close();
    delete out_cursor;

    LOG_MSG("Thread {} (OMP thread {}) finishing and joining",
            i,
            omp_get_thread_num());
    // graph->close(false);
  }

  return total;
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

void cleanup_graph_handles()
{
  for (auto graph : graph_handles)
  {
    if (graph != nullptr)
    {
      graph->close(false);
    }
  }
}

int main(int argc, char *argv[])
{
  std::cout << "Running TC (Optimized with cursor-based lookups)" << std::endl;
  LOG_MSG("=== Triangle Counting (Optimized) Starting ===");
  CmdLineApp tc_cli(argc, argv);
  if (!tc_cli.parse_args())
  {
    return -1;
  }

  cmdline_opts opts = tc_cli.get_parsed_opts();
  opts.stat_log += "/" + opts.db_name;
  opts.create_new = false;  // we will work on a checkpoint
  opts.read_only = true;

  LOG_MSG("Configuration: {} threads, {} trials, database: {}",
          THREAD_NUM,
          opts.num_trials,
          opts.db_name);

  Times t;
  t.start();
  LOG_MSG("Loading graph from database...");
  GraphEngine graphEngine(THREAD_NUM, opts);
  graphEngine.set_partition_strategy(PartitionStrategy::EDGE_AWARE);
  graphEngine.set_partition_scale(
      1600);  // finer partitions for better balancing
  std::string checkpt = graphEngine.make_checkpoint();
  graphEngine.calculate_thread_offsets();

  std::cout << "Partition strategy: " << graphEngine.get_strategy_name()
            << std::endl;
  std::cout << "creating " << graphEngine.get_total_partitions()
            << " partitions for work chunks" << std::endl;
  t.stop();
  LOG_MSG("Graph loaded in {} seconds", t.t_secs());
  std::cout << "Graph loaded in " << t.t_secs() << std::endl;

  // Create graph handles
  t.start();
  LOG_MSG("Creating {} graph handles for parallel processing...", THREAD_NUM);
  create_graph_handles(graphEngine, checkpt, THREAD_NUM);
  t.stop();
  LOG_MSG("Graph handles created in {} seconds", t.t_secs());
  std::cout << "Graph handles created in " << t.t_secs() << std::endl;

  long double total_time_trust = 0;
  for (int i = 0; i < opts.num_trials; i++)
  {
    LOG_MSG(
        "========== Starting Trial {}/{} ==========", i + 1, opts.num_trials);
    tc_info info(0);
    // Count Trust Triangles
    t.start();
    LOG_MSG("Beginning triangle counting...");
    info.trust_count = OrderedCount(graphEngine);
    t.stop();

    total_time_trust += t.t_secs();
    LOG_MSG("Trial {}/{} completed in {} seconds",
            i + 1,
            opts.num_trials,
            t.t_secs());
    std::cout << "Trust Triangle_Counting_ITER completed in : " << t.t_secs()
              << std::endl;
    std::cout << "Trust Triangles count = " << info.trust_count << std::endl;
  }
  std::cout << "Average time Trust: " << total_time_trust / opts.num_trials
            << std::endl;
  LOG_MSG("=== Triangle Counting Completed ===");
  LOG_MSG("Average time over {} trials: {} seconds",
          opts.num_trials,
          total_time_trust / opts.num_trials);

  cleanup_graph_handles();
  graphEngine.close_graph();
}
