#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <deque>
#include <fstream>
#include <iostream>
#include <list>
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

/**
 * This runs the BFS on the graph using src as the starting node.
 */

template <typename Graph>
bfs_info *bfs(Graph &graph, node_id_t src)
{
    Times timer;
    bfs_info *info = new bfs_info(0);
    timer.start();
    for (int i = 0; i < 10; i++)
    {
        set<node_id_t> visited = {src};
        list<node_id_t> queue = {src};
        vector<node_id_t> result;

        while (!queue.empty())
        {
            node_id_t node_id = queue.front();
            queue.pop_front();
            result.push_back(node_id);
            vector<node> out_nbrs = graph->get_out_nodes(node_id);
            info->sum_out_deg += out_nbrs.size();
            for (node nbr : out_nbrs)
            {
                if (visited.find(nbr.id) == visited.end())
                {
                    visited.insert(nbr.id);
                    queue.push_back(nbr.id);
                }
            }
        }
        info->num_visited = visited.size();
    }
    timer.stop();
    info->time_taken = timer.t_micros();
    // average the time taken to get average per iteration.
    info->time_taken = info->time_taken / 10;
    return info;
}

template <typename Graph>
node_id_t find_random_start(Graph &graph)
{
    while (1)
    {
        node random_start = graph->get_random_node();
        std::cout << random_start.id << std ::endl;
        std::cout << "random start vertex is " << random_start.id << std::endl;
        if (graph->get_out_degree(random_start.id) != 0)
        {
            return random_start.id;
        }
        else
        {
            std::cout << "out degree was zero. finding a new random vertex."
                      << std::endl;
        }
    }
}

int main(int argc, char *argv[])
{
    cout << "Running BFS" << endl;
    CmdLineApp bfs_cli(argc, argv);
    if (!bfs_cli.parse_args())
    {
        return -1;
    }

    graph_opts opts;
    opts.create_new = bfs_cli.is_create_new();
    opts.is_directed = bfs_cli.is_directed();
    opts.read_optimize = bfs_cli.is_read_optimize();
    opts.is_weighted = bfs_cli.is_weighted();
    opts.optimize_create = bfs_cli.is_create_optimized();
    opts.db_name = bfs_cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = bfs_cli.get_db_path();
    std::string bfs_log = bfs_cli.get_logdir();  //$RESULT/$bmark
    opts.stat_log = bfs_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";  // bfs_cli.get_conn_config();
    opts.type = bfs_cli.get_graph_type();

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
    int num_trials = bfs_cli.get_num_trials();
#ifdef STAT
    num_trials = 1;  // We want only one run with stats collection
#endif
    for (int i = 0; i < num_trials; i++)
    {
        node_id_t start_vertex = bfs_cli.start_vertex();
        if (start_vertex == -1)
        {
            start_vertex = find_random_start(graph);
        }
        timer.start();
        bfs_info *bfs_run = bfs(graph, start_vertex);
        timer.stop();
        double time_from_outside = timer.t_micros();
        std::cout << "BFS  completed in : " << time_from_outside << std::endl;
        print_csv_info(
            opts.db_name, start_vertex, bfs_run, time_from_outside, bfs_log);
    }
    graph->close();
    graphEngine.close_graph();
}
