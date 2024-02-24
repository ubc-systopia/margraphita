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
#include "common_util.h"
#include "csv_log.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"

/**
 * Connected Components for undirected graph
 */

std::vector<node_id_t> connected_components(GraphBase* graph, bool is_directed)
{
    node_id_t numNodes = graph->get_num_nodes();
    std::vector<node_id_t> labels(numNodes);
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
                if (is_directed)
                {
                    for (node_id_t k : graph->get_in_nodes_id(u))
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
    }

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
    get_graph_opts(cc_cli, opts);
    opts.stat_log = cc_cli.get_logdir() + "/" + opts.db_name;

    const int THREAD_NUM = 1;
    int num_trials = cc_cli.get_num_trials();

    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    GraphBase* graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < num_trials; i++)
    {
        cc_info info(0);
        t.start();
        std::vector<node_id_t> labels =
            connected_components(graph, opts.is_directed);
        t.stop();

        for (uint64_t i = 0; i < labels.size() - 1; i++)
        {
            if (labels[i] != labels[i + 1])
            {
                info.component_count++;
            }
        }

        info.time_taken = t.t_micros();
        std::cout << "Connected components completed in : " << info.time_taken
                  << std::endl;
        std::cout << "Connencted components count = " << info.component_count
                  << std::endl;
        print_csv_info(opts.db_name, info, cc_cli.get_logdir());
    }

    return 0;
}