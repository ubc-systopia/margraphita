#include <stdio.h>
#include <vector>
#include <math.h>
#include <chrono>
#include <unistd.h>
#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "command_line.h"
#include "logger.h"
#include "reader.h"

/**
 * @brief This runs the PageRank kernel on the 
 * 
 */

using namespace std;
const float dampness = 0.85;

vector<float> pagerank(StandardGraph &graph, graph_opts opts, int iterations, double tolerance, Logger *logger)
{

    int num_nodes = graph.get_num_nodes();
    const float init_score = 1.0f / num_nodes;
    const float base_score = (1.0f - dampness) / num_nodes;

    vector<float> scores(num_nodes, init_score);
    vector<float> outgoing_contribution(num_nodes);

    for (int iter = 0; iter < iterations; iter++)
    {
        cout << iter << endl;
        auto start = chrono::steady_clock::now();
        double error = 0;
        for (int nodeid = 0; nodeid < num_nodes; nodeid++)
        {
            if (graph.get_out_degree(nodeid))
            {
                outgoing_contribution[nodeid] = scores[nodeid] / graph.get_out_degree(nodeid);
            }
        }
        for (int nodeid = 0; nodeid < num_nodes; nodeid++)
        {
            float incoming_total = 0;
            for (node n : graph.get_in_nodes(nodeid))
            {
                incoming_total += outgoing_contribution[n.id];
            }
            float old_score = scores[nodeid];
            scores[nodeid] = base_score + dampness * incoming_total;
            error += fabs(scores[nodeid] - old_score);
        }
        printf("%2d     %lf\n", iter, error);
        if (error < tolerance)
        {
            break;
        }
        auto end = chrono::steady_clock::now();
        // logger->out(to_string(chrono::duration_cast<chrono::microseconds>(end
        // - start).count()));
        cout << iter << "\t" << to_string(chrono::duration_cast<chrono::microseconds>(end - start).count());
    }
    return scores;
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

        auto end = chrono::steady_clock::now();
        // logger->out("Graph loaded in " + chrono::duration_cast<chrono::microseconds>(end - start).count());
        cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
        start = chrono::steady_clock::now();
        //Now insert edges into the graph if create_new is true
        if (pr_cli.is_create_new())
        {
            vector<edge> edges = reader::parse_entries(pr_cli.get_filename());
            cout << "number of edges read " << edges.size() << endl;
            if (edges.size() == 0)
            {
                cout << "Could not parse the file containing edges. Abort.";
                exit(-1);
            }
            for (edge e : edges)
            {
                graph.add_edge(e);
            }
        }

        end = chrono::steady_clock::now();
        cout << "Edges inserted in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
        //  logger->out("Edges inserted in " + chrono::duration_cast<chrono::microseconds>(end - start).count());
        graph.close();
        exit(0);
        start = chrono::steady_clock::now();
        vector<float> scores = pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), logger);
        end = chrono::steady_clock::now();
        cout << "PR  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
        // logger->out("PR  completed in : " +
        // chrono::duration_cast<chrono::microseconds>(end - start).count());
        graph.close();
    }
    else
    {
        cout << "unimplemented.";
        return -1;
    }
}