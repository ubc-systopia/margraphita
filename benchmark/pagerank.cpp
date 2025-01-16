#include <unistd.h>

#include <cassert>
#include <iostream>
#include <shared_mutex>
#include <vector>

#include "benchmark_definitions.h"
#include "command_line.h"
#include "csv_log.h"
#include "graph.h"
#include "graph_engine.h"
#include "times.h"

const float dampness = 0.85;
std::hash<int> hashfn;
int N = 1610612741;  // Hash bucket size
int p_cur = 0;
int p_next = 1;

pr_map *ptr = nullptr;  // pointer to mmap region
// float *pr_cur, *pr_next;

void init_pr_map(std::vector<node> &nodes)
{
  size_t size = nodes.size();
  make_pr_mmap(size, &ptr);
  // since the access pattern is random and because the node id's
  // non-continuous we cannot do any clever madvise tricks.
  assert(ptr != nullptr);
  float init_val = 1.0f / (float)size;

  for (node n : nodes)
  {
    if (ptr[n.id].id == 0)
    {
      ptr[n.id].id = n.id;
      ptr[n.id].p_rank[p_cur] = init_val;
      ptr[n.id].p_rank[p_next] = 0.0f;
    }
  }
}

void delete_map(int num_nodes) { munmap(ptr, sizeof(pr_map) * num_nodes); }

template <typename Graph>
void pagerank(Graph &graph,
              cmdline_opts &opts,
              int iterations,
              double tolerance,
              const std::string &csv_logdir)
{
  Times t;
  int num_nodes = graph.get_num_nodes();
  t.start();
  std::vector<node> nodes = graph.get_nodes();
  init_pr_map(nodes);
  std::vector<long double> times;
  t.stop();
  cout << "Loading the nodes and constructing the map took " << t.t_micros()
       << endl;
  times.push_back(t.t_micros());

  int iter_count = 0;
  float constant = (1 - dampness) / (float)num_nodes;

  while (iter_count < iterations)
  {
    t.start();
    for (node n : nodes)
    {
      float sum = 0.0f;
      vector<node> in_nodes =
          graph.get_in_nodes(n.id);  // <-- make this just list of node_ids to
                                     // avoid looking up node table

      for (node in : in_nodes)
      {
        sum += (ptr[in.id].p_rank[p_cur]) / (float)in.out_degree;
      }
      ptr[n.id].p_rank[p_next] = constant + (dampness * sum);
    }
    iter_count++;

    p_cur = 1 - p_cur;
    p_next = 1 - p_next;

    t.stop();
    cout << "Iter " << iter_count << "took \t" << to_string(t.t_micros())
         << endl;
    times.push_back(t.t_micros());
  }
  print_to_csv(
      opts.db_name, times, csv_logdir + "/" + opts.db_name + "_old_pr.csv");
  std::string map_file = csv_logdir + "/" + opts.db_name + "_pr_map.txt";
  print_map(map_file, num_nodes, ptr, p_next);
  delete_map(num_nodes);
}

int main(int argc, char *argv[])
{
  cout << "Running PageRank" << endl;
  PageRankOpts pr_cli(argc, argv, 1e-4, 10);
  if (!pr_cli.parse_args())
  {
    return -1;
  }

  cmdline_opts opts = pr_cli.get_parsed_opts();
  opts.stat_log += "/" + opts.db_name;

  const int THREAD_NUM = 1;
  // set_group_id();  // set the group id to GRAPHS_GROUP_ID

  Times t;
  t.start();
  GraphEngine graphEngine(THREAD_NUM, opts);
  GraphBase *graph = graphEngine.create_graph_handle();
  t.stop();
  cout << "Graph loaded in " << t.t_micros() << endl;

  opts.dump_cmd_config(opts.stat_log + "pr_config");

  // Now run PR
  t.start();
  pagerank(*graph, opts, opts.iterations, opts.tolerance, opts.stat_log);
  t.stop();
  cout << "PR  completed in : " << t.t_micros() << endl;
  graph->close(false);
  graphEngine.close_graph();
}
