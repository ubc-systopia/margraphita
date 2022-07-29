#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <numeric>
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
#include "thread_utils.h"
#include "times.h"

template <typename Graph>
wcc_lp_info *wcc_lp(Graph &graph)
{
    Times timer;
    wcc_lp_info *info = new wcc_lp_info(0);
    timer.start();
    uint64_t num_nodes = graph->get_num_nodes();
    vector<uint64_t> label(num_nodes, 0);
    std::iota(label.begin(), label.end(), 0);
    bool done = false;
    while (!done)
    {
        done = true;
        EdgeCursor *edge_iter = graph->get_edge_iter();
        edge curr_edge;
        edge_iter->next(&curr_edge);
        while (curr_edge.src_id != -1)
        {
            if (label[curr_edge.dst_id] != label[curr_edge.src_id])
            {
                done = false;
                if (label[curr_edge.dst_id] < label[curr_edge.src_id])
                {
                    label[curr_edge.src_id] = label[curr_edge.dst_id];
                }
                else
                {
                    label[curr_edge.dst_id] = label[curr_edge.src_id];
                }
            }
        }
        for (uint64_t i = 0; i < num_nodes; i++)
        {
            while (label[i] != label[label[i]])
            {
                label[i] = label[label[i]];
            }
        }
        info->iterations++;
    }

    timer.stop();

    for (uint64_t i = 0; i < num_nodes - 1; i++)
    {
        if (label[i] != label[i + 1])
        {
            info->components++;
        }
    }

    info->time_taken = timer.t_micros();
    // average the time taken to get average per iteration.
    info->time_taken = info->time_taken / 10;
    return info;
}

int main(int argc, char *argv[])
{
    cout << "Running Weakly Connected Components/Label Propogation" << endl;
    CmdLineApp wcc_lp_cli(argc, argv);
    if (!wcc_lp_cli.parse_args())
    {
        return -1;
    }

    graph_opts opts;
    opts.create_new = wcc_lp_cli.is_create_new();
    opts.is_directed = wcc_lp_cli.is_directed();
    opts.read_optimize = wcc_lp_cli.is_read_optimize();
    opts.is_weighted = wcc_lp_cli.is_weighted();
    opts.optimize_create = wcc_lp_cli.is_create_optimized();
    opts.db_name = wcc_lp_cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = wcc_lp_cli.get_db_path();
    std::string wcc_lp_log = wcc_lp_cli.get_logdir();  //$RESULT/$bmark
    opts.stat_log = wcc_lp_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";
    opts.type = wcc_lp_cli.get_graph_type();

    const int THREAD_NUM = 1;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    Times timer;
    timer.start();
    GraphEngine graphEngine(engine_opts);
    GraphBase *graph = graphEngine.create_graph_handle();
    timer.stop();
    std::cout << "Graph loaded in " << timer.t_micros() << std::endl;

    // do 10 runs with random starting nodes and run 10 trial per each
    // random node
    int num_trials = wcc_lp_cli.get_num_trials();
#ifdef STAT
    num_trials = 1;  // We want only one run with stats collection
#endif
    for (int i = 0; i < num_trials; i++)
    {
        timer.start();
        wcc_lp_info *wcc_lp_run = wcc_lp(graph);
        timer.stop();
        double time_from_outside = timer.t_micros();
        std::cout << "WCC/LP completed in: " << time_from_outside << std::endl;
        print_csv_info(opts.db_name, wcc_lp_run, time_from_outside, wcc_lp_log);
    }
    graph->close();
    graphEngine.close_graph();
}
