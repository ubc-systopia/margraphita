#include <atomic>
#include <iostream>
#include <set>
#include <unordered_map>

#include "benchmark_definitions.h"
#include "command_line.h"
#include "common_util.h"
#include "graph_engine.h"
#include "mem_usage.h"
#include "omp.h"
#include "pvector.h"
#include "times.h"
/*
GAP Benchmark Suite
Kernel: Connected Components (CC)
Author: Scott Beamer

Will return comp array labelling each vertex with a connected component ID

This CC implementation makes use of the Shiloach-Vishkin [2] algorithm with
implementation optimizations from Bader et al. [1]. Michael Sutton contributed
a fix for directed graphs using the min-max swap from [3], and it also produces
more consistent performance for undirected graphs.

[1] David A Bader, Guojing Cong, and John Feo. "On the architectural
    requirements for efficient execution of graph algorithms." International
    Conference on Parallel Processing, Jul 2005.

[2] Yossi Shiloach and Uzi Vishkin. "An o(logn) parallel connectivity algorithm"
    Journal of Algorithms, 3(1):57–67, 1982.

[3] Kishore Kothapalli, Jyothish Soman, and P. J. Narayanan. "Fast GPU
    algorithms for graph connectivity." Workshop on Large Scale Parallel
    Processing, 2010.
*/

const int THREAD_NUM = omp_get_max_threads();
std::vector<GraphBase*> graph_handles(THREAD_NUM);

// The hooking condition (comp_u < comp_v) may not coincide with the edge's
// direction, so we use a min-max swap such that lower component IDs propagate
// independent of the edge's direction.
pvector<node_id_t> ShiloachVishkin(GraphEngine& g,
                                   std::string& chkpt,
                                   node_id_t numNodes,
                                   node_id_t maxNodeID)
{
  pvector<node_id_t> comp(maxNodeID);
#pragma omp parallel for
  for (node_id_t n = 0; n < maxNodeID; n++) comp[n] = n;

  std::atomic_bool change = true;
  int num_iter = 0;
  while (change)
  {
    change = false;
    num_iter++;
#pragma omp parallel for
    for (int i = 0; i < THREAD_NUM; i++)
    {
      GraphBase* graph = graph_handles[omp_get_thread_num()];
      auto* out_nbd_cur = graph->get_outnbd_iter();
      out_nbd_cur->set_key_range(g.get_key_range(i));

      adjlist u;  // to keep it consistent with gapbs vars
      out_nbd_cur->next(&u);
      while (u.node_id != OutOfBand_ID_MAX)
      {
        for (node_id_t v : u.edgelist)
        {
          node_id_t comp_u = comp[u.node_id];
          node_id_t comp_v = comp[v];
          if (comp_u == comp_v) continue;
          node_id_t high_comp = comp_u > comp_v ? comp_u : comp_v;
          node_id_t low_comp = comp_u + (comp_v - high_comp);
          if (high_comp == comp[high_comp])
          {
            change = true;
            comp[high_comp] = low_comp;
          }
        }
        u.clear();
        out_nbd_cur->next(&u);
      }
      out_nbd_cur->close();
      delete out_nbd_cur;
      // graph->close(false);
    }
#pragma omp parallel for
    for (node_id_t n = 0; n < maxNodeID; n++)
    {
      while (comp[n] != comp[comp[n]])
      {
        comp[n] = comp[comp[n]];
      }
    }
  }
  cout << "Shiloach-Vishkin took " << num_iter << " iterations" << endl;
  return comp;
}

// Returns k pairs with largest values from list of key-value pairs
template <typename KeyT, typename ValT>
std::vector<std::pair<ValT, KeyT>> TopK(
    const std::vector<std::pair<KeyT, ValT>>& to_sort, size_t k)
{
  std::vector<std::pair<ValT, KeyT>> top_k;
  ValT min_so_far = 0;
  for (auto kvp : to_sort)
  {
    if ((top_k.size() < k) || (kvp.second > min_so_far))
    {
      top_k.push_back(std::make_pair(kvp.second, kvp.first));
      std::sort(
          top_k.begin(), top_k.end(), std::greater<std::pair<ValT, KeyT>>());
      if (top_k.size() > k) top_k.resize(k);
      min_so_far = top_k.back().first;
    }
  }
  return top_k;
}

void PrintCompStats(GraphBase* graph,
                    const pvector<node_id_t>& comp,
                    node_id_t numNodes,
                    node_id_t maxNodeID)
{
  cout << endl;
  std::unordered_map<node_id_t, node_id_t> count;
  for (node_id_t comp_i : comp)
  {
    count[comp_i] += 1;
  }
  int k = 5;
  vector<pair<node_id_t, node_id_t>> count_vector;
  count_vector.reserve(count.size());
  for (auto kvp : count) count_vector.emplace_back(kvp);
  vector<pair<node_id_t, node_id_t>> top_k = TopK(count_vector, k);
  k = min(k, static_cast<int>(top_k.size()));
  cout << k << " biggest clusters" << endl;
  for (auto kvp : top_k) cout << kvp.second << ":" << kvp.first << endl;
  cout << "There are " << count.size() << " components" << endl;
}

void print_comp_to_file(const pvector<node_id_t>& comp)
{
  std::ofstream comp_file("cc_comp.txt");
  for (node_id_t comp_i : comp) comp_file << comp_i << std::endl;
  comp_file.close();
}

void create_graph_handles(GraphEngine& graph_engine,
                          std::string& checkpoint_name,
                          int thread_num)
{
  graph_handles.resize(thread_num);
#pragma omp parallel for num_threads(thread_num)
  for (int i = 0; i < thread_num; i++)
  {
    GraphBase* graph = graph_engine.create_ro_graph_handle(checkpoint_name);
    graph_handles[i] = graph;
  }
}

int main(int argc, char* argv[])
{
  std::cout << "Running CC" << std::endl;
  mem_util::MemoryCounter memory_usage;
  CmdLineApp cc_cli(argc, argv);
  if (!cc_cli.parse_args())
  {
    return -1;
  }

  cmdline_opts opts = cc_cli.get_parsed_opts();
  opts.stat_log += "/" + opts.db_name;
  opts.read_only = true;
  opts.create_new = false;

  Times t;
  t.start();
  GraphEngine graphEngine(THREAD_NUM, opts);
  std::string checkpoint = graphEngine.make_checkpoint();
  graphEngine.calculate_thread_offsets();
  create_graph_handles(graphEngine, checkpoint, THREAD_NUM);
  t.stop();

  std::cout << "Graph loaded in " << t.t_secs() << std::endl;

  long double total_seconds = 0;
  for (int i = 0; i < opts.num_trials; i++)
  {
    t.start();
    GraphBase* graph = graph_handles[0];
    node_id_t numNodes = graph->get_num_nodes();
    node_id_t maxNodeID = graph->get_max_node_id();

    auto result = ShiloachVishkin(graphEngine, checkpoint, numNodes, maxNodeID);
    t.stop();
    std::cout << "CC took " << t.t_secs() << " s" << std::endl;
    total_seconds += t.t_secs();
    if (i == 0 && opts.print_stats == true)
      PrintCompStats(graph, result, numNodes, maxNodeID);
  }
  for (int i = 0; i < THREAD_NUM; i++)
  {
    graph_handles[i]->close(false);
  }
  std::cout << "Average CC took " << total_seconds / opts.num_trials << " s"
            << std::endl;
  graphEngine.close_graph();
  return 0;
}