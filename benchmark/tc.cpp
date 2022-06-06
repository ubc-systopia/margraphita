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

bool node_compare(node a, node b) { return ((a.id < b.id)); }

std::vector<node_id_t> intersection(std::vector<node_id_t> A,
                                    std::vector<node_id_t> B)
{
    size_t a = A.size();
    size_t b = B.size();
    std::vector<node_id_t> ABintersection;
    std::vector<node_id_t>::iterator A_iter = A.begin();
    std::vector<node_id_t>::iterator B_iter = B.begin();
    size_t i = 0, j = 0, k = 0;
    while (i < a and j < b)
    {
        if ((*A_iter) == (*B_iter))
        {
            ABintersection[k] = *A_iter;
            k++;
            ++A_iter;
            ++B_iter;
        }
        else if ((*A_iter) < (*B_iter))
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
int64_t trust_tc(Graph &graph)
{
    int64_t count = 0;
    vector<node> nodes = graph->get_nodes();
    for (node n : nodes)
    {
        vector<edge> out_edges = graph->get_out_edges(n.id);

        for (edge e : out_edges)
        {
            vector<edge> edges2;
            node_id_t nbr_id = e.dst_id;
            if (nbr_id == n.id)
            {
                continue;  // self loop -- ignore.
            }

            edges2 = graph->get_out_edges(nbr_id);

            set<node_id_t> edge_nei;
            set<node_id_t> edge2_nei;

            for (edge e : out_edges)
            {
                if (e.src_id != e.dst_id)
                {
                    edge_nei.insert(e.dst_id);
                }
            }

            for (edge e : edges2)
            {
                if (e.src_id != e.dst_id &&
                    edge_nei.find(e.dst_id) != edge_nei.end())
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
int64_t cycle_tc(Graph &graph)
{
    int64_t count = 0;
    vector<node> nodes = graph->get_nodes();
    for (node n : nodes)
    {
        vector<node> out_nodes = graph->get_out_nodes(n.id);
        for (node out_node : out_nodes)
        {
            if (n.id < out_node.id)
            {
                std::vector<node> intersect =
                    (graph->get_in_nodes(n.id),
                     graph->get_out_nodes(out_node.id));
                for (node w : intersect)
                {
                    if (n.id < w.id and out_node.id < w.id)
                    {
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