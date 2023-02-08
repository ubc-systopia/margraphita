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
#include "graph_engine.h"
#include "graph_exception.h"
#include "pvector.h"
#include "standard_graph.h"
#include "thread_utils.h"
#include "times.h"

const float dampness = 0.85;
int p_cur = 0;
int p_next = 1;
typedef float ScoreT;

pr_iter_map *ptr;  // pointer to mmap region

/**
 * @brief This function takes a single parameter - the number of nodes in
 * the graph. We then allocate a memory region of size N*sizeof(pr_map) and
 * initialize the memory region with 1/N.
 *
 * @param N
 */
void init_pr_map(int N, std::vector<node> &nodes)
{
    make_pr_mmap(N, &ptr);
    float init_val = 1.0f / N;

#pragma omp parallel for
    for (int i = 0; i < N; i++)
    {
        ptr[i].id = nodes.at(i).id;  // We are not assigning node_id's to the
                                     // pr_map struct at this point.
        ptr[i].p_rank[0] = init_val;
        ptr[i].p_rank[1] = 0.0f;
        ptr[i].in_deg = nodes.at(i).in_degree;
        ptr[i].out_deg = nodes.at(i).out_degree;
    }
}

void print_map(int N)
{
    ofstream FILE;
    FILE.open("pr_out.txt", ios::out | ios::ate);
    for (int i = 0; i < N; i++)
    {
        FILE << ptr[i].id << "\t" << ptr[i].p_rank[p_next] << "\n";
    }
    FILE.close();
}

void delete_map(int N) { munmap(&ptr, sizeof(pr_map) * N); }

/**
 * !If we want to parallelize this function, we need per-thread offsets into the
 * pr_map. init_pr_map would need to be called prior to this function. Extend
 * PR_map to have a lock.
 */
void pagerank(GraphBase *graph,
              graph_opts opts,
              int iterations,
              double tolerance,
              string csv_logdir)
{
    Times t;
    t.start();
    std::vector<node> nodes = graph->get_nodes();
    int num_nodes = nodes.size();
    init_pr_map(num_nodes, nodes);
    nodes.clear();
    std::vector<node>().swap(nodes);
    t.stop();
    cout << "Loading the nodes and constructing the map took " << t.t_micros()
         << endl;
    std::vector<double> times;
    times.push_back(t.t_micros());

    double diff = 1.0;
    int iter_count = 0;
    float constant = (1 - dampness) / num_nodes;

    while (iter_count < iterations)
    {
        t.start();
        int i = 0;

        int index = 0;
        adjlist found = {0};
        node curr_node = {0};
        InCursor *in_cursor = graph->get_innbd_iter();
        in_cursor->next(&found);

        while (found.node_id != -1)
        {
            float sum = 0.0f;
            for (auto in_node : found.edgelist)
            {
                sum += (ptr[in_node].p_rank[p_cur]) / ptr[in_node].out_deg;
            }
            ptr[index].p_rank[p_next] = constant + (dampness * sum);
            index++;
            in_cursor->next(&found);
        }
        iter_count++;

        p_cur = 1 - p_cur;
        p_next = 1 - p_next;

        t.stop();
        cout << "Iter " << iter_count << "took \t" << t.t_micros()
             << "(nodes = " << index << ")" << endl;
        times.push_back(t.t_micros());
    }
    print_to_csv(
        opts.db_name, times, csv_logdir + "/" + opts.db_name + "_iter_map.csv");
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
    opts.conn_config = "cache_size=10GB";  // tc_cli.get_conn_config();
    opts.type = pr_cli.get_graph_type();

    const int THREAD_NUM = 1;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    GraphBase *graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    // Now run PR
    t.start();
    pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), pr_log);
    t.stop();
    cout << "PR  completed in : " << t.t_micros() << endl;
    graph->close();
    graphEngine.close_graph();
}