#include <stdio.h>
#include <vector>
#include <math.h>
#include <set>
#include <deque>
#include <algorithm>
#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "command_line.h"
#include "logger.h"

/**
 * This runs the Triangle Counting on the graph -- both Trust and Cycle counts
 */

using namespace std;

int tc(StandardGraph graph, bool isTrust)
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

            if (isTrust or e.dst_id > n.id)
            {
                edges2 = graph.get_out_edges(nbr_id);
            }

            if (isTrust)
            {
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

                // set<int> intersect;
                // set_intersection(edge_nei.begin(), edge_nei.end(), edge2_nei.begin(), edge2_nei.end(), std::inserter(intersect, intersect.begin()));
                // count += intersect.size();
            }

            else
            { //Cycle Triangle
                if (e.dst_id > n.id)
                {
                    for (edge edge2 : edges2)
                    {
                        edge last_edge = graph.get_edge(edge2.dst_id, n.id);
                        if ((edge2.dst_id > e.dst_id) && (last_edge.id != 0))
                        {
                            count++;
                        }
                    }
                }
            }
        }
    }

    return count;
}

int main(int argc, char *argv[])
{
    cout << "Running TC" << endl;
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

    if (tc_cli.get_graph_type() == "std")
    {
        Logger *logger = new Logger("tc_run", opts.db_name, "tc", tc_cli.get_dataset());
        auto start = chrono::steady_clock::now();
        StandardGraph graph(opts);
        auto end = chrono::steady_clock::now();
        logger->out("Graph loaded in " + chrono::duration_cast<chrono::microseconds>(end - start).count());

        for (int i = 0; i < tc_cli.get_num_trials(); i++)
        {
            //Count Trust Triangles
            node random_start = graph.get_random_node();
            start = chrono::steady_clock::now();
            int count = tc(graph, true);
            end = chrono::steady_clock::now();
            logger->out("Trust TriangleCounting completed in : " + chrono::duration_cast<chrono::microseconds>(end - start).count());
            logger->out("Trust Triangles count = " + count);

            //Count Cycle Triangles
            start = chrono::steady_clock::now();
            count = tc(graph, false);
            end = chrono::steady_clock::now();
            logger->out("Cycle TriangleCounting  completed in : " + chrono::duration_cast<chrono::microseconds>(end - start).count());
            logger->out("Cycle Triangles count = " + count);
        }
    }
    else
    {
        cout << "unimplemented.";
        return -1;
    }
}