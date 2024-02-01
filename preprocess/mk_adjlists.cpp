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
    std::string middle;
    if (opts.read_optimize)
    {
        middle += "r";
    }
    if (opts.is_directed)
    {
        middle += "d";
    }
    dataset = opts.dataset;
    std::string _db_name;
    std::string conn_config = "create,cache_size=10GB";

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
        std::cout <<"dbname: " << opts.db_name << std::endl;


#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        insert_edge_thread(i);
    }

    //print the conflict map
    for(const auto& item: conflicts)
    {
        std::cout << "Thread: " << item.first << std::endl;
        std::cout << "Top: " << std::endl;
        adjlist temp = item.second.first;
        std::cout << "node: " << temp.node_id << ":: ";
        for(auto i: temp.edgelist)
        {
            std::cout << i << " ";
        }
        std::cout << std::endl;
        std::cout << "Bottom: " << std::endl;
        temp = item.second.second;
        std::cout << "node: " << temp.node_id<<":: ";
        for(auto i: temp.edgelist)
        {
            std::cout << i << " ";
        }
        std::cout << "\n----------------------"<< std::endl << "----------------------"<< std::endl;
    }

    //merge the conflicts
    merge_conflicts();
}

//-d cit-Patents  -f /drives/hdd_main/cit-Patents/cit-Patents.txt -t adj -p /home/puneet/db/cit-Patents -l /home/puneet/scratch/adj_rd_cit-Patents.log
