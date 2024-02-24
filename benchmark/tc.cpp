
#include <deque>
#include <iostream>
#include <set>
#include <vector>
#include "benchmark_definitions.h"
#include "command_line.h"
#include "csv_log.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "times.h"

/**
 * This runs the Triangle Counting on the graph -- both Trust and Cycle counts
 */

bool id_compare(node_id_t a, node_id_t b) { return (a < b); }

std::vector<node_id_t> intersection_id(std::vector<node_id_t> A,
                                       std::vector<node_id_t> B)
{
    std::sort(A.begin(), A.end(), id_compare);
    std::sort(B.begin(), B.end(), id_compare);
    std::vector<node_id_t> ABintersection;
    auto A_iter = A.begin();
    auto B_iter = B.begin();

    while (A_iter != std::end(A) && B_iter != std::end(B))
    {
        if (*A_iter == *B_iter)
        {
            ABintersection.push_back(*A_iter);
            ++A_iter;
            ++B_iter;
        }
        else if (*A_iter < *B_iter)
        {
            ++A_iter;
        }
        else
        {
            ++B_iter;
        }
    }
    return ABintersection;
}

template <typename Graph>
size_t trust_tc(Graph &graph)
{
    size_t count = 0;
    vector<node> nodes = graph->get_nodes();
    for (node u : nodes)
    {
        vector<node_id_t> u_out_ids = graph->get_out_nodes_id(u.id);
        for (node_id_t v : u_out_ids)
        {
            vector<node_id_t> v_out_ids = graph->get_out_nodes_id(v);
            std::vector<node_id_t> intersect =
                intersection_id(u_out_ids, v_out_ids);
            count += intersect.size();
        }
    }

    return count;
}

template <typename Graph>
int64_t cycle_tc(Graph &graph)
{
    int64_t count = 0;
    vector<node> nodes = graph->get_nodes();
    for (node u : nodes)
    {
        vector<node_id_t> u_out_ids = graph->get_out_nodes_id(u.id);
        for (node_id_t v : u_out_ids)
        {
            if (u.id < v)
            {
                vector<node_id_t> v_out_ids = graph->get_out_nodes_id(v);
                vector<node_id_t> u_in_ids = graph->get_in_nodes_id(u.id);
                std::vector<node_id_t> intersect =
                    intersection_id(v_out_ids, u_in_ids);
                for (node_id_t w : intersect)
                {
                    if (u.id < w)
                    {
                        count += 1;
                    }
                }
            }
        }
    }
    return count;
}

int main(int argc, char *argv[])
{
    std::cout << "Running TC" << std::endl;
    CmdLineApp tc_cli(argc, argv);
    if (!tc_cli.parse_args())
    {
        return -1;
    }

    cmdline_opts opts = tc_cli.get_parsed_opts();
    opts.stat_log += "/" + opts.db_name;

    const int THREAD_NUM = 1;

    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    GraphBase *graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < opts.num_trials; i++)
    {
        tc_info info;
        // Count Trust Triangles
        t.start();
        info.trust_count = trust_tc(graph);
        t.stop();

        info.trust_time = t.t_micros();
        std::cout << "Trust TriangleCounting completed in : " << info.trust_time
                  << std::endl;
        std::cout << "Trust Triangles count = " << info.trust_count
                  << std::endl;

        // Count Cycle Triangles
        t.start();
        info.cycle_count = cycle_tc(graph);
        t.stop();
        info.cycle_time = t.t_micros();
        std::cout << "Cycle TriangleCounting  completed in : "
                  << info.cycle_time << std::endl;
        std::cout << "Cycle Triangles count = " << info.cycle_count
                  << std::endl;

        print_csv_info(opts.db_name, info, opts.stat_log);
    }
    graph->close();
    graphEngine.close_graph();
}