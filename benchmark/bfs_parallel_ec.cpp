#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
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
#include "graph_engine.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "standard_graph.h"
#include "times.h"

// debug workflow:
// get to compile. look at cmakelists in benchmarks.

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

void BFS_EC(GraphEngine *graph_engine,
               pvector<node_id_t> &parent,
               int thread_num)
{
bool done = false;

GraphBase *graph = graph_engine->create_graph_handle();
parent[find_random_start(*graph)] = find_random_start(*graph);

while (!done) {

    done = true;
    #pragma omp parallel for reduction(& : done) num_threads(thread_num)
        for (int i = 0; i < thread_num; i++)
        {
            GraphBase *graph = graph_engine->create_graph_handle();
            edge found = {0};

            EdgeCursor* edge_cursor = graph->get_edge_iter();
            edge_cursor->set_key(graph_engine->get_edge_range(i));
            edge_cursor->next(&found);

            while (found.src_id != -1)
            {   
                // we need to check this edge on the frontier
                if (parent[found.src_id] != -1 && parent[found.dst_id] == -1)
                {   
                    done = false;
                    parent[found.dst_id] = found.src_id;
                }
                edge_cursor->next(&found);
            }

            graph->close();
        }
    }
}

int main(int argc, char *argv[])
{
    cout << "Running Edge-centric BFS" << endl;
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

    GraphEngine graphEngine(engine_opts);

    pvector<node_id_t> parent(n, -1); // TODO: set n = number of nodes

    BFS_EC(graphEngine, parent, THREAD_NUM)

    return 1
}
