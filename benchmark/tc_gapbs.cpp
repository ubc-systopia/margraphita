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
#pragma omp parallel for reduction(+ : total) schedule(dynamic, 64)
  for (int i = 0; i < THREAD_NUM; i++)
  {
    GraphBase *graph = graph_handles[i];
    OutCursor *out_cursor = graph->get_outnbd_iter();
    out_cursor->set_key_range(graph_engine.get_key_range(i));
    adjlist found;
    out_cursor->next(&found);
    while (found.node_id != OutOfBand_ID_MAX)
    {
      for (node_id_t v : found.edgelist)
      {
        if (v > found.node_id) break;
        std::vector<node_id_t> v_nbd = graph->get_out_nodes_id(v);
        auto it = v_nbd.begin();
        for (node_id_t w : found.edgelist)
        {
          if (w > v) break;
          //                    while (it != v_nbd.end() && *it < w)
          //                    it++; if (w == *it) total++;
          while (it != v_nbd.end() && *it < w) it++;
          if (it != v_nbd.end() && w == *it) total++;
        }
      }
      out_cursor->next(&found);
    }
    out_cursor->close();
    delete out_cursor;
    graph->close(false);
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

int main(int argc, char *argv[])
{
  std::cout << "Running TC" << std::endl;
  CmdLineApp tc_cli(argc, argv);
  if (!tc_cli.parse_args())
  {
    return -1;
  }

  cmdline_opts opts = tc_cli.get_parsed_opts();
  opts.stat_log += "/" + opts.db_name;
  opts.create_new = false;  // we will work on a checkpoint
  opts.read_only = true;

  Times t;
  t.start();
  GraphEngine graphEngine(THREAD_NUM, opts);
  std::string checkpt = graphEngine.make_checkpoint();
  graphEngine.calculate_thread_offsets();
  t.stop();
  std::cout << "Graph loaded in " << t.t_secs() << std::endl;

  // Create graph handles
  t.start();
  create_graph_handles(graphEngine, checkpt, THREAD_NUM);
  t.stop();
  std::cout << "Graph handles created in " << t.t_secs() << std::endl;

  long double total_time_trust = 0;
  for (int i = 0; i < opts.num_trials; i++)
  {
    tc_info info(0);
    // Count Trust Triangles
    t.start();
    info.trust_count = OrderedCount(graphEngine);
    t.stop();

    total_time_trust += t.t_secs();
    std::cout << "Trust Triangle_Counting_ITER completed in : " << t.t_secs()
              << std::endl;
    std::cout << "Trust Triangles count = " << info.trust_count << std::endl;
  }
  std::cout << "Average time Trust: " << total_time_trust / opts.num_trials
            << std::endl;
  graphEngine.close_graph();
}