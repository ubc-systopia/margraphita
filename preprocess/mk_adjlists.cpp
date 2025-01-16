//
// Created by puneet on 29/01/24.
//
#include "mk_adjlists.h"

#include <iostream>

#include "common_util.h"
#include "time_structs.h"

int main(int argc, char** argv)
{
  InsertOpts params(argc, argv);
  if (!params.parse_args())
  {
    params.print_help();
    return -1;
  }
  graph_opts opts = params.make_graph_opts();
  dataset = opts.dataset;
  std::string _db_name;

  num_per_chunk = (int)(opts.num_edges / opts.num_threads);

  // dump the opts
  std::cout << "Dataset: " << dataset << std::endl;
  std::cout << "Num Edges: " << opts.num_edges << std::endl;
  std::cout << "Num Nodes: " << opts.num_nodes << std::endl;
  std::cout << "Read Optimized: " << opts.read_optimize << std::endl;
  std::cout << "Is Directed: " << opts.is_directed << std::endl;
  std::cout << "Num Per Chunk: " << num_per_chunk << std::endl;
  std::cout << "Num Threads: " << opts.num_threads << std::endl;
  std::cout << "dbname: " << opts.db_name << std::endl;

#pragma omp parallel for num_threads(opts.num_threads)
  for (int i = 0; i < opts.num_threads; i++)
  {
    insert_edge_thread(i, "out");
  }
  // merge the conflicts
  merge_conflicts("out", opts.num_threads);

  std::cout << "\n\nNow repeat this process for the reversed graph\n\n";
  // change the filename to the reversed graph
  dataset = dataset + "_reverse";

#pragma omp parallel for num_threads(opts.num_threads)
  for (int i = 0; i < opts.num_threads; i++)
  {
    insert_edge_thread(i, "in");
  }
  // print the conflicts
  // print_conflict_map();
  // merge the conflicts
  merge_conflicts("in", opts.num_threads);
}
