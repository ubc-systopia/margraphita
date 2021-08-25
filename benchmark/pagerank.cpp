#include <stdio.h>
#include <vector>
#include <math.h>
#include <chrono>
#include <unistd.h>
#include <cassert>
#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "adj_list.h"
#include "edgekey.h"
#include "command_line.h"
#include "logger.h"
#include "reader.h"

/**
 * @brief This runs the PageRank kernel on the 
 * 
 */

using namespace std;
const float dampness = 0.85;

std::unordered_map<int, std::vector<float>> pr_map;

void init_pr_map(std::vector<node> nodes)
{
    vector<float> pr_vals = {(1 / nodes.size()), 0};
    for (node n : nodes)
    {
        pr_map.insert({n.id, pr_vals});
    }
}

template <typename Graph>
void pagerank(Graph &graph, graph_opts opts, int iterations, double tolerance, Logger *logger)
{

    int num_nodes = graph.get_num_nodes();
    std::vector<node> nodes = graph.get_nodes();
    init_pr_map(nodes);
    nodes.clear();                   //delete all elements
    std::vector<node>().swap(nodes); //free memory

    int p_curr = 0;
    int p_next = 1;
    WT_CURSOR *edge_iter = graph.get_edge_iter();
    WT_CURSOR *node_iter = graph.get_node_iter();
    for (int iter = 0; iter < iterations; iter++)
    {
        auto start = chrono::steady_clock::now();

        edge e_found;
        e_found = graph.get_next_edge(edge_iter);
        while (e_found.id > 0)
        {

            int src_out_deg = graph.get_out_degree(e_found.src_id);
            float rank = pr_map.at(e_found.dst_id).at(p_next) + (pr_map.at(e_found.dst_id).at(p_curr) / src_out_deg);
            pr_map.at(e_found.dst_id).at(p_next) = rank;

            e_found = graph.get_next_edge(edge_iter);
        }

        //Now apply Damping

        for (auto itr = pr_map.begin(); itr != pr_map.end(); ++itr)
        {
            float damped_rank = (dampness * ((itr->second).at(p_next))) + ((1 - dampness) / num_nodes);
            itr->second.at(p_next) = damped_rank;
            itr->second.at(p_curr) = 0;
        }

        p_curr = 1 - p_curr;
        p_next = 1 - p_next;

        auto end = chrono::steady_clock::now();
        cout << "Iter " << iter << "took \t" << to_string(chrono::duration_cast<chrono::microseconds>(end - start).count()) << endl;
        edge_iter->reset(edge_iter);
    }
    edge_iter->close(edge_iter);
    node_iter->close(node_iter);
}

int main(int argc, char *argv[])
{

    cout << "Running PageRank" << endl;
    PageRankOpts pr_cli(argc, argv, 1e-4, 10);
    if (!pr_cli.parse_args())
    {
        return -1;
    }

    graph_opts opts;
    opts.create_new = pr_cli.is_create_new();
    opts.is_directed = pr_cli.is_directed();
    opts.read_optimize = pr_cli.is_read_optimize();
    opts.is_weighted = pr_cli.is_weighted();
    opts.optimize_create = pr_cli.is_create_optimized();
    opts.db_name = pr_cli.get_db_name();

    if (pr_cli.get_graph_type() == "std")
    {

        Logger *logger = new Logger("pr_run", opts.db_name, "pagerank", pr_cli.get_dataset());

        auto start = chrono::steady_clock::now();

        StandardGraph graph(opts);
        if (pr_cli.is_exit_on_create()) // Exit after creating the db
        {
            exit(0);
        }

        auto end = chrono::steady_clock::now();
        // logger->out("Graph loaded in " + chrono::duration_cast<chrono::microseconds>(end - start).count());
        cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;

        // We assume here that the edges have already been inserted into the
        // graph. We just create indices here. Adjlist does not have any
        // indices.
        start = chrono::steady_clock::now();
        if (pr_cli.get_graph_type() == "std" || pr_cli.get_graph_type() == "edgekey")
        {
            //graph.create_indices();
        }
        end = chrono::steady_clock::now();
        cout << "Indices created in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;

        //Now run PR
        start = chrono::steady_clock::now();
        pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), logger);
        end = chrono::steady_clock::now();
        cout << "PR  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
        // logger->out("PR  completed in : " +
        // chrono::duration_cast<chrono::microseconds>(end - start).count());
        graph.close();
    }
    else if (pr_cli.get_graph_type() == "adjlist")
    {
        Logger *logger = new Logger("pr_run", opts.db_name, "pagerank", pr_cli.get_dataset());

        AdjList graph(opts);
        if (pr_cli.is_exit_on_create()) // Exit after creating the db
        {
            exit(0);
        }
        //Now run PR
        auto start = chrono::steady_clock::now();
        pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), logger);
        auto end = chrono::steady_clock::now();
        cout << "PR  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
        // logger->out("PR  completed in : " +
        // chrono::duration_cast<chrono::microseconds>(end - start).count());
        graph.close();
    }
    else if (pr_cli.get_graph_type() == "edgekey")
    {
        Logger *logger = new Logger("pr_run", opts.db_name, "pagerank", pr_cli.get_dataset());

        EdgeKey graph(opts);
        if (pr_cli.is_exit_on_create()) // Exit after creating the db
        {
            exit(0);
        }
        // We assume here that the edges have already been inserted into the
        // graph. We just create indices here. Adjlist does not have any
        // indices.
        auto start = chrono::steady_clock::now();
        if (pr_cli.get_graph_type() == "std" || pr_cli.get_graph_type() == "edgekey")
        {
            graph.create_indices();
        }
        auto end = chrono::steady_clock::now();
        cout << "Indices created in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;

        //Now run PR
        start = chrono::steady_clock::now();
        pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), logger);
        end = chrono::steady_clock::now();
        cout << "PR  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
        // logger->out("PR  completed in : " +
        // chrono::duration_cast<chrono::microseconds>(end - start).count());
        graph.close();
    }
    else
    {
        std::cout << "Unrecognized graph representation";
    }
}
