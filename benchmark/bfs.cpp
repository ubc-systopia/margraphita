#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <vector>

#include "adj_list.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"

/**
 * This runs the BFS on the graph using src as the starting node.
 */

using namespace std;

typedef struct bfs_info
{
    int num_visited;
    int sum_out_deg;
    int64_t time_taken;
    bfs_info(int _val)
        : num_visited(_val), sum_out_deg(_val), time_taken(_val){};
} bfs_info;

template <typename Graph>
bfs_info *bfs(Graph &graph, int src)
{
    bfs_info *info = new bfs_info(0);
    auto start = chrono::steady_clock::now();
    for (int i = 0; i < 10; i++)
    {
        set<int> visited = {src};
        list<int> queue = {src};
        vector<int> result;

        while (!queue.empty())
        {
            int node_id = queue.front();
            queue.pop_front();
            result.push_back(node_id);
            vector<node> out_nbrs = graph.get_out_nodes(node_id);
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
    auto end = chrono::steady_clock::now();
    info->time_taken =
        chrono::duration_cast<chrono::microseconds>(end - start).count();
    // average the time taken to get average per iteration.
    info->time_taken = info->time_taken / 10;
    return info;
}

void print_csv_info(std::string name,
                    int starting_node,
                    bfs_info *info,
                    int time_from_outside,
                    std::string csv_logdir)
{
    fstream FILE;
    std::string _name = csv_logdir + "/" + name + "_bfs.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, ios::out | ios::app);
        FILE << "#db_name, benchmark, starting node_id, num_visited, "
                "sum_out_deg, total_time_taken_usecs, time_from_main\n";
    }
    else
    {
        FILE.open(_name, ios::out | ios::app);
    }

    FILE << name << ",bfs," << starting_node << "," << info->num_visited << ","
         << info->sum_out_deg << "," << info->time_taken << ","
         << time_from_outside << "\n";

    FILE.close();
}
template <typename Graph>
int find_random_start(Graph &graph)
{
    while (1)
    {
        node random_start = graph.get_random_node();
        std::cout << random_start.id << std ::endl;
        std::cout << "random start vertex is " << random_start.id << std::endl;
        if (graph.get_out_degree(random_start.id) != 0)
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

    if (bfs_cli.get_graph_type() == "std")
    {
        auto start = chrono::steady_clock::now();
        StandardGraph graph(opts);
        auto end = chrono::steady_clock::now();
        std::cout
            << "Graph loaded in "
            << chrono::duration_cast<chrono::microseconds>(end - start).count()
            << std::endl;

        // do 10 runs with random starting nodes and run 10 trial per each
        // random node
        for (int i = 0; i < bfs_cli.get_num_trials(); i++)
        {
            int start_vertex = bfs_cli.start_vertex();
            if (start_vertex == -1)
            {
                start_vertex = find_random_start(graph);
            }
            start = chrono::steady_clock::now();
            bfs_info *bfs_run = bfs(graph, start_vertex);
            end = chrono::steady_clock::now();
            int time_from_outside =
                chrono::duration_cast<chrono::microseconds>(end - start)
                    .count();
            std::cout << "BFS  completed in : " << time_from_outside
                      << std::endl;
            print_csv_info(opts.db_name,
                           start_vertex,
                           bfs_run,
                           time_from_outside,
                           bfs_log);
        }
        graph.close();
    }

    if (bfs_cli.get_graph_type() == "adj")
    {
        auto start = chrono::steady_clock::now();
        AdjList graph(opts);
        auto end = chrono::steady_clock::now();
        std::cout
            << "Graph loaded in "
            << chrono::duration_cast<chrono::microseconds>(end - start).count()
            << std::endl;

        // do 10 runs with random starting nodes and run 10 trial per each
        // random node

        for (int i = 0; i < bfs_cli.get_num_trials(); i++)
        {
            int start_vertex = bfs_cli.start_vertex();
            if (start_vertex == -1)
            {
                start_vertex = find_random_start(graph);
            }
            start = chrono::steady_clock::now();
            bfs_info *bfs_run = bfs(graph, start_vertex);
            end = chrono::steady_clock::now();
            int time_from_outside =
                chrono::duration_cast<chrono::microseconds>(end - start)
                    .count();
            std::cout << "BFS  completed in : " << time_from_outside
                      << std::endl;
            print_csv_info(opts.db_name,
                           start_vertex,
                           bfs_run,
                           time_from_outside,
                           bfs_log);
        }
        graph.close();
    }

    if (bfs_cli.get_graph_type() == "ekey")
    {
        auto start = chrono::steady_clock::now();
        EdgeKey graph(opts);
        auto end = chrono::steady_clock::now();
        std::cout
            << "Graph loaded in "
            << chrono::duration_cast<chrono::microseconds>(end - start).count()
            << std::endl;

        // do 10 runs with random starting nodes and run 10 trial per each
        // random node
        /*
        FOR SOME REASON SOME VERTICES IN THE EKEY SETUP NEVER TERMINATE. THEY
        ARE THE ONES COMMENTED OUT FROM THE VECTOR BELOW. THIS VECTOR WAS
        GENERATED BY RUNNING THE find_Random_start method get_num_trial() times.
         std::vector<int> starts = {4027332, 2282634, 4161954, 5615385, 4276469,
        4339986, 3865592, 4788124, 4639484, 4841846, 4169896, 4245846, 4608514,
        5717290, 3916985}; // 5052634, , // 5848383, for (int start_vertex :
        starts)

        */
        int num_trials = bfs_cli.get_num_trials();
#ifdef STAT
        num_trials = 1;  // We want only one run with stats collection
#endif
        for (int i = 0; i < num_trials; i++)
        {
            int start_vertex = bfs_cli.start_vertex();
            if (start_vertex == -1)
            {
                start_vertex = find_random_start(graph);
            }
            // std::cout << "start vertex is : " << start_vertex << endl;
            start = chrono::steady_clock::now();
            bfs_info *bfs_run = bfs(graph, start_vertex);
            end = chrono::steady_clock::now();
            int time_from_outside =
                chrono::duration_cast<chrono::microseconds>(end - start)
                    .count();
            std::cout << "BFS  completed in : " << time_from_outside
                      << std::endl;
            print_csv_info(opts.db_name,
                           start_vertex,
                           bfs_run,
                           time_from_outside,
                           bfs_log);
        }
        graph.close();
    }
}
