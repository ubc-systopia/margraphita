#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "times.h"
#include <sched.h>
#ifdef DEBUG
#include <ittnotify.h>
__itt_domain* domain = __itt_domain_create("MyTraces.MyDomain");
__itt_string_handle* edgeScan_hub =
    __itt_string_handle_create("HUB EdgeScan Task");
__itt_string_handle* outnbdScan_hub =
    __itt_string_handle_create("HUB OutNbd Scan Task");
__itt_string_handle* edgeScan =
    __itt_string_handle_create("no-hub EdgeScan Task");
__itt_string_handle* outnbdScan =
    __itt_string_handle_create("no-hub OutNbd Scan Task");

#endif

#define HUB
#define HUB_BREAK

#define ITERS 50

#define HUB_COUNT 100000


int main(int argc, char* argv[])
{
    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    int result = sched_setaffinity(0, sizeof(mask), &mask);
    
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
    std::ofstream stat_file;
    std::string stat_file_name = opts.stat_log + "/adj_ekey_microbenchmark";
#ifdef DEBUG
    stat_file_name = stat_file_name + "_debug";
#endif

#ifdef HUB
    #ifdef HUB_BREAK
        stat_file_name = stat_file_name + "_hub_break";
    #endif
    stat_file_name = stat_file_name + "_hub.log";
#else
    stat_file_name = stat_file_name + "_no_hub.log";
#endif
    std::cout << "Stat file name: " << stat_file_name << std::endl;
    stat_file.open(stat_file_name, std::ios::app);
    if (std::filesystem::file_size(stat_file_name) == 0)
    {
        stat_file << "edge_cursor, out_nbd_cursor" << std::endl;
    }

#ifdef HUB
    GraphEngine graphEngineHub(engine_opts);
    GraphBase* graphHub = nullptr;
    if (opts.create_new)
    {
        t.start();
        graphHub = graphEngineHub.create_graph_handle();
        for (int i = 0; i < HUB_COUNT; i++)
        {
            edge e{.src_id = 1, .dst_id = i + 2};
            int err = graphHub->add_edge(e, false);
            if (i % 1000 == 0)
            {
                cout << "Percentage finished:" << i / 1000 << '\n';
            }
        }
        graphHub->close();
        t.stop();
        std::cout << "Graph loaded in " << t.t_micros() << std::endl;
    }
    for (int i = 0; i < ITERS; i++)
    {
        graphHub = graphEngineHub.create_graph_handle();
        edge_cursor = graphHub->get_edge_iter();
        t.start();
#ifdef DEBUG
        __itt_task_begin(domain, __itt_null, __itt_null, edgeScan_hub);
#endif
        edge_cursor->next(&e);
        while (e.src_id != -1)
        {
            edge_cursor->next(&e);
            counter1++;
        }
#ifdef DEBUG
        __itt_task_end(domain);
#endif
        t.stop();
        stat_file << t.t_micros() << ",";
        std::cout << "Hub traversal completed in : " << t.t_micros()
                  << std::endl;
#ifdef STAT
        // return 0;
#endif
        out_cursor = graphHub->get_outnbd_iter();
        t.start();
#ifdef DEBUG
        __itt_task_begin(domain, __itt_null, __itt_null, outnbdScan_hub);
#endif
        out_cursor->next(&adj);
        while (adj.node_id != -1)
        {
            if(opts.type == GraphType::EKey)
            {
                std::cout << "adj.node_id: " << adj.node_id << "has " << adj.edgelist.size() <<" neighbours" <<std::endl;
            }
            for (node_id_t v : adj.edgelist)
            {
                counter2++;
            }
// #ifdef HUB_BREAK
//             break;//for hub node, only iterate over the core.
// #endif
            out_cursor->next(&adj);
        }
#ifdef DEBUG
        __itt_task_end(domain);
#endif
        graphHub->close();
        t.stop();
        std::cout <<"counter1(edge): "<< counter1 << " counter2:(adjlist) " << counter2 << std::endl;
        assert(counter1 == counter2);
        stat_file << t.t_micros() << std::endl;
        std::cout << "Hub traversal completed in : " << t.t_micros()
                  << std::endl;
    }
#else
    GraphEngine graphEngineEdge(engine_opts);

    t.start();
    GraphBase* graphEdge = graphEngineEdge.create_graph_handle();
    for (int i = 0; i < HUB_COUNT; i++)
    {
        edge e{.src_id = i, .dst_id = i + 1};
        graphEdge->add_edge(e, false);
    }
    graphEdge->close();
    t.stop();

    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < ITERS; i++)
    {
        graphEdge = graphEngineEdge.create_graph_handle();
        edge_cursor = graphEdge->get_edge_iter();
        t.start();
#ifdef DEBUG
        __itt_task_begin(domain, __itt_null, __itt_null, edgeScan);
#endif
        edge_cursor->next(&e);
        while (e.src_id != -1)
        {
            edge_cursor->next(&e);
            counter1++;
        }
#ifdef DEBUG
        __itt_task_end(domain);
#endif
        t.stop();
        stat_file << t.t_micros() << ",";
        std::cout << "(non-hub)Edge traversal completed in : " << t.t_micros()
                  << std::endl;

        out_cursor = graphEdge->get_outnbd_iter();
        t.start();
#ifdef DEBUG
        __itt_task_begin(domain, __itt_null, __itt_null, outnbdScan);
#endif
        out_cursor->next(&adj);
        while (adj.node_id != -1)
        {
            for (node_id_t v : adj.edgelist)
            {
                counter2++;
            }
            out_cursor->next(&adj);
        }
#ifdef DEBUG
        __itt_task_end(domain);
#endif
        graphEdge->close();
        t.stop();
        stat_file << t.t_micros() << std::endl;
        assert(counter1 == counter2);
        std::cout << "Edge traversal completed in : " << t.t_micros()
                  << std::endl;
    }
#endif
    stat_file.close();
    return 0;
}
