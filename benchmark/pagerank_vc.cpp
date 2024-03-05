#include <omp.h>

#include <cmath>
#include <cstdio>
#include <iostream>

#include "benchmark_definitions.h"
#include "command_line.h"
#include "common_util.h"
#include "graph_engine.h"
#include "pvector.h"
#include "times.h"

typedef float ScoreT;
const float kDamp = 0.85;

pvector<ScoreT> pagerank(GraphEngine& graph_engine,
                         int thread_num,
                         int max_iters,
                         node_id_t num_nodes,
                         node_id_t max_node_id,
                         double epsilon = 0)
{
    pvector<ScoreT> src(max_node_id, 0);
    pvector<ScoreT> dst(max_node_id, 1 / num_nodes);
    pvector<node_id_t> deg(max_node_id, 0);

#pragma omp parallel for
    for (int i = 0; i < thread_num; i++)
    {
        GraphBase* graph = graph_engine.create_graph_handle();
        NodeCursor* node_cursor = graph->get_node_iter();
        node_cursor->set_key_range(graph_engine.get_key_range(i));

        node found = {0};
        node_cursor->next(&found);

        while (found.id != -1)
        {
            deg[found.id] = found.out_degree;
            node_cursor->next(&found);
        }
        node_cursor->close();
        graph->close();
    }

    for (int iter = 0; iter < max_iters; iter++)
    {
        double error = 0;
#pragma omp parallel for reduction(+ : error)
        for (int i = 0; i < thread_num; i++)
        {
            GraphBase* graph = graph_engine.create_graph_handle();
            InCursor* in_cursor = graph->get_innbd_iter();
            in_cursor->set_key_range(graph_engine.get_key_range(i));

            adjlist found;
            in_cursor->next(&found);

            while (found.node_id != -1)
            {
                ScoreT incoming_total = 0;
                for (node_id_t v : found.edgelist)
                {
                    incoming_total += src[v];
                }

                ScoreT old_score = dst[found.node_id];
                dst[found.node_id] =
                    (1 - kDamp) / num_nodes + kDamp * incoming_total;
                error += fabs(dst[found.node_id] - old_score);
                src[found.node_id] = dst[found.node_id] / deg[found.node_id];

                in_cursor->next(&found);
            }

            in_cursor->close();
            graph->close();
        }
        printf(" %2d    %lf\n", iter, error);
        if (error < epsilon) break;
    }
    return dst;
}

// Returns k pairs with largest values from list of key-value pairs
template <typename KeyT, typename ValT>
std::vector<std::pair<ValT, KeyT>> TopK(
    const std::vector<std::pair<KeyT, ValT>>& to_sort, size_t k)
{
    std::vector<std::pair<ValT, KeyT>> top_k;
    ValT min_so_far = 0;
    for (auto kvp : to_sort)
    {
        if ((top_k.size() < k) || (kvp.second > min_so_far))
        {
            top_k.push_back(std::make_pair(kvp.second, kvp.first));
            std::sort(top_k.begin(),
                      top_k.end(),
                      std::greater<std::pair<ValT, KeyT>>());
            if (top_k.size() > k) top_k.resize(k);
            min_so_far = top_k.back().first;
        }
    }
    return top_k;
}

void print_top_scores(pvector<ScoreT>& score, node_id_t n_nodes, GraphBase* g)
{
    std::vector<std::pair<node_id_t, ScoreT>> score_pairs;
    NodeCursor* node_cursor = g->get_node_iter();
    node found = {0};
    node_cursor->next(&found);
    while (found.id != UINT32_MAX)
    {
        score_pairs.emplace_back(found.id, score[found.id]);
        node_cursor->next(&found);
    }
    node_id_t k = 100;
    vector<pair<ScoreT, node_id_t>> top_k = TopK(score_pairs, k);
    node_id_t it = 0;
    std::cout << "Top " << k << " nodes by PageRank:" << std::endl;
    for (auto kvp : top_k)
    {
        it++;
        if (kvp.first > 1e-4)
            std::cout << "(" << it << ") " << kvp.second << ":" << kvp.first
                      << std::endl;

        std::cout << kvp.second << ":" << kvp.first << std::endl;
    }
}

int main(int argc, char* argv[])
{
    cout << "Running PageRank" << endl;
    PageRankOpts pr_cli(argc, argv, 1e-4, 10);
    if (!pr_cli.parse_args())
    {
        return -1;
    }

    cmdline_opts opts = pr_cli.get_parsed_opts();
    opts.stat_log += "/" + opts.db_name;

    const int THREAD_NUM = omp_get_max_threads();
    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    graphEngine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;
    // return 0;
    // get the number of nodes in the graph:
    // Now run PR
    long double total_time = 0;
    for (int i = 0; i < opts.num_trials; i++)
    {
        t.start();
        GraphBase* g = graphEngine.create_graph_handle();
        node_id_t num_nodes = g->get_num_nodes();
        node_id_t max_node_id = g->get_max_node_id();
        g->close();
        pvector<ScoreT> score = pagerank(graphEngine,
                                         THREAD_NUM,
                                         opts.iterations,
                                         num_nodes,
                                         max_node_id,
                                         opts.tolerance);
        t.stop();
        cout << "PR  completed in : " << t.t_secs() << endl;
        total_time += t.t_secs();
        if (i == opts.num_trials - 1 && opts.print_stats)
            print_top_scores(score, num_nodes, g);
    }
    cout << "Average time: " << total_time / opts.num_trials << endl;
    graphEngine.close_graph();
}