#include <unistd.h>

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "adj_list.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_exception.h"
#include "standard_graph.h"

#define delim "--------------"
#define INFO() fprintf(stderr, "%s\nNow running: %s\n", delim, __FUNCTION__);
#define quote(x) #x

std::unordered_map<int, std::vector<int>> in_adjlist;
std::unordered_map<int, std::vector<int>> out_adjlist;
std::set<int> nodes;

typedef struct insert_time
{
    int64_t std_insert;
    int64_t std_index;
    int64_t adj_insert;
    int64_t ekey_insert;
    int64_t ekey_index;
    insert_time(int _val)
        : std_insert(_val),
          std_index(_val),
          adj_insert(_val),
          ekey_index(_val),
          ekey_insert(_val){};
} insert_time;

void print_csv_info(std::string name, insert_time *info)
{
    fstream FILE;
    std::string _name =
        "/home/puneet/scratch/margraphita/outputs/"
        "single_threaded_kron_inserts_cpp.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, ios::out | ios::app);
        FILE << "#name, std_insert, std_index, adj_insert, ekey_insert, "
                "ekey_index\n";
    }
    else
    {
        FILE.open(_name, ios::out | ios::app);
    }

    FILE << name << "," << info->std_insert << "," << info->std_index << ","
         << info->adj_insert << "," << info->ekey_insert << ","
         << info->ekey_index << "\n";

    FILE.close();
}

void get_edge_entries(std::vector<edge> &edges,
                      std::string filename,
                      bool is_adj)
{
    std::ifstream newfile(filename);

    if (newfile.is_open())
    {
        std::string tp;
        while (getline(newfile, tp))
        {
            if ((tp.compare(0, 1, "#") == 0) or (tp.compare(0, 1, "%") == 0) or
                (tp.empty()))
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
                nodes.insert(a);
                nodes.insert(b);
                if (is_adj)
                {
                    in_adjlist[b].push_back(a);   // insert a in b's in_adjlist
                    out_adjlist[a].push_back(b);  // insert b in a's out_adjlist
                }

                edges.push_back(to_insert);
            }
        }
        newfile.close();
    }
    else
    {
        std::cout << "**could not open " << filename << std::endl;
    }
}

int64_t insert_adj(AdjList graph, std::string filename)
{
    auto start = std::chrono::steady_clock::now();
    std::vector<edge> edjlist;
    get_edge_entries(edjlist, filename, true);

    for (int node : nodes)
    {
        std::vector<int> innodes, outnodes;
        try
        {
            innodes = in_adjlist.at(node);
        }
        catch (std::exception &e)
        {
        }

        try
        {
            outnodes = out_adjlist.at(node);
        }
        catch (std::exception &e)
        {
        }
        graph.add_node(node, innodes, outnodes);
    }

    for (edge x : edjlist)
    {
        graph.add_edge(x, true);
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "read+insert in : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << "\n";
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start)
        .count();
}

template <typename Graph>
int64_t create_init_nodes(Graph graph, std::string filename)
{
    auto start = std::chrono::steady_clock::now();
    std::vector<edge> edjlist;
    get_edge_entries(edjlist, filename, false);

    for (int x : nodes)
    {
        std::vector<int> indeg, outdeg;
        node to_insert = {.id = x};
        try
        {
            to_insert.in_degree = in_adjlist.at(x).size();
        }
        catch (std::exception &e)
        {
        }

        try
        {
            to_insert.out_degree = out_adjlist.at(x).size();
        }
        catch (std::exception &e)
        {
        }

        graph.add_node(to_insert);
    }

    for (edge x : edjlist)
    {
        graph.add_edge(x, true);
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "read+insert in : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << "\n";
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start)
        .count();
}

template <typename Graph>
void tearDown(Graph graph)
{
    graph.close();
}

int main(int argc, char **argv)
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = true;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = false;
    opts.db_dir = "./db";
    // int i = atoi(argv[1]); // to accept i from script
    for (int i = 10; i < 21; i++)
    {
        insert_time *info = new insert_time(0);
        std::string db_name = "test_s" + std::to_string(i) + "_e8";
        // std::string filename = "/home/puneet/gen_graphs/s" +
        // std::to_string(i) + "_e8" + "/graph_s" + std::to_string(i) + "_e8";

        std::string filename =
            "/home/puneet/gen_graphs/graph_s" + std::to_string(i) + "_e8";

        opts.db_name = db_name + "_std";
        opts.create_new = true;
        StandardGraph graph1 = StandardGraph(opts);
        info->std_insert = create_init_nodes(graph1, filename);
        auto start = std::chrono::steady_clock::now();
        graph1.create_indices();
        auto end = std::chrono::steady_clock::now();
        info->std_index =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                .count();
        tearDown(graph1);
        print_csv_info("s" + std::to_string(i), info);

        opts.db_name = db_name + "_adj";
        AdjList graph2 = AdjList(opts);
        info->adj_insert = insert_adj(graph2, filename);
        tearDown(graph2);
        print_csv_info("s" + std::to_string(i), info);

        opts.db_name = db_name + "_ekey";
        EdgeKey graph3 = EdgeKey(opts);
        info->ekey_insert = create_init_nodes(graph3, filename);
        tearDown(graph3);
        // opts.create_new = false;
        // EdgeKey _graph3 = EdgeKey(opts);
        // start = std::chrono::steady_clock::now();
        // graph3.create_indices();
        // end = std::chrono::steady_clock::now();
        // info->ekey_index =
        // std::chrono::duration_cast<std::chrono::microseconds>(end -
        // start).count();

        // tearDown(_graph3);
        print_csv_info("s" + std::to_string(i), info);
    }
}
