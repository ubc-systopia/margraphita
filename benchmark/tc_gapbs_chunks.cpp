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
Modified from original GAP Benchmark Suite with chunk-based dynamic scheduling:

This version uses dynamic work distribution where the graph is divided into
many small chunks instead of fixed per-thread partitions. OpenMP dynamically
assigns chunks to threads, providing better load balancing when triangle
density varies across the graph.

--------------------------------------------------------------------------
GAP Benchmark Suite
Kernel: Triangle Counting (TC)
Author: Scott Beamer

Will count the number of triangles (cliques of size 3)

Input graph requirements:
  - undirected
  - has no duplicate edges (or else will be counted as multiple triangles)
  - neighborhoods are sorted by vertex identifiers

Other than symmetrizing, the rest of the requirements are done by SquishCSR
during graph building.

This implementation reduces the search space by counting each triangle only
once. A naive implementation will count the same triangle six times because
each of the three vertices (u, v, w) will count it in both ways. To count
a triangle only once, this implementation only counts a triangle if u > v > w.
Once the remaining unexamined neighbors identifiers get too big, it can break
out of the loop, but this requires that the neighbors are sorted.

This implementation relabels the vertices by degree. This optimization is
beneficial if the average degree is sufficiently high and if the degree
distribution is sufficiently non-uniform. To decide whether to relabel the
graph, we use the heuristic in WorthRelabelling.
*/

using namespace std;
vector<GraphBase *> graph_handles;
const int THREAD_NUM = omp_get_max_threads();

size_t OrderedCount(GraphEngine &graph_engine)
{
  size_t total = 0;

  // Get work chunks from GraphEngine (many small chunks for dynamic scheduling)
  const int CHUNK_SIZE = 500000;
  std::vector<key_range> chunks = graph_engine.get_work_chunks(CHUNK_SIZE);

  LOG_MSG("Created {} work chunks with ~{} nodes each for dynamic scheduling",
          chunks.size(),
          CHUNK_SIZE);

  // Log chunk details
  for (size_t i = 0; i < chunks.size(); i++)
  {
    LOG_MSG("Chunk {}: start={}, end={}, node_range={}",
            i,
            chunks[i].start,
            chunks[i].end,
            chunks[i].end - chunks[i].start + 1);
  }

  atomic<size_t> total_nodes_processed = 0;

  // Use dynamic scheduling to distribute chunks across threads
  #pragma omp parallel for reduction(+ : total) schedule(dynamic, 1)
  for (size_t chunk_idx = 0; chunk_idx < chunks.size(); chunk_idx++)
  {
    Times chunk_timer;
    chunk_timer.start();

    int tid = omp_get_thread_num();
    GraphBase *graph = graph_handles[tid];
    OutCursor *out_cursor = graph->get_outnbd_iter();
    out_cursor->set_key_range(chunks[chunk_idx]);

    // Create a separate cursor for neighbor lookups
    OutCursor *lookup_cursor = graph->get_outnbd_iter();
    adjlist v_adjlist;  // Reusable buffer for v's neighbors

    size_t local_triangles = 0;
    size_t local_nodes_processed = 0;
    size_t local_edges_processed = 0;

    adjlist found;
    out_cursor->next(&found);
    while (found.node_id != OutOfBand_ID_MAX)
    {
      local_nodes_processed++;
      total_nodes_processed++;
      local_edges_processed += found.edgelist.size();

      for (node_id_t v : found.edgelist)
      {
        if (v > found.node_id) break;

        // Use cursor to fetch v's adjacency list directly
        lookup_cursor->set_key_range({v, v});  // Seek to node v
        v_adjlist.clear();
        lookup_cursor->next(&v_adjlist);

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
    LOG_MSG("Thread {} completed chunk {}/{} [{}, {}]: {} nodes, {} edges, {} triangles in {:.3f}s (total progress: {}/? nodes)",
            tid,
            chunk_idx + 1,
            chunks.size(),
            chunks[chunk_idx].start,
            chunks[chunk_idx].end,
            local_nodes_processed,
            local_edges_processed,
            local_triangles,
            chunk_timer.t_secs(),
            total_nodes_processed.load());

    lookup_cursor->close();
    delete lookup_cursor;
    out_cursor->close();
    delete out_cursor;

    LOG_MSG("Thread {} finishing chunk and continuing to next work item or joining", tid);
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
  std::cout << "Running TC (Chunk-based Dynamic Scheduling)" << std::endl;
  LOG_MSG("=== Triangle Counting (Chunks) Starting ===");
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
  std::string checkpt = graphEngine.make_checkpoint();
  graphEngine.calculate_thread_offsets();
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
    LOG_MSG("Beginning triangle counting with dynamic chunk scheduling...");
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
