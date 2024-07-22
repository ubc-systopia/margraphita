#include <fstream>
#include <iostream>
#include <shared_mutex>
#include <vector>
#include "benchmark_definitions.h"
#include "command_line.h"
#include "common_util.h"
#include "csv_log.h"
#include "edgekey.h"
#include "graph_engine.h"
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
void init_pr_map(size_t N, std::vector<node> &nodes)
{
    make_pr_mmap(N, &ptr);
    float init_val = 1.0f / static_cast<float>(N);

#pragma omp parallel for
    for (uint64_t i = 0; i < N; i++)
    {
        ptr[i].id = nodes.at(i).id;  // We are not assigning node_id's to the
                                     // pr_map struct at this point.
        ptr[i].p_rank[0] = init_val;
        ptr[i].p_rank[1] = 0.0f;
        ptr[i].in_deg = nodes.at(i).in_degree;
        ptr[i].out_deg = nodes.at(i).out_degree;
    }
}


/**
 * !If we want to parallelize this function, we need per-thread offsets into the
 * pr_map. init_pr_map would need to be called prior to this function. Extend
 * PR_map to have a lock.
 */
void pagerank(GraphBase *graph,
              const cmdline_opts &opts,
              int iterations,
              double tolerance,
              const std::string &csv_logdir)
{
    Times t;
    t.start();
    std::vector<node> nodes = graph->get_nodes();
    size_t num_nodes = nodes.size();
    init_pr_map(num_nodes, nodes);
    nodes.clear();
    std::vector<node>().swap(nodes);
    t.stop();
    cout << "Loading the nodes and constructing the map took " << t.t_micros()
         << endl;
    std::vector<long double> times;
    times.push_back(t.t_micros());

    int iter_count = 0;
    float constant = (1 - dampness) / (float)num_nodes;

    while (iter_count < iterations)
    {
        t.start();
        int index = 0;
        adjlist found;
        InCursor *in_cursor = graph->get_innbd_iter();
        in_cursor->next(&found);

        while (found.node_id != OutOfBand_ID_MAX)
        {
            float sum = 0.0f;
            for (auto in_node : found.edgelist)
            {
                sum += (ptr[in_node].p_rank[p_cur]) /
                       static_cast<float>(ptr[in_node].out_deg);
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
    std::string map_file = csv_logdir + "/" + opts.db_name + "_pr_iter_map.txt";
    print_map(map_file, num_nodes, ptr, p_next);
    delete_map(num_nodes, ptr);
}

int main(int argc, char *argv[])
{
    cout << "Running PageRank" << endl;
    PageRankOpts pr_cli(argc, argv, 1e-4, 10);
    if (!pr_cli.parse_args())
    {
        return -1;
    }

    cmdline_opts opts = pr_cli.get_parsed_opts();
    opts.stat_log += "/" + opts.db_name;

    const int THREAD_NUM = 1;

    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    GraphBase *graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    // Now run PR
    t.start();
    pagerank(graph, opts, opts.iterations, opts.tolerance, opts.stat_log);
    t.stop();
    cout << "PR  completed in : " << t.t_micros() << endl;
    graph->close(false);
    graphEngine.close_graph();
}