#include <stdio.h>
#include <vector>
#include <math.h>
#include <set>
#include <deque>
#include <algorithm>
#include <cassert>
#include <unistd.h>
#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "adj_list.h"
#include "edgekey.h"
#include "command_line.h"
#include <iostream>
#include <fstream>
/**
 * This runs the Triangle Counting on the graph -- both Trust and Cycle counts
 */

typedef struct tc_info
{
    int cycle_count;
    int trust_count;
    int64_t trust_time;
    int64_t cycle_time;
    tc_info(int _val) : cycle_count(_val), trust_count(_val), trust_time(_val), cycle_time(_val){};
} tc_info;

void print_csv_info(std::string name, tc_info &info)
{
    fstream FILE;
    std::string _name = "/home/puneet/scratch/margraphita/outputs/" + name + "_tc.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        //The file does not exist yet.
        FILE.open(_name, ios::out | ios::app);
        FILE << "#db_name, benchmark, cycle_count, cycle_time_usec, trust_count, trust_time_usecs\n";
    }
    else
    {
        FILE.open(_name, ios::out | ios::app);
    }

    FILE << name << ",tc,"
         << info.cycle_count << ","
         << info.cycle_time << ","
         << info.trust_count << ","
         << info.trust_time << "\n";

    FILE.close();
}

bool node_compare(node a, node b)
{
    return ((a.id < b.id));
}

std::vector<node> intersection(std::vector<node> A, std::vector<node> B)
{
    int a = A.size();
    int b = B.size();
    std::sort(A.begin(), A.end(), node_compare);
    std::sort(B.begin(), B.end(), node_compare);
    //std::set<int> ABintersection;
    std::vector<node> ABintersection;
    // std::set_intersect(A.begin(),
    //                    A.end(),
    //                    B.begin(),
    //                    B.end(),
    //                    std::inserter(ABintersection, ABintersection.begin()));
    std::vector<node>::iterator A_iter = A.begin();
    std::vector<node>::iterator B_iter = B.begin();
    int i = 0;
    int j = 0;
    int k = 0;
    while (i < a and j < b)
    {
        if ((*A_iter).id == (*B_iter).id)
        {
            ABintersection[k] = *A_iter;
            k++;
            ++A_iter;
            ++B_iter;
        }
        else if ((*A_iter).id < (*B_iter).id)
        {
            ++A_iter;
        }
        else
        {
            ++B_iter;
        }
    }
    assert(ABintersection.size() == k);
    return ABintersection;
}

template <typename Graph>
int trust_tc(Graph &graph)
{
    int count = 0;
    vector<node> nodes = graph.get_nodes();
    for (node n : nodes)
    {
        vector<edge> out_edges = graph.get_out_edges(n.id);

        for (edge e : out_edges)
        {
            vector<edge> edges2;
            int nbr_id = e.dst_id;
            if (nbr_id == n.id)
            {
                continue; //self loop -- ignore.
            }

            // if (isTrust) // or e.dst_id > n.id)
            // {
            edges2 = graph.get_out_edges(nbr_id);
            // }

            // if (isTrust)
            // {
            set<int> edge_nei;
            set<int> edge2_nei;

            for (edge e : out_edges)
            {

                if (e.src_id != e.dst_id)
                {
                    edge_nei.insert(e.dst_id);
                }
            }

            for (edge e : edges2)
            {
                if (e.src_id != e.dst_id && edge_nei.find(e.dst_id) != edge_nei.end())
                {
                    count++;
                }
            }
            //}
        }
    }

    return count;
}

