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

#define BENCHMARK_SIZE 64000  // multiple of 16

std::vector<std::pair<uint32_t, uint32_t>> to_delete;
std::vector<std::pair<uint32_t, uint32_t>> to_insert;

void setup(GraphEngine* graphEngine)
{
    GraphBase* graph = graphEngine->create_graph_handle();
    uint64_t edge_count = graph->get_num_edges();
    uint64_t node_count = graph->get_num_nodes();

    EdgeCursor* edge_cursor = graph->get_edge_iter();
    edge e;
    edge_cursor->next(&e);
    while (to_delete.size() < BENCHMARK_SIZE)
    {
        if (std::rand() % edge_count < BENCHMARK_SIZE ||
            edge_count - to_delete.size() < BENCHMARK_SIZE)
        {
            to_delete.push_back(
                std::pair<uint32_t, uint32_t>(e.src_id, e.dst_id));
        }
        edge_cursor->next(&e);
    }

    while (to_insert.size() < BENCHMARK_SIZE)
    {
        std::pair<uint32_t, uint32_t> gen = {std::rand() % node_count,
                                             std::rand() % node_count};
        if (!graph->has_edge(gen.first, gen.second))
        {
            to_insert.push_back({gen});
        }
    }
    graph->close();
}

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

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    setup(&graphEngine);
    t.stop();

    std::cout << "Graph loaded in " << t.t_micros() << std::endl;
    iter_info info(0);
    t.start();

    int rollback_count = 0;

#pragma omp parallel for num_threads(THREAD_NUM) reduction(+ : rollback_count)
    for (int i = 0; i < THREAD_NUM; i++)
    {
        GraphBase* graph = graphEngine.create_graph_handle();
        int per_thread = BENCHMARK_SIZE / THREAD_NUM;
        for (int j = i * per_thread; j < (i + 1) * per_thread; j++)
        {
            while (graph->delete_edge(to_delete[j].first,
                                      to_delete[j].second) == WT_ROLLBACK)
            {
                rollback_count++;
            }
            while (graph->add_edge(
                       edge{to_insert[j].first, to_insert[j].second}, false) ==
                   WT_ROLLBACK)
            {
                rollback_count++;
            }
        }
        graph->close();
    }
    t.stop();
    info.time_taken = t.t_micros();
    std::cout << "Insertion-deletion completed in : " << info.time_taken
              << std::endl;
    std::cout << "Rollback count:" << rollback_count << std::endl;
    print_csv_info(opts.db_name, info, iter_log);

    return 0;
}