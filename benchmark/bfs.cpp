#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <vector>

#include "adj_list.h"
#include "benchmark_definitions.h"
#include "command_line.h"
#include "csv_log.h"
#include "graph_engine.h"
#include "times.h"

/**
 * This runs the BFS on the graph using src as the starting node.
 */

template <typename Graph>
bfs_info *bfs(Graph &graph, node_id_t src)
{
  Times timer;
  auto *info = new bfs_info(0);
  timer.start();
  for (int i = 0; i < 10; i++)
  {
    set<node_id_t> visited = {src};
    list<node_id_t> queue = {src};
    vector<node_id_t> result;

    while (!queue.empty())
    {
      node_id_t node_id = queue.front();
      queue.pop_front();
      result.push_back(node_id);
      vector<node> out_nbrs = graph->get_out_nodes(node_id);
      info->sum_out_deg += out_nbrs.size();
      for (node nbr : out_nbrs)
      {
        if (visited.find(nbr.id) == visited.end())
        {
          visited.insert(nbr.id);
          queue.push_back(nbr.id);
        }
      }
    }
    info->num_visited = visited.size();
  }
  timer.stop();
  info->time_taken = timer.t_micros();
  // average the time taken to get average per iteration.
  info->time_taken = info->time_taken / 10;
  return info;
}

template <typename Graph>
node_id_t find_random_start(Graph &graph)
{
  while (1)
  {
    node random_start = graph->get_random_node();
    std::cout << random_start.id << std ::endl;
    std::cout << "random start vertex is " << random_start.id << std::endl;
    if (graph->get_out_degree(random_start.id) != 0)
    {
      return random_start.id;
    }
    else
    {
      std::cout << "out degree was zero. finding a new random vertex."
                << std::endl;
    }
  }
}

int main(int argc, char *argv[])
{
  cout << "Running BFS" << endl;
  CmdLineApp bfs_cli(argc, argv);
  if (!bfs_cli.parse_args())
  {
    return -1;
  }

  cmdline_opts opts = bfs_cli.get_parsed_opts();
  opts.stat_log += "/" + opts.db_name;

  const int THREAD_NUM = 1;

  Times timer;
  timer.start();
  GraphEngine graphEngine(THREAD_NUM, opts);
  GraphBase *graph = graphEngine.create_graph_handle();
  timer.stop();
  std::cout << "Graph loaded in " << timer.t_micros() << std::endl;

  // do 10 runs with random starting nodes and run 10 trial per each
  // random node

#ifdef STAT
  opts.num_trials = 1;  // We want only one run with stats collection
#endif
  for (int i = 0; i < opts.num_trials; i++)
  {
    node_id_t start_vertex = opts.start_vertex;
    if (start_vertex == -1)
    {
      start_vertex = find_random_start(graph);
    }
    timer.start();
    bfs_info *bfs_run = bfs(graph, start_vertex);
    timer.stop();
    double time_from_outside = timer.t_micros();
    std::cout << "BFS  completed in : " << time_from_outside << std::endl;
    print_csv_info(
        opts.db_name, start_vertex, bfs_run, time_from_outside, opts.stat_log);
  }
  graph->close(false);
  graphEngine.close_graph();
}
