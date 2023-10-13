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
#include "platform_atomics.h"
#include "pvector.h"
#include "standard_graph.h"
#include "times.h"

/**
 * Connected Components
 */

bool Link(node_id_t u, node_id_t v, pvector<node_id_t>& labels)
{
    bool done = true;
    node_id_t p1 = labels[u];
    node_id_t p2 = labels[v];
    while (p1 != p2)
    {
        done = false;
        node_id_t high = p1 > p2 ? p1 : p2;
        node_id_t low = p1 + (p2 - high);
        node_id_t p_high = labels[high];
        // Was already 'low' or succeeded in writing 'low'
        if ((p_high == low) ||
            (p_high == high && compare_and_swap(labels[high], high, low)))
            break;
        p1 = labels[labels[high]];
        p2 = labels[low];
    }
    return done;
}

pvector<node_id_t> connected_components(GraphEngine& graph_engine,
                                        int thread_num,
                                        bool is_directed)
{
    bool done = false;
    GraphBase* stat = graph_engine.create_graph_handle();
    node_id_t numNodes = stat->get_num_nodes();
    stat->close();
    pvector<node_id_t> labels(numNodes);
#pragma omp parallel for
    for (node_id_t n = 0; n < numNodes; n++) labels[n] = n;

    while (!done)
    {
        done = true;
#pragma omp parallel for reduction(& : done) num_threads(thread_num)
        for (int i = 0; i < thread_num; i++)
        {
            GraphBase* graph = graph_engine.create_graph_handle();
            edge found = {0};

            EdgeCursor* edge_cursor = graph->get_edge_iter();
            edge_cursor->set_key_range(graph_engine.get_edge_range(i));
            edge_cursor->next(&found);
            while (found.src_id != -1)
            {
                done &= Link(found.src_id, found.dst_id, labels);
                edge_cursor->next(&found);
            }
            graph->close();
        }

#pragma omp parallel for schedule(dynamic, 16384)
        for (node_id_t i = 0; i < numNodes; i++)
        {
            while (labels[i] != labels[labels[i]])
            {
                labels[i] = labels[labels[i]];
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
    const int THREAD_NUM = 8;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    int num_trials = cc_cli.get_num_trials();

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    graphEngine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < num_trials; i++)
    {
        cc_info info(0);
        t.start();
        pvector<node_id_t> labels =
            connected_components(graphEngine, THREAD_NUM, opts.is_directed);
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
        print_csv_info(opts.db_name, info, cc_log);
    }

    return 0;
}