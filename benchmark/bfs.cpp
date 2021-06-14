#include <stdio.h>
#include <vector>
#include <math.h>
#include <set>
#include <deque>
#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "command_line.h"
#include "logger.h"

/**
 * This runs the BFS on the graph using src as the starting node.  
 */

using namespace std;

vector<int> bfs(StandardGraph graph, int src)
{
    set<int> visited = {src};
    deque<int> queue = {src};
    vector<int> result;

    while (!queue.empty())
    {
        int node_id = queue.at(0);
        queue.pop_front();
        result.push_back(node_id);
        vector<node> out_nbrs = graph.get_out_nodes(node_id);
        for (node nbr : out_nbrs)
        {
            if (visited.find(nbr.id) == visited.end())
            {
                visited.insert(nbr.id);
                queue.push_back(nbr.id);
            }
        }
    }
    return result;
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

    if (bfs_cli.get_graph_type() == "std")
    {
        Logger *logger = new Logger("bfs_run", opts.db_name, "bfs", bfs_cli.get_dataset());
        auto start = chrono::steady_clock::now();
        StandardGraph graph(opts);
        auto end = chrono::steady_clock::now();
        logger->out("Graph loaded in " + chrono::duration_cast<chrono::microseconds>(end - start).count());

        for (int i = 0; i < bfs_cli.get_num_trials(); i++)
        {

            if (bfs_cli.start_vertex() == -1)
            {
                node random_start = graph.get_random_node();
                start = chrono::steady_clock::now();
                bfs(graph, random_start.id);
                end = chrono::steady_clock::now();
                logger->out("BFS  completed in : " + chrono::duration_cast<chrono::microseconds>(end - start).count());
            }
            else
            {
                start = chrono::steady_clock::now();
                bfs(graph, bfs_cli.start_vertex());
                end = chrono::steady_clock::now();
                logger->out("BFS  completed in : " + chrono::duration_cast<chrono::microseconds>(end - start).count());
            }
        }
    }
    else
    {
        cout << "unimplemented.";
        return -1;
    }
}