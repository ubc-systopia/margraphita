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

#include "GraphCreate.h"
#include "adj_list.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"

/**
 * This runs the Triangle Counting on the graph -- both Trust and Cycle counts
 */

typedef struct tc_info
{
    int64_t cycle_count;
    int64_t trust_count;
    double trust_time;
    double cycle_time;
    tc_info(int _val)
        : cycle_count(_val),
          trust_count(_val),
          trust_time(_val),
          cycle_time(_val){};
} tc_info;

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

std::vector<node> intersection_id(std::vector<node_id_t> A,
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
int64_t trust_tc(Graph &graph)
{
    int64_t count = 0;
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
int64_t trust_tc_iter(Graph &graph)
{
    int64_t count = 0;
    OutCursor out_cursor = graph.get_outnbd_iter();
    adjlist found = {0};

    out_cursor.next(&found);
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
        out_cursor.next(&found);
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

template <typename Graph>
int64_t cycle_tc_iter(Graph &graph)
{
    int64_t count = 0;
    InCursor in_cursor = graph.get_innbd_iter();
    OutCursor out_cursor = graph.get_outnbd_iter();
    adjlist found = {0};
    adjlist found_out = {0};

    in_cursor.next(&found);
    out_cursor.next(&found_out);

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
        in_cursor.next(&found);
        out_cursor.next(&found_out);
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
    opts.create_new = tc_cli.is_create_new();
    opts.is_directed = tc_cli.is_directed();
    opts.read_optimize = tc_cli.is_read_optimize();
    opts.is_weighted = tc_cli.is_weighted();
    opts.optimize_create = tc_cli.is_create_optimized();
    opts.db_name = tc_cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = tc_cli.get_db_path();
    std::string tc_log = tc_cli.get_logdir();  //$RESULT/$bmark
    opts.stat_log = tc_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";  // tc_cli.get_conn_config();
    opts.type = tc_cli.get_graph_type();

    int num_trials = tc_cli.get_num_trials();

    Times t;
    t.start();
    GraphFactory f;
    GraphBase *graph = f.CreateGraph(opts);
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < num_trials; i++)
    {
        tc_info info(0);
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

        print_csv_info(opts.db_name, info, tc_log);
    }
}