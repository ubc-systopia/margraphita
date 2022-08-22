#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

#include "adj_list.h"
#include "benchmark_definitions.h"
#include "command_line.h"
#include "common.h"
#include "csv_log.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"

/**
 * Connected Components for undirected graph
 */

std::vector<node_id_t> connected_components(GraphBase* graph)
{
    node_id_t numNodes = graph->get_num_nodes();
    std::vector<node_id_t> labels = std::vector<node_id_t>(numNodes);
    int comp_num = 0;
    for (node_id_t i = 0; i < numNodes; i++)
    {
        labels[i] = -1;
    }

    for (node_id_t v = 0; v < numNodes; v++)
    {
        if (labels[v] == -1)
        {
            comp_num += 1;
            std::queue<node_id_t> frontier;
            frontier.push(v);
            labels[v] = v;

            while (!frontier.empty())
            {
                node_id_t u = frontier.front();
                frontier.pop();
                for (node_id_t k : graph->get_out_nodes_id(u))
                {
                    if (labels[k] == -1)
                    {
                        labels[k] = v;
                        frontier.push(k);
                    }
                }
            }
        }
    }

    cout << comp_num << " components counted";
    return labels;
}

int main(int argc, char* argv[])
{
    std::cout << "Running CC" << std::endl;
    CmdLineApp cc_cli(argc, argv);
    if (!cc_cli.parse_args())
    {
        return -1;
    }

    graph_opts opts;
    opts.create_new = cc_cli.is_create_new();
    opts.is_directed = cc_cli.is_directed();
    opts.read_optimize = cc_cli.is_read_optimize();
    opts.is_weighted = cc_cli.is_weighted();
    opts.optimize_create = cc_cli.is_create_optimized();
    opts.db_name = cc_cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = cc_cli.get_db_path();
    std::string cc_log = cc_cli.get_logdir();  //$RESULT/$bmark
    opts.stat_log = cc_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";  // tc_cli.get_conn_config();
    opts.type = cc_cli.get_graph_type();
    const int THREAD_NUM = 1;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    int num_trials = cc_cli.get_num_trials();

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    GraphBase* graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < num_trials; i++)
    {
        cc_info info(0);
        t.start();
        info.component_count = connected_components(graph).size();
        t.stop();

        info.time_taken = t.t_micros();
        std::cout << "Connected components completed in : " << info.time_taken
                  << std::endl;
        std::cout << "Connencted components count = " << info.component_count
                  << std::endl;
        print_csv_info(opts.db_name, info, cc_log);
    }

    return 0;
}