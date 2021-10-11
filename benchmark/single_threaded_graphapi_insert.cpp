#include <vector>
#include <string>
#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "graph_exception.h"
#include "standard_graph.h"
#include "adj_list.h"
#include "edgekey.h"
#include "command_line.h"
#include <cassert>
#include <chrono>

#define delim "--------------"
#define INFO() \
    fprintf(stderr, "%s\nNow running: %s\n", delim, __FUNCTION__);
#define quote(x) #x

std::vector<edge> get_edge_entries(std::vector<edge> &edges, std::string filename)
{
    std::ifstream newfile(filename);

    if (newfile.is_open())
    {
        std::string tp;
        while (getline(newfile, tp))
        {
            if ((tp.compare(0, 1, "#") == 0) or (tp.compare(0, 1, "%") == 0) or (tp.empty()))
            {
                continue;
            }

            else
            {
                int a, b;
                std::stringstream s_stream(tp);
                s_stream >> a;
                s_stream >> b;
                edge to_insert;
                to_insert.src_id = a;
                to_insert.dst_id = b;

                edges.push_back(to_insert);
            }
        }
        newfile.close();
    }
    else
    {
        std::cout << "**could not open " << filename << std::endl;
    }
    return edges;
}

template <typename Graph>
void create_init_nodes(Graph graph)
{
    auto start = std::chrono::steady_clock::now();
    std::vector<edge> edjlist;
    get_edge_entries(edjlist, "/home/puneet/graph_s15_e8");

    auto end = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << ",";

    start = std::chrono::steady_clock::now();
    for (edge x : edjlist)
    {
        graph.add_edge(x);
    }
    end = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << ",";
}

template <typename Graph>
void tearDown(Graph graph)
{
    graph.close();
}

int main()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = true;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = false;
    opts.db_name = "test_s15";
    opts.db_dir = "./db";

    std::cout << "Inserting STD \n";
    for (int i = 0; i < 10; i++)
    {
        opts.db_name = "test_s15_std";
        StandardGraph graph1 = StandardGraph(opts);
        create_init_nodes(graph1);
        auto start = std::chrono::steady_clock::now();
        graph1.create_indices();
        auto end = std::chrono::steady_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
        tearDown(graph1);
    }

    std::cout << "Inserting ADJ \n";
    for (int i = 0; i < 10; i++)
    {
        opts.db_name = "test_s15_adj";
        AdjList graph2 = AdjList(opts);
        create_init_nodes(graph2);
        std::cout << "NA" << std::endl;
        tearDown(graph2);
    }

    std::cout << "Inserting EKEY \n";
    for (int i = 0; i < 10; i++)
    {
        opts.db_name = "test_s15_ekey";
        EdgeKey graph3 = EdgeKey(opts);
        create_init_nodes(graph3);
        auto start = std::chrono::steady_clock::now();
        tearDown(graph3);
    }
}
