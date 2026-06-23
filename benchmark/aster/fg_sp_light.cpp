/**
 * fg_sp_light.cpp — Single-pair shortest path matching the Gremlin baselines.
 *
 * Equivalent to:
 *   g.V(src).repeat(out().where(without("x")).aggregate("x"))
 *     .until(hasId(dst)).limit(1).path().count(local)
 *
 * Plain BFS from source to destination (unweighted), returns path length.
 * Single-threaded, stops as soon as destination is found.
 *
 * Usage: fg_sp_light -p <db_dir> -m <db_name> -g adj -r [-v <source>]
 *        [--dst=N]
 *
 * Output matches the Gremlin format:
 *   Shortest Path from <src> to <dst> having <len> path in <ns> ns
 */

#include <iostream>
#include <queue>
#include <unordered_set>
#include <vector>

#include "command_line.h"
#include "graph_engine.h"
#include "mem_usage.h"
#include "times.h"

using namespace std;

int main(int argc, char *argv[])
{
  node_id_t dst = OutOfBand_ID_MAX;
  vector<char *> clean_argv;
  for (int i = 0; i < argc; i++)
  {
    string arg(argv[i]);
    if (arg.rfind("--dst=", 0) == 0)
      dst = (node_id_t)stoul(arg.substr(6));
    else
      clean_argv.push_back(argv[i]);
  }

  int clean_argc = clean_argv.size();
  CmdLineApp cli(clean_argc, clean_argv.data());
  if (!cli.parse_args()) return -1;

  cmdline_opts opts = cli.get_parsed_opts();
  opts.stat_log += "/" + opts.db_name;
  opts.read_only = true;
  opts.create_new = false;

  Times t;
  t.start();
  GraphEngine engine(1, opts);
  string checkpoint = engine.make_checkpoint();
  GraphBase *graph = engine.create_ro_graph_handle(checkpoint);
  t.stop();
  cout << "Graph loaded in " << t.t_secs() << "s" << endl;

  node_id_t source = opts.start_vertex;
  if (source == OutOfBand_ID_MAX) source = graph->get_random_node().id;
  if (dst == OutOfBand_ID_MAX)
  {
    cerr << "ERROR: --dst=<vertex_id> is required" << endl;
    return 1;
  }

  for (int trial = 0; trial < opts.num_trials; trial++)
  {
    auto t0 = chrono::high_resolution_clock::now();

    int64_t path_len = -1;
    unordered_set<node_id_t> visited;
    // BFS: queue holds (vertex, depth)
    queue<pair<node_id_t, int64_t>> frontier;
    frontier.push({source, 0});
    visited.insert(source);

    while (!frontier.empty())
    {
      auto [vid, d] = frontier.front();
      frontier.pop();

      if (vid == dst)
      {
        path_len = d;
        break;
      }

      vector<node_id_t> neighbors = graph->get_out_nodes_id(vid);
      for (node_id_t nbr : neighbors)
      {
        if (visited.insert(nbr).second)
        {
          frontier.push({nbr, d + 1});
        }
      }
    }

    auto t1 = chrono::high_resolution_clock::now();
    long long ns =
        chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();

    cout << "Shortest Path from " << source << " to " << dst << " having "
         << path_len << " path in " << ns << " ns" << endl;
  }

  graph->close(false);
  engine.close_graph();
  return 0;
}
