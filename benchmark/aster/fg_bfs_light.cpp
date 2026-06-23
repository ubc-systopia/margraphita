/**
 * fg_bfs_light.cpp — Depth-bounded BFS matching the Gremlin baselines.
 *
 * Equivalent to: g.V(src).repeat(out()).emit().times(depth).dedup().toList()
 * Single-threaded, depth-limited (default 5), counts unique reachable vertices.
 *
 * Usage: fg_bfs_light -p <db_dir> -m <db_name> -g adj -r [-v <source>]
 *        [--depth=N]
 *
 * Output matches the Gremlin format:
 *   BFS start from <src> finished in <ns> ns, count: <N>
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
  int depth = 5;
  vector<char *> clean_argv;
  for (int i = 0; i < argc; i++)
  {
    string arg(argv[i]);
    if (arg.rfind("--depth=", 0) == 0)
      depth = stoi(arg.substr(8));
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

  for (int trial = 0; trial < opts.num_trials; trial++)
  {
    auto t0 = chrono::high_resolution_clock::now();

    unordered_set<node_id_t> visited;
    queue<pair<node_id_t, int>> frontier;
    frontier.push({source, 0});
    visited.insert(source);

    while (!frontier.empty())
    {
      auto [vid, d] = frontier.front();
      frontier.pop();
      if (d >= depth) continue;

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

    // -1 to exclude source (Gremlin emit() doesn't include start vertex)
    long long count = (long long)visited.size() - 1;
    cout << "BFS start from " << source << " finished in " << ns
         << " ns, count: " << count << endl;
  }

  graph->close(false);
  engine.close_graph();
  return 0;
}
