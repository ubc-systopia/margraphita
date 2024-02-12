#include <unistd.h>

#include <deque>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>

#include "benchmark_definitions.h"
#include "command_line.h"
#include "common_util.h"
#include "csv_log.h"
#include "graph_engine.h"
#include "standard_graph.h"
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
size_t trust_tc_iter(Graph &graph)
{
    size_t count = 0;
    OutCursor *out_cursor = graph->get_outnbd_iter();
    adjlist found;

    out_cursor->next(&found);
    while (found.node_id != -1)
    {
        std::vector<node_id_t> out_nbrhood = found.edgelist;
        for (node_id_t node : out_nbrhood)
        {
            std::vector<node_id_t> node_out_nbrhood =
                graph->get_out_nodes_id(node);
            std::vector<node_id_t> intersect =
                intersection_id(out_nbrhood, node_out_nbrhood);
            count += intersect.size();
        }
        out_cursor->next(&found);
    }

    return count;
}

template <typename Graph>
int64_t cycle_tc_iter(Graph &graph)
{
    int64_t count = 0;
    InCursor *in_cursor = graph->get_innbd_iter();
    OutCursor *out_cursor = graph->get_outnbd_iter();
    adjlist found;
    adjlist found_out;

    in_cursor->next(&found);
    out_cursor->next(&found_out);

    while (found.node_id != -1)
    {
        std::vector<node_id_t> in_nbrhood = found.edgelist;
        std::vector<node_id_t> out_nbrhood = found_out.edgelist;
        for (node_id_t node : out_nbrhood)
        {
            if (found.node_id < node)
            {
                std::vector<node_id_t> node_out_nbrhood =
                    graph->get_out_nodes_id(node);
                std::vector<node_id_t> intersect =
                    intersection_id(in_nbrhood, node_out_nbrhood);

                for (node_id_t itsc : intersect)
                {
                    if (found.node_id < itsc)
                    {
                        count += 1;
                    }
                }
            }
        }
        in_cursor->next(&found);
        out_cursor->next(&found_out);
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
        tc_info info(0);
        // Count Trust Triangles
        t.start();
        info.trust_count = trust_tc_iter(graph);
        t.stop();

        info.trust_time = t.t_micros();
        std::cout << "Trust Triangle_Counting_ITER completed in : "
                  << info.trust_time << std::endl;
        std::cout << "Trust Triangles count = " << info.trust_count
                  << std::endl;

        // Count Cycle Triangles
        t.start();
        info.cycle_count = cycle_tc_iter(graph);
        t.stop();
        info.cycle_time = t.t_micros();
        std::cout << "Cycle TriangleCounting_ITER completed in : "
                  << info.cycle_time << std::endl;
        std::cout << "Cycle Triangles count = " << info.cycle_count
                  << std::endl;

        print_csv_info(opts.db_name, info, opts.stat_log);
    }
    graph->close();
    graphEngine.close_graph();
}