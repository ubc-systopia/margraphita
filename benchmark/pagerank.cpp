#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <vector>

#include "adj_list.h"
#include "benchmark_definitions.h"
#include "command_line.h"
#include "common.h"
#include "csv_log.h"
#include "edgekey.h"
#include "graph.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"

const float dampness = 0.85;
std::hash<int> hashfn;
int N = 1610612741;  // Hash bucket size
int p_cur = 0;
int p_next = 1;

pr_map *ptr = nullptr;  // pointer to mmap region
// float *pr_cur, *pr_next;

void init_pr_map(std::vector<node> &nodes)
{
    int size = nodes.size();
    make_pr_mmap(size, &ptr);
    // since the access pattern is random and because the node id's
    // non-continuous we cannot do any clever madvise tricks.
    assert(ptr != nullptr);
    float init_val = 1.0f / size;

    for (node n : nodes)
    {
        if (ptr[n.id].id == 0)
        {
            ptr[n.id].id = n.id;
            ptr[n.id].p_rank[p_cur] = init_val;
            ptr[n.id].p_rank[p_next] = 0.0f;
        }
    }
}

void delete_map(int num_nodes) { munmap(ptr, sizeof(pr_map) * num_nodes); }

template <typename Graph>
void pagerank(Graph &graph,
              graph_opts opts,
              int iterations,
              double tolerance,
              string csv_logdir)
{
    Times t;
    int num_nodes = graph.get_num_nodes();
    t.start();
    std::vector<node> nodes = graph.get_nodes();
    init_pr_map(nodes);
    std::vector<double> times;
    t.stop();
    cout << "Loading the nodes and constructing the map took " << t.t_micros()
         << endl;
    times.push_back(t.t_micros());

    int iter_count = 0;
    float constant = (1 - dampness) / num_nodes;

    while (iter_count < iterations)
    {
        t.start();
        for (node n : nodes)
        {
            int index = hashfn(n.id) % N;
            float sum = 0.0f;
            vector<node> in_nodes = graph.get_in_nodes(
                n.id);  // <-- make this just list of node_ids to avoid looking
                        // up node table

            for (node in : in_nodes)
            {
                sum += (ptr[hashfn(in.id) % N].p_rank[p_cur]) / in.out_degree;
            }
            ptr[index].p_rank[p_next] = constant + (dampness * sum);
        }
        iter_count++;

        p_cur = 1 - p_cur;
        p_next = 1 - p_next;

        t.stop();
        cout << "Iter " << iter_count << "took \t" << to_string(t.t_micros())
             << endl;
        times.push_back(t.t_micros());
    }
    print_to_csv(
        opts.db_name, times, csv_logdir + "/" + opts.db_name + "_old_pr.csv");
    print_map(num_nodes, ptr, p_next);
    delete_map(num_nodes);
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
    opts.db_name = pr_cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = pr_cli.get_db_path();
    std::string pr_log = pr_cli.get_logdir();  //$RESULT/$bmark
    opts.stat_log = pr_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";  // pr_cli.get_conn_config();
    opts.type = pr_cli.get_graph_type();

    const int THREAD_NUM = 1;
    // set_group_id();  // set the group id to GRAPHS_GROUP_ID
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    GraphBase *graph = graphEngine.create_graph_handle();
    t.stop();
    cout << "Graph loaded in " << t.t_micros() << endl;

    // Now run PR
    t.start();
    pagerank(*graph, opts, pr_cli.iterations(), pr_cli.tolerance(), pr_log);
    t.stop();
    cout << "PR  completed in : " << t.t_micros() << endl;
    graph->close();
    graphEngine.close_graph();
}
