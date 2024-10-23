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
#include "omp.h"
#include "times.h"
/**
 * This runs the Triangle Counting on the graph -- both Trust and Cycle counts
 */

const int THREAD_NUM = omp_get_max_threads();

bool id_compare(node_id_t a, node_id_t b) { return (a < b); }

std::vector<node_id_t> intersection_id(std::vector<node_id_t> &A,
                                       std::vector<node_id_t> &B)
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

int64_t trust_tc_iter(GraphEngine &graph_engine)
{
    int64_t count = 0;
#pragma omp parallel for reduction(+ : count)
    for (int i = 0; i < THREAD_NUM; i++)
    {
        GraphBase *graph = graph_engine.create_graph_handle();
        OutCursor *out_cursor = graph->get_outnbd_iter();
        out_cursor->set_key_range(graph_engine.get_key_range(i));
        adjlist found;

        out_cursor->next(&found);
        while (found.node_id != OutOfBand_ID_MAX)
        {
            for (auto node : found.edgelist)
            {
                std::vector<node_id_t> node_out_nbrhood =
                    graph->get_out_nodes_id(node);
                if (node_out_nbrhood.empty())
                {
                    continue;
                }
                std::vector<node_id_t> intersect =
                    intersection_id(found.edgelist, node_out_nbrhood);
                count += (int64_t)(intersect.size());
            }

            out_cursor->next(&found);
        }
        out_cursor->close();
        graph->close(false);
        delete out_cursor;
        delete graph;
    }

    return count;
}

int64_t cycle_tc_iter(GraphEngine &graph_engine)
{
    int64_t count = 0;
#pragma omp parallel for reduction(+ : count)
    for (int i = 0; i < THREAD_NUM; i++)
    {
        GraphBase *graph = graph_engine.create_graph_handle();
        OutCursor *out_cursor = graph->get_outnbd_iter();
        out_cursor->set_key_range(graph_engine.get_key_range(i));
        adjlist found;
        out_cursor->next(&found);
        while (found.node_id != OutOfBand_ID_MAX)
        {
            if (found.edgelist.empty())
            {
                out_cursor->next(&found);
                continue;
            }
            for (auto v : found.edgelist)
            {
                if (found.node_id < v)
                {
                    std::vector<node_id_t> v_out_nbrhood =
                        graph->get_out_nodes_id(v);
                    if (v_out_nbrhood.empty()) continue;

                    std::vector<node_id_t> u_in_nbrhood =
                        graph->get_in_nodes_id(found.node_id);
                    if (u_in_nbrhood.empty()) continue;
                    std::vector<node_id_t> intersect =
                        intersection_id(v_out_nbrhood, u_in_nbrhood);
                    for (auto w : intersect)
                    {
                        if (found.node_id < w)
                        {
                            count += 1;
                        }
                    }
                }
            }
            found.clear();
            out_cursor->next(&found);
        }
        out_cursor->close();
        graph->close(false);
        delete out_cursor;
        delete graph;
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

    std::cout << "THREAD_NUM: " << THREAD_NUM << std::endl;

    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    graphEngine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;
    long double total_time_cycle = 0, total_time_trust = 0;
    for (int i = 0; i < opts.num_trials; i++)
    {
        tc_info info(0);
        // Count Trust Triangles
        t.start();
        info.trust_count = trust_tc_iter(graphEngine);
        t.stop();

        info.trust_time = t.t_secs();
        total_time_trust += info.trust_time;
        std::cout << "Trust Triangle_Counting_ITER completed in : "
                  << info.trust_time << std::endl;
        std::cout << "Trust Triangles count = " << info.trust_count
                  << std::endl;

        // Count Cycle Triangles
        t.start();
        info.cycle_count = cycle_tc_iter(graphEngine);
        t.stop();
        info.cycle_time = t.t_secs();
        total_time_cycle += info.cycle_time;
        std::cout << "Cycle TriangleCounting_ITER completed in : "
                  << info.cycle_time << std::endl;
        std::cout << "Cycle Triangles count = " << info.cycle_count
                  << std::endl;

        print_csv_info(opts.db_name, info, opts.stat_log);
    }
    std::cout << "Average time Trust: " << total_time_trust / opts.num_trials
              << std::endl;
    std::cout << "Average time Cycle: " << total_time_cycle / opts.num_trials
              << std::endl;
    graphEngine.close_graph();
}