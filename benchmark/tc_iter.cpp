#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>

#include "adj_list.h"
#include "benchmark_definitions.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"
/**
 * This runs the Triangle Counting on the graph -- both Trust and Cycle counts
 */

void print_csv_info(std::string name, tc_info &info, std::string csv_logdir)
{
    fstream FILE;
    std::string _name = csv_logdir + "/" + name + "_tc.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, ios::out | ios::app);
        FILE << "#db_name, benchmark, cycle_count, cycle_time_usec, "
                "trust_count, trust_time_usecs\n";
    }
    else
    {
        FILE.open(_name, ios::out | ios::app);
    }

    FILE << name << ",tc," << info.cycle_count << "," << info.cycle_time << ","
         << info.trust_count << "," << info.trust_time << "\n";

    FILE.close();
}

bool id_compare(node_id_t a, node_id_t b) { return (a < b); }

std::vector<node_id_t> intersection_id(std::vector<node_id_t> A,
                                       std::vector<node_id_t> B)
{
    std::sort(A.begin(), A.end(), id_compare);
    std::sort(B.begin(), B.end(), id_compare);
    std::vector<node_id_t> ABintersection;
    std::vector<node_id_t>::iterator A_iter = A.begin();
    std::vector<node_id_t>::iterator B_iter = B.begin();

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
int64_t trust_tc_iter(Graph &graph)
{
    int64_t count = 0;
    OutCursor *out_cursor = graph->get_outnbd_iter();
    adjlist found = {0};

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
    adjlist found = {0};
    adjlist found_out = {0};

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

    graph_opts opts;
    get_graph_opts(tc_cli, opts);
    opts.stat_log = tc_cli.get_logdir() + "/" + opts.db_name;

    const int THREAD_NUM = 1;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    int num_trials = tc_cli.get_num_trials();

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    GraphBase *graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < num_trials; i++)
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

        print_csv_info(opts.db_name, info, tc_cli.get_logdir());
    }
    graph->close();
    graphEngine.close_graph();
}