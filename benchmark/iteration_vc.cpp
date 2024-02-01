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
#include "platform_atomics.h"
#include "pvector.h"
#include "standard_graph.h"
#include "times.h"

int main(int argc, char* argv[])
{
    std::cout << "Running Iteration" << std::endl;
    CmdLineApp iter_cli(argc, argv);
    if (!iter_cli.parse_args())
    {
        return -1;
    }

    graph_opts opts;
    get_graph_opts(iter_cli, opts);
    opts.stat_log = iter_cli.get_logdir() + "/" + opts.db_name;

    const int THREAD_NUM = 1;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    int num_trials = iter_cli.get_num_trials();

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    graphEngine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < num_trials; i++)
    {
        uint64_t counter = 0;
        iter_info info(0);
        t.start();
#pragma omp parallel for num_threads(THREAD_NUM)
        for (int i = 0; i < THREAD_NUM; i++)
        {
            GraphBase* graph = graphEngine.create_graph_handle();
            adjlist found = {0};

            OutCursor* out_cursor = graph->get_outnbd_iter();
            out_cursor->set_key_range(graphEngine.get_key_range(i));
            out_cursor->next(&found);
            while (found.node_id != -1)
            {
                for (node_id_t v : found.edgelist)
                {
                    counter += 1;
                }
                out_cursor->next(&found);
            }

            if (opts.is_directed)
            {
                InCursor* in_cursor = graph->get_innbd_iter();
                in_cursor->set_key_range(graphEngine.get_key_range(i));
                in_cursor->next(&found);
                while (found.node_id != -1)
                {
                    for (node_id_t v : found.edgelist)
                    {
                        counter += 1;
                    }
                    in_cursor->next(&found);
                }
            }
            graph->close();
        }
        t.stop();
        info.time_taken = t.t_micros();
        std::cout << "Iteration over all edges completed in : "
                  << info.time_taken << std::endl;
        print_csv_info(opts.db_name, info, iter_cli.get_logdir());
    }

    return 0;
}