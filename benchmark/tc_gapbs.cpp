// Encourage use of gcc's parallel algorithms (for sort for relabeling)

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
#include "omp.h"
#include "times.h"
/**
 * This runs the Triangle Counting on the graph -- both Trust and Cycle counts
 */
#define _GLIBCXX_PARALLEL
using namespace std;

size_t OrderedCount(GraphEngine &graph_engine, int THREAD_NUM = 1)
{
  size_t total = 0;
#pragma omp parallel for reduction(+ : total) schedule(dynamic, 64)
  for (int i = 0; i < THREAD_NUM; i++)
  {
    GraphBase *graph = graph_engine.create_graph_handle();
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

  const int THREAD_NUM = omp_get_max_threads();
  std::cout << "THREAD_NUM: " << THREAD_NUM << std::endl;

  Times t;
  t.start();
  GraphEngine graphEngine(THREAD_NUM, opts);
  graphEngine.calculate_thread_offsets();
  t.stop();
  std::cout << "Graph loaded in " << t.t_micros() << std::endl;
  long double total_time_trust = 0;
  for (int i = 0; i < opts.num_trials; i++)
  {
    tc_info info(0);
    // Count Trust Triangles
    t.start();
    info.trust_count = OrderedCount(graphEngine, THREAD_NUM);
    t.stop();

    info.trust_time = t.t_secs();
    total_time_trust += info.trust_time;
    std::cout << "Trust Triangle_Counting_ITER completed in : "
              << info.trust_time << std::endl;
    std::cout << "Trust Triangles count = " << info.trust_count << std::endl;

    print_csv_info(opts.db_name, info, opts.stat_log);
  }
  std::cout << "Average time Trust: " << total_time_trust / opts.num_trials
            << std::endl;
  graphEngine.close_graph();
}