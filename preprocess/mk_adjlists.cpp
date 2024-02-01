//
// Created by puneet on 29/01/24.
//
#include <iostream>
#include "common_util.h"
#include "time_structs.h"
#include "mk_adjlists.h"

int main (int argc, char** argv)
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

    num_per_chunk = (int)(opts.num_edges / NUM_THREADS);

    //dump the opts
    std::cout << "Dataset: " << dataset << std::endl;
    std::cout << "Num Edges: " << opts.num_edges << std::endl;
    std::cout << "Num Nodes: " << opts.num_nodes << std::endl;
    std::cout << "Read Optimized: " << opts.read_optimize << std::endl;
    std::cout << "Is Directed: " << opts.is_directed << std::endl;
    std::cout << "Num Per Chunk: " << num_per_chunk << std::endl;
    std::cout << "Type Opt: " << type_opt << std::endl;
    std::cout << "Num Threads: " << NUM_THREADS << std::endl;
    std::cout << "dbname: " << opts.db_name << std::endl;

#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        insert_edge_thread(i);
    }
    //merge the conflicts
    merge_conflicts();

    std::cout << "\n\nNow repeat this process for the reversed graph\n\n";
    dataset = opts.dataset + "_reverse";

#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        insert_edge_thread(i);
    }
    // print the conflicts
    // print_conflict_map();
    // merge the conflicts
    merge_conflicts();
}

