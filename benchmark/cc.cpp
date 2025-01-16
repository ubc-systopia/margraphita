#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
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
 * Connected Components for undirected graph
 */

const int THREAD_NUM = omp_get_max_threads();

std::vector<node_id_t> connected_components(GraphBase* graph, bool is_directed)
{
  node_id_t max_id = graph->get_max_node_id();
  node_id_t numNodes = graph->get_num_nodes();
  std::vector<node_id_t> labels(max_id, -1);
  int comp_num = 0;
  NodeCursor* node_cursor = graph->get_node_iter();
  node_cursor->set_key_range({0, numNodes});
  node found;
  node_cursor->next(&found);
  while (found.id != OutOfBand_ID_MAX)
  {
    if (labels[found.id] == -1)
    {
      comp_num += 1;
      std::queue<node_id_t> frontier;
      frontier.push(found.id);
      labels[found.id] = found.id;

      while (!frontier.empty())
      {
        node_id_t u = frontier.front();
        frontier.pop();
        for (node_id_t k : graph->get_out_nodes_id(u))
        {
          if (labels[k] == -1)
          {
            labels[k] = found.id;
            frontier.push(k);
          }
        }
        if (is_directed)
        {
          for (node_id_t k : graph->get_in_nodes_id(u))
          {
            if (labels[k] == -1)
            {
              labels[k] = found.id;
              frontier.push(k);
            }
          }
        }
      }
    }
    node_cursor->next(&found);
  }
  node_cursor->close();
  delete node_cursor;

  return labels;
}

int main(int argc, char* argv[])
{
  std::cout << "Running CC" << std::endl;
  CmdLineApp cc_cli(argc, argv);
  if (!cc_cli.parse_args())
  {
    return -1;
  }

  cmdline_opts opts = cc_cli.get_parsed_opts();
  opts.stat_log += "/" + opts.db_name;

  Times t;
  t.start();
  GraphEngine graphEngine(THREAD_NUM, opts);
  GraphBase* graph = graphEngine.create_graph_handle();
  t.stop();
  std::cout << "Graph loaded in " << t.t_micros() << std::endl;

  for (int i = 0; i < opts.num_trials; i++)
  {
    cc_info info(0);
    t.start();
    std::vector<node_id_t> labels =
        connected_components(graph, opts.is_directed);
    t.stop();

    for (uint64_t i = 0; i < labels.size() - 1; i++)
    {
      if (labels[i] == -1) continue;
      if (labels[i] != labels[i + 1])
      {
        info.component_count++;
      }
    }

    info.time_taken = t.t_micros();
    std::cout << "Connected components completed in : " << info.time_taken
              << std::endl;
    std::cout << "Connencted components count = " << info.component_count
              << std::endl;
    print_csv_info(opts.db_name, info, opts.stat_log);
  }

  return 0;
}