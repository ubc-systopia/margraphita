/**
 * fg_aster_bench.cpp — FlexoGraph benchmark for Aster artifact evaluation.
 *
 * Modes:
 *   fig6  — mixed get_out_degree + add_edge throughput (matches reproduce_script.sh Fig 6)
 *   fig7a — individual operation latencies: get, addv, adde, dele (Fig 7A-B)
 *
 * Usage:
 *   fg_aster_bench -p <db_dir> -m <db_name> -g adj -r --mode=<fig6|fig7a>
 *                  [--rops=N --wops=N] [--cache_size=<bytes>]
 *
 * Output format matches the awk parsers in reproduce_script.sh.
 */

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "command_line.h"
#include "graph_engine.h"

using namespace std;
using Clock = chrono::high_resolution_clock;

// ── Helpers ─────────────────────────────────────────────────────────────────

static vector<node_id_t> collect_valid_ids(GraphBase *graph)
{
  vector<node_id_t> ids;
  node_id_t max_id = graph->get_max_node_id();
  ids.reserve(max_id);
  for (node_id_t i = 0; i < max_id; i++)
  {
    if (graph->has_node(i)) ids.push_back(i);
  }
  return ids;
}

static node_id_t random_id(const vector<node_id_t> &ids, mt19937_64 &rng)
{
  return ids[rng() % ids.size()];
}

// ── fig6: mixed throughput ──────────────────────────────────────────────────

static void run_fig6(GraphBase *graph, int rops, int wops,
                     const vector<node_id_t> &valid_ids, mt19937_64 &rng)
{
  // Build a shuffled schedule: 0 = read, 1 = write
  int total = rops + wops;
  vector<int> schedule(total);
  fill(schedule.begin(), schedule.begin() + rops, 0);
  fill(schedule.begin() + rops, schedule.end(), 1);
  shuffle(schedule.begin(), schedule.end(), rng);

  double get_total_us = 0, add_total_us = 0;
  int get_count = 0, add_count = 0;

  for (int i = 0; i < total; i++)
  {
    if (schedule[i] == 0)
    {
      // read: get_out_degree
      node_id_t vid = random_id(valid_ids, rng);
      auto t0 = Clock::now();
      graph->get_out_degree(vid);
      auto t1 = Clock::now();
      get_total_us +=
          chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count() / 1000.0;
      get_count++;
    }
    else
    {
      // write: add_edge
      node_id_t src = random_id(valid_ids, rng);
      node_id_t dst = random_id(valid_ids, rng);
      edge e;
      e.src_id = src;
      e.dst_id = dst;
      auto t0 = Clock::now();
      graph->add_edge(e, false);
      auto t1 = Clock::now();
      add_total_us +=
          chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count() / 1000.0;
      add_count++;
    }
  }

  double get_avg = get_count > 0 ? get_total_us / get_count : 0;
  double add_avg = add_count > 0 ? add_total_us / add_count : 0;

  // Output matches awk pattern: get: <X>, add: <Y>
  cout << "get: " << get_avg << ", add: " << add_avg << endl;
}

// ── fig7a: individual operation latencies ───────────────────────────────────

static void run_fig7a(GraphBase *graph, const vector<node_id_t> &valid_ids,
                      mt19937_64 &rng)
{
  const int OPS = 100000;

  // 1. get_neighbors (get out-neighbor IDs)
  {
    auto t0 = Clock::now();
    for (int i = 0; i < OPS; i++)
    {
      node_id_t vid = random_id(valid_ids, rng);
      graph->get_out_nodes_id(vid);
    }
    auto t1 = Clock::now();
    double avg_us =
        chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count() /
        (1000.0 * OPS);
    cout << "get avg: " << avg_us << " us" << endl;
  }

  // 2. add_vertex
  node_id_t next_id = graph->get_max_node_id();
  {
    auto t0 = Clock::now();
    for (int i = 0; i < OPS; i++)
    {
      node n;
      n.id = next_id++;
      graph->add_node(n, false);
    }
    auto t1 = Clock::now();
    double avg_us =
        chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count() /
        (1000.0 * OPS);
    cout << "addv avg: " << avg_us << " us" << endl;
  }

  // 3. add_edge (save edges for later deletion)
  vector<pair<node_id_t, node_id_t>> added_edges;
  added_edges.reserve(OPS);
  {
    auto t0 = Clock::now();
    for (int i = 0; i < OPS; i++)
    {
      node_id_t src = random_id(valid_ids, rng);
      node_id_t dst = random_id(valid_ids, rng);
      edge e;
      e.src_id = src;
      e.dst_id = dst;
      graph->add_edge(e, false);
      added_edges.emplace_back(src, dst);
    }
    auto t1 = Clock::now();
    double avg_us =
        chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count() /
        (1000.0 * OPS);
    cout << "adde avg: " << avg_us << " us" << endl;
  }

  // 4. del_edge (delete previously added edges)
  {
    auto t0 = Clock::now();
    for (int i = 0; i < OPS; i++)
    {
      graph->delete_edge(added_edges[i].first, added_edges[i].second);
    }
    auto t1 = Clock::now();
    double avg_us =
        chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count() /
        (1000.0 * OPS);
    cout << "dele avg: " << avg_us << " us" << endl;
  }
}

// ── main ────────────────────────────────────────────────────────────────────

static void print_usage()
{
  cerr << "Usage: fg_aster_bench -p <db_dir> -m <db_name> -g <graph_type> -r"
       << endl;
  cerr << "       --mode=<fig6|fig7a> [--rops=N --wops=N]" << endl;
}

int main(int argc, char *argv[])
{
  // Parse custom args before CmdLineApp consumes them
  string mode;
  int rops = 100000, wops = 900000;

  // Extract our custom flags, build a cleaned argv for CmdLineApp
  vector<char *> clean_argv;
  for (int i = 0; i < argc; i++)
  {
    string arg(argv[i]);
    if (arg.rfind("--mode=", 0) == 0)
    {
      mode = arg.substr(7);
    }
    else if (arg.rfind("--rops=", 0) == 0)
    {
      rops = stoi(arg.substr(7));
    }
    else if (arg.rfind("--wops=", 0) == 0)
    {
      wops = stoi(arg.substr(7));
    }
    else
    {
      clean_argv.push_back(argv[i]);
    }
  }

  if (mode.empty())
  {
    print_usage();
    return 1;
  }

  int clean_argc = clean_argv.size();
  CmdLineApp cli(clean_argc, clean_argv.data());
  if (!cli.parse_args()) return 1;

  cmdline_opts opts = cli.get_parsed_opts();
  opts.read_only = false;  // we need to write (add_edge, add_node)
  opts.create_new = false;

  GraphEngine engine(1, opts);
  GraphBase *graph = engine.create_graph_handle();

  cout << "Collecting valid node IDs..." << flush;
  vector<node_id_t> valid_ids = collect_valid_ids(graph);
  cout << " " << valid_ids.size() << " nodes" << endl;

  mt19937_64 rng(42);

  if (mode == "fig6")
  {
    cout << "rops: " << rops << " wops: " << wops << endl;
    run_fig6(graph, rops, wops, valid_ids, rng);
  }
  else if (mode == "fig7a")
  {
    run_fig7a(graph, valid_ids, rng);
  }
  else
  {
    cerr << "Unknown mode: " << mode << endl;
    print_usage();
    graph->close(false);
    engine.close_graph();
    return 1;
  }

  graph->close(false);
  engine.close_graph();
  return 0;
}