template <typename Graph>
int cycle_tc(Graph &graph)
{
    int count = 0;
    vector<node> nodes = graph.get_nodes();
    for (node n : nodes)
    {
        vector<node> out_nodes = graph.get_out_nodes(n.id);
        for (node out_node : out_nodes)
        {
            if (n.id < out_node.id)
            {
                std::vector<node> intersect = (graph.get_in_nodes(n.id), graph.get_out_nodes(out_node.id));
                for (node w : intersect)
                {
                    if (n.id < w.id and out_node.id < w.id)
                    {
                        //cout << "(" << n.id << "," << out_node.id << "," << w.id << ")\n";
                        count++;
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

    graph_opts opts;
    opts.create_new = tc_cli.is_create_new();
    opts.is_directed = tc_cli.is_directed();
    opts.read_optimize = tc_cli.is_read_optimize();
    opts.is_weighted = tc_cli.is_weighted();
    opts.optimize_create = tc_cli.is_create_optimized();
    opts.db_name = tc_cli.get_db_name();
    opts.db_dir = tc_cli.get_db_path();

    int num_trials = 1;

    if (tc_cli.get_graph_type() == "std")
    {
        auto start = chrono::steady_clock::now();
        StandardGraph graph(opts);
        auto end = chrono::steady_clock::now();
        std::cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << std::endl;

        for (int i = 0; i < num_trials; i++)
        {
            tc_info info(0);
            //Count Trust Triangles
            start = chrono::steady_clock::now();
            info.trust_count = trust_tc(graph);
            end = chrono::steady_clock::now();

            info.trust_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
            std::cout << "Trust TriangleCounting completed in : " << info.trust_time << std::endl;
            std::cout << "Trust Triangles count = " << info.trust_count << std::endl;

            //Count Cycle Triangles
            start = chrono::steady_clock::now();
            info.cycle_count = cycle_tc(graph);
            end = chrono::steady_clock::now();
            info.cycle_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
            std::cout << "Cycle TriangleCounting  completed in : " << info.cycle_time << std::endl;
            std::cout << "Cycle Triangles count = " << info.cycle_count << std::endl;

            print_csv_info(opts.db_name, info);
        }
    }
    if (tc_cli.get_graph_type() == "adj")
    {
        auto start = chrono::steady_clock::now();
        AdjList graph(opts);
        auto end = chrono::steady_clock::now();
        std::cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << std::endl;

        for (int i = 0; i < num_trials; i++)
        {
            tc_info info(0);
            //Count Trust Triangles
            start = chrono::steady_clock::now();
            info.trust_count = trust_tc(graph);
            end = chrono::steady_clock::now();
            info.trust_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
            std::cout << "Trust TriangleCounting completed in : " << info.trust_time << std::endl;
            std::cout << "Trust Triangles count = " << info.trust_count << std::endl;

            //Count Cycle Triangles
            start = chrono::steady_clock::now();
            info.cycle_count = cycle_tc(graph);
            end = chrono::steady_clock::now();
            info.cycle_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
            std::cout << "Cycle Triangle Counting  completed in : " << info.cycle_time << std::endl;
            std::cout << "Cycle Triangles count = " << info.cycle_count << std::endl;
            print_csv_info(opts.db_name, info);
        }
    }

    if (tc_cli.get_graph_type() == "ekey")
    {
        auto start = chrono::steady_clock::now();
        EdgeKey graph(opts);
        auto end = chrono::steady_clock::now();
        std::cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << std::endl;

        for (int i = 0; i < num_trials; i++)
        {
            tc_info info(0);
            //Count Trust Triangles
            start = chrono::steady_clock::now();
            info.trust_count = trust_tc(graph);
            end = chrono::steady_clock::now();
            info.trust_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
            std::cout << "Trust TriangleCounting completed in : " << info.trust_time << std::endl;
            std::cout << "Trust Triangles count = " << info.trust_count << std::endl;

            //Count Cycle Triangles
            start = chrono::steady_clock::now();
            info.cycle_count = cycle_tc(graph);
            end = chrono::steady_clock::now();
            info.cycle_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
            std::cout << "Cycle TriangleCounting  completed in : " << info.cycle_time << std::endl;
            std::cout << "Cycle Triangles count = " << info.cycle_count << std::endl;
            print_csv_info(opts.db_name, info);
        }
    }
}
