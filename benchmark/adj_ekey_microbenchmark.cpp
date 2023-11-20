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

#define HUB

#define ITERS 50

#define HUB_COUNT 100000

int main(int argc, char* argv[])
{
    std::cout << "Running Iteration" << std::endl;
    CmdLineApp iter_cli(argc, argv);
    if (!iter_cli.parse_args())
    {
        return -1;
    }

    graph_opts opts;

    opts.create_new = iter_cli.is_create_new();
    opts.is_directed = iter_cli.is_directed();
    opts.read_optimize = iter_cli.is_read_optimize();
    opts.is_weighted = iter_cli.is_weighted();
    opts.optimize_create = iter_cli.is_create_optimized();
    opts.db_name = iter_cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = iter_cli.get_db_path();
    std::string iter_log = iter_cli.get_logdir();  //$RESULT/$bmark
    opts.stat_log = iter_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";  // tc_cli.get_conn_config();
    opts.type = iter_cli.get_graph_type();
    const int THREAD_NUM = 4;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    int num_trials = iter_cli.get_num_trials();
    EdgeCursor* edge_cursor;
    OutCursor* out_cursor;
    node_id_t counter1 = 0;
    node_id_t counter2 = 0;

    Times t;
    edge e = {0};
    adjlist adj = {0};
#ifdef HUB
    GraphEngine graphEngineHub(engine_opts);

    t.start();
    GraphBase* graphHub = graphEngineHub.create_graph_handle();
    for (int i = 0; i < HUB_COUNT; i++)
    {
        int err = graphHub->add_edge(edge{1, 2 + i}, false);
        if (i % 10000)
        {
            cout << "Percentage finished:" << i / 10000 << '\n';
        }
    }
    graphHub->close();
    t.stop();

    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < ITERS; i++)
    {
        graphHub = graphEngineHub.create_graph_handle();
        edge_cursor = graphHub->get_edge_iter();
        t.start();
        edge_cursor->next(&e);
        while (e.src_id != -1)
        {
            edge_cursor->next(&e);
            counter1++;
        }
        t.stop();

        std::cout << "Hub traversal completed in : " << t.t_micros()
                  << std::endl;

        out_cursor = graphHub->get_outnbd_iter();
        t.start();
        out_cursor->next(&adj);
        while (adj.node_id != -1)
        {
            for (node_id_t v : adj.edgelist)
            {
                counter2++;
            }
            out_cursor->next(&adj);
        }
        graphHub->close();
        t.stop();
        assert(counter1 == counter2);
        std::cout << "Hub traversal completed in : " << t.t_micros()
                  << std::endl;
    }
#else
    GraphEngine graphEngineEdge(engine_opts);

    t.start();
    GraphBase* graphEdge = graphEngineEdge.create_graph_handle();
    for (int i = 0; i < HUB_COUNT; i++)
    {
        graphEdge->add_edge(edge{i, i + 1}, false);
    }
    graphEdge->close();
    t.stop();

    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < ITERS; i++)
    {
        graphEdge = graphEngineEdge.create_graph_handle();
        edge_cursor = graphEdge->get_edge_iter();
        t.start();
        edge_cursor->next(&e);
        while (e.src_id != -1)
        {
            edge_cursor->next(&e);
            counter1++;
        }
        t.stop();

        std::cout << "Edge traversal completed in : " << t.t_micros()
                  << std::endl;

        out_cursor = graphEdge->get_outnbd_iter();
        t.start();
        out_cursor->next(&adj);
        while (adj.node_id != -1)
        {
            for (node_id_t v : adj.edgelist)
            {
                counter2++;
            }
            out_cursor->next(&adj);
        }
        graphEdge->close();
        t.stop();
        assert(counter1 == counter2);
        std::cout << "Edge traversal completed in : " << t.t_micros()
                  << std::endl;
    }
#endif
    return 0;
}
