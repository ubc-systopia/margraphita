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
#include "benchmark_definitions.h"
#include "bitmap.h"
#include "command_line.h"
#include "common.h"
#include "csv_log.h"
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

// eventually swap this find_random_start with any node from the largest
// connected component.
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
            pvector<node_id_t> &depth,
            int thread_num)
{
    bool done = false;
    node_id_t iter = 0;
    while (!done)
    {
        done = true;
#pragma omp parallel for reduction(& : done) num_threads(thread_num)
        for (int i = 0; i < thread_num; i++)
        {
            GraphBase *graph = graph_engine->create_graph_handle();
            edge found = {0};

            EdgeCursor *edge_cursor = graph->get_edge_iter();
            edge_cursor->set_key_range(graph_engine->get_edge_range(i));
            edge_cursor->next(&found);

            while (found.src_id != -1)
            {
                // we need to check this edge on the frontier
                node_id_t old_val = -1;
                if (parent[found.dst_id] == -1)
                {
                    if (depth[found.src_id] == iter)
                    {
                        done = false;
                        compare_and_swap(
                            parent[found.dst_id], old_val, found.src_id);
                        compare_and_swap(depth[found.dst_id], -1, iter + 1);
                    }
                }
                edge_cursor->next(&found);
            }

            graph->close();
        }
        iter++;
    }
}

// BFS verifier does a serial BFS from same source and asserts:
// - parent[source] = source
// - parent[v] = u  =>  depth[v] = depth[u] + 1 (except for source)
// - parent[v] = u  => there is edge from u to v
// - all vertices reachable from source have a parent
bool BFSVerifier(GraphEngine *graph_engine,
                 node_id_t source,  // i removed 'const' graph_engine
                 const pvector<node_id_t> &parent)
{
    GraphBase *g = graph_engine->create_graph_handle();

    pvector<int> depth(g->get_num_nodes(), -1);
    depth[source] = 0;
    vector<node_id_t> to_visit;
    to_visit.reserve(g->get_num_nodes());
    to_visit.push_back(source);
    for (auto it = to_visit.begin(); it != to_visit.end(); it++)
    {
        node_id_t u = *it;
        for (node_id_t v : g->get_out_nodes_id(u))
        {
            if (depth[v] == -1)
            {
                depth[v] = depth[u] + 1;
                to_visit.push_back(v);
            }
        }
    }
    for (node_id_t u = 0; u < (node_id_t)g->get_num_nodes(); ++u)
    {  // is this cast ok?
        if ((depth[u] != -1) && (parent[u] != -1))
        {
            if (u == source)
            {
                if (!((parent[u] == u) && (depth[u] == 0)))
                {
                    cout << "Source wrong" << endl;
                    return false;
                }
                continue;
            }
            bool parent_found = false;
            for (node_id_t v : g->get_in_nodes_id(u))
            {
                if (v == parent[u])
                {
                    if (depth[v] != depth[u] - 1)
                    {
                        cout << "Wrong depths for " << u << " & " << v << endl;
                        return false;
                    }
                    parent_found = true;
                    break;
                }
            }
            if (!parent_found)
            {
                cout << "Couldn't find edge from " << parent[u] << " to " << u
                     << endl;
                return false;
            }
        }
        else if (depth[u] != parent[u])
        {
            cout << "Reachability mismatch" << endl;
            return false;
        }

        g->close();
    }
    return true;
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
    node_id_t start_vertex = bfs_cli.start_vertex();

    const int THREAD_NUM = 16;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    bfs_info info(0);
    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    graphEngine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    t.start();
    GraphBase *graph = graphEngine.create_graph_handle();
    pvector<node_id_t> parent(graph->get_num_nodes(), -1);
    pvector<node_id_t> depth(graph->get_num_nodes(), -1);
    graph->close();

    node_id_t start = start_vertex;
    parent[start] = start;  // init parent...
    depth[start] = 0;

    BFS_EC(&graphEngine, parent, depth, THREAD_NUM);
    t.stop();

    // assert(BFSVerifier(&graph_engine, start, parent));  // test for
    // correctness
    info.time_taken = t.t_micros();
    std::cout << "BFS finished in " << t.t_micros() << std::endl;

    print_csv_info(opts.db_name, start_vertex, info, start_vertex, bfs_log);

    return 0;
}