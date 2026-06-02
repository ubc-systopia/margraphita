/**
 * fg_cdlp.cpp — Community Detection via Label Propagation for FlexoGraph.
 *
 * Based on the Teseo CDLP implementation in gtx_driver.
 * Each node starts with its own ID as label. In each iteration, every node
 * adopts the most frequent label among its neighbors (ties broken by smallest
 * label). Repeats until convergence or max_iterations.
 *
 * Usage: fg_cdlp -p <db_dir> -m <db_name> -g adj -r [--max_iter=N]
 */

#include <atomic>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "command_line.h"
#include "graph_engine.h"
#include "mem_usage.h"
#include "omp.h"
#include "pvector.h"
#include "times.h"

using namespace std;

const int THREAD_NUM = omp_get_max_threads();
vector<GraphBase *> graph_handles(THREAD_NUM);

void create_graph_handles(GraphEngine &engine, string &checkpoint, int nthreads)
{
  graph_handles.resize(nthreads);
#pragma omp parallel for num_threads(nthreads)
  for (int i = 0; i < nthreads; i++)
  {
    graph_handles[i] = engine.create_ro_graph_handle(checkpoint);
  }
}

void cdlp(GraphEngine &engine, node_id_t max_node_id, int max_iterations)
{
  pvector<node_id_t> labels0(max_node_id);
  pvector<node_id_t> labels1(max_node_id);

  // Initialize: each node's label = its own ID
#pragma omp parallel for
  for (node_id_t v = 0; v < max_node_id; v++)
    labels0[v] = v;

  bool change = true;
  int iter = 0;
  while (iter < max_iterations && change)
  {
    change = false;
    iter++;

#pragma omp parallel for
    for (int t = 0; t < THREAD_NUM; t++)
    {
      GraphBase *graph = graph_handles[omp_get_thread_num()];
      auto *cur = graph->get_outnbd_iter();
      cur->set_key_range(engine.get_key_range(t));

      adjlist u;
      cur->next(&u);
      while (u.node_id != OutOfBand_ID_MAX)
      {
        unordered_map<node_id_t, node_id_t> histogram;
        for (node_id_t nbr : u.edgelist)
        {
          histogram[labels0[nbr]]++;
        }

        // Find most frequent label (smallest label breaks ties)
        node_id_t best_label = labels0[u.node_id];
        node_id_t best_count = 0;
        for (const auto &p : histogram)
        {
          if (p.second > best_count ||
              (p.second == best_count && p.first < best_label))
          {
            best_label = p.first;
            best_count = p.second;
          }
        }

        labels1[u.node_id] = best_label;
        if (labels0[u.node_id] != best_label) change = true;

        u.clear();
        cur->next(&u);
      }
      cur->close();
      delete cur;
    }

    labels0.swap(labels1);
  }

  cout << "CDLP converged in " << iter << " iterations" << endl;
}

int main(int argc, char *argv[])
{
  cout << "Running CDLP" << endl;
  mem_util::MemoryCounter memory_usage;

  // Extract --max_iter before CmdLineApp
  int max_iterations = 10;
  vector<char *> clean_argv;
  for (int i = 0; i < argc; i++)
  {
    string arg(argv[i]);
    if (arg.rfind("--max_iter=", 0) == 0)
      max_iterations = stoi(arg.substr(11));
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
  GraphEngine engine(THREAD_NUM, opts);
  string checkpoint = engine.make_checkpoint();
  engine.set_partition_strategy(PartitionStrategy::NODE_COUNT);
  engine.calculate_thread_offsets();
  create_graph_handles(engine, checkpoint, THREAD_NUM);
  t.stop();
  cout << "Graph loaded in " << t.t_secs() << "s" << endl;

  GraphBase *graph = graph_handles[0];
  node_id_t max_node_id = graph->get_max_node_id();
  node_id_t num_nodes = graph->get_num_nodes();
  cout << "max_node_id=" << max_node_id << " num_nodes=" << num_nodes << endl;

  for (int trial = 0; trial < opts.num_trials; trial++)
  {
    t.start();
    cdlp(engine, max_node_id, max_iterations);
    t.stop();
    cout << "CDLP completed in " << t.t_secs() << "s" << endl;
  }

  for (int i = 0; i < THREAD_NUM; i++)
    graph_handles[i]->close(false);

  engine.close_graph();
  return 0;
}
