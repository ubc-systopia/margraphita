#include <stdio.h>
#include <vector>
#include <unistd.h>
#include <math.h>
#include <set>
#include <deque>
#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "adj_list.h"
#include "edgekey.h"
#include "command_line.h"
#include "logger.h"

/**
 * This runs the BFS on the graph using src as the starting node.  
 */

using namespace std;

typedef struct bfs_info
{
    int num_visited;
    int sum_out_deg;
    int64_t time_taken;
    bfs_info(int _val) : num_visited(_val), sum_out_deg(_val), time_taken(_val){};
} bfs_info;

template <typename Graph>
bfs_info *bfs(Graph &graph, int src)
{
    bfs_info *info = new bfs_info(0);

    for (int i = 0; i < 10; i++)
    {
        auto start = chrono::steady_clock::now();
        set<int> visited = {src};
        deque<int> queue = {src};
        vector<int> result;

        while (!queue.empty())
        {
            int node_id = queue.at(0);
            queue.pop_front();
            result.push_back(node_id);
            vector<node> out_nbrs = graph.get_out_nodes(node_id);
            info->sum_out_deg += out_nbrs.size();
            for (node nbr : out_nbrs)
            {
                //std::cout << "nbr is " << nbr.id << std::endl;
                if (visited.find(nbr.id) == visited.end())
                {
                    visited.insert(nbr.id);
                    queue.push_back(nbr.id);
                }
            }
        }
        auto end = chrono::steady_clock::now();
        info->time_taken += chrono::duration_cast<chrono::microseconds>(end - start).count();
        info->num_visited = visited.size();
    }
    //average the time taken to get average per iteration.
    info->time_taken = info->time_taken / 10;
    return info;
}

void print_csv_info(std::string name, int starting_node, bfs_info *info)
{
    fstream FILE;
    std::string _name = "/home/puneet/scratch/margraphita/outputs/" + name + "_bfs.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        //The file does not exist yet.
        FILE.open(_name, ios::out | ios::app);
        FILE << "#db_name, benchmark, starting node_id, num_visited, sum_out_deg, time_taken_usecs\n";
    }
    else
    {
        FILE.open(_name, ios::out | ios::app);
    }

    FILE << name << ",bfs,"
         << starting_node << ","
         << info->num_visited << ","
         << info->sum_out_deg << ","
         << info->time_taken << "\n";

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
            std::cout << "out degree was zero. finding a new random vertex." << std::endl;
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
    opts.db_name = bfs_cli.get_db_name();
    opts.db_dir = bfs_cli.get_db_path();

    if (bfs_cli.get_graph_type() == "std")
    {

        auto start = chrono::steady_clock::now();
        StandardGraph graph(opts);
        auto end = chrono::steady_clock::now();
        std::cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << std::endl;

        //do 10 runs with random starting nodes and run 10 trial per each random node
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
            std::cout << "BFS  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << std::endl;

            print_csv_info(opts.db_name, start_vertex, bfs_run);
        }
        graph.close();
    }

    if (bfs_cli.get_graph_type() == "adj")
    {

        auto start = chrono::steady_clock::now();

        AdjList graph(opts);
        auto end = chrono::steady_clock::now();
        std::cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << std::endl;

        //do 10 runs with random starting nodes and run 10 trial per each random node
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
            std::cout << "BFS  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << std::endl;

            print_csv_info(opts.db_name, start_vertex, bfs_run);
        }
        graph.close();
    }

    if (bfs_cli.get_graph_type() == "ekey")
    {

        auto start = chrono::steady_clock::now();

        EdgeKey graph(opts);
        auto end = chrono::steady_clock::now();
        std::cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << std::endl;

        //do 10 runs with random starting nodes and run 10 trial per each random node
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
            std::cout << "BFS  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << std::endl;

            print_csv_info(opts.db_name, start_vertex, bfs_run);
        }
        graph.close();
    }
}