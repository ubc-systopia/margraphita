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
#include "bitmap.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "standard_graph.h"
#include "times.h"

/**
 * This runs the BFS on the graph using src as the starting node.
 */

using namespace std;

typedef struct bfs_info
{
    int bfs_tree_num_nodes;
    int bfs_tree_num_edges;
    int64_t time_taken;
    bfs_info(int _val)
        : bfs_tree_num_nodes(_val),
          bfs_tree_num_edges(_val),
          time_taken(_val){};
} bfs_info;

template <typename Graph>
int64_t TDStep(Graph &g, pvector<int> &parent, SlidingQueue<int> &queue)
{
    int64_t scout_count = 0;
    // #pragma omp parallel
    {
        QueueBuffer<int> lqueue(queue);
        // #pragma omp for reduction(+ : scout_count) nowait //Is this safe to
        // do? Must make sure that we don't introduce a data race.
        for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++)
        {
            int u = *q_iter;
            for (node v : g.get_out_nodes(u))  // use iter
            {
                int curr_val = parent[v.id];
                if (curr_val < 0)
                {
                    if (compare_and_swap(parent[v.id], curr_val, u))
                    {
                        lqueue.push_back(v.id);
                        scout_count += -curr_val;
                    }
                }
            }
        }
        lqueue.flush();
    }
    return scout_count;
}

template <typename Graph>
void PrintBFSStats(Graph &g, const pvector<int> &bfs_tree, bfs_info *info)
{
    int64_t tree_size = 0;
    int64_t n_edges = 0;
    for (node n : g.get_nodes())
    {
        if (bfs_tree[n.id] >= 0)
        {
            n_edges += g.get_out_degree(n.id);
            tree_size++;
        }
    }
    cout << "BFS Tree has " << tree_size << " nodes and ";
    cout << n_edges << " edges" << endl;
    info->bfs_tree_num_edges = n_edges;
    info->bfs_tree_num_nodes = tree_size;
}

template <typename Graph>
pvector<int> InitParent(Graph &g)
{
    int num_nodes = g.get_num_nodes();
    auto node_iter = g.get_node pvector<int> parent(num_nodes);
    //#pragma omp parallel for
    for (int n = 0; n < num_nodes; n++)
        parent[n] = g.get_out_degree(n) != 0 ? -g.get_out_degree(n) : -1;
    return parent;
}

template <typename Graph>
bfs_info *bfs(Graph &graph, int source)
{
    Times t;
    bfs_info *info = new bfs_info(0);
    pvector<int> parent = InitParent(graph);
    parent[source] = source;
    SlidingQueue<int> queue(graph.get_num_nodes());
    queue.push_back(source);
    queue.slide_window();
    Bitmap curr(graph.get_num_nodes());
    curr.reset();
    Bitmap front(graph.get_num_nodes());
    front.reset();
    int64_t edges_to_check = graph.get_num_edges();
    int64_t scout_count = graph.get_out_degree(source);  // iter?

    while (!queue.empty())
    {
        t.start();
        edges_to_check -= scout_count;
        scout_count = TDStep(graph, parent, queue);
        queue.slide_window();
        t.stop();
        printf("%5s%11" PRId64 "  %10.5lf\n",
               "top down step",
               t.t_secs(),
               queue.size());
    }

    for (int n = 0; n < graph.get_num_nodes(); n++)
    {
        if (parent[n] < -1)
        {
            parent[n] = -1;
        }
    }

    PrintBFSStats(graph, parent, info);
    info->time_taken = t.t_micros();
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
        FILE << "#db_name, benchmark, starting node_id, bfs_tree_nodes, "
                "bfs_tree_edges, total_time_taken_usecs, time_from_main\n";
    }
    else
    {
        FILE.open(_name, ios::out | ios::app);
    }

    FILE << name << ",bfs," << starting_node << "," << info->bfs_tree_num_nodes
         << "," << info->bfs_tree_num_edges << "," << info->time_taken << ","
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
        FOR SOME REASON SOME VERTICES IN THE EKEY SETUP NEVER TERMINATE.
        THEY ARE THE ONES COMMENTED OUT FROM THE VECTOR BELOW. THIS VECTOR
        WAS GENERATED BY RUNNING THE find_Random_start method
        get_num_trial() times. std::vector<int> starts = {4027332, 2282634,
        4161954, 5615385, 4276469, 4339986, 3865592, 4788124, 4639484,
        4841846, 4169896, 4245846, 4608514, 5717290, 3916985}; // 5052634, ,
        // 5848383, for (int start_vertex : starts)

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
