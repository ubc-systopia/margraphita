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

#include "adj_list.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "standard_graph.h"
#include "times.h"

typedef float ScoreT;
const float kDamp = 0.85;

pvector<ScoreT> pagerank(GraphEngine& graph_engine,
                         int thread_num,
                         int max_iters,
                         graph_opts opts,
                         double epsilon = 0)
{
    GraphBase* g = graph_engine.create_graph_handle();
    node_id_t num_nodes = g->get_num_nodes();
    g->close();

    pvector<ScoreT> src(num_nodes, 0);
    pvector<ScoreT> dst(num_nodes, 1 / num_nodes);
    pvector<node_id_t> deg(num_nodes, 0);

    if (opts.type == GraphType::EList)
    {
#pragma omp parallel for num_threads(thread_num)
        for (int i = 0; i < thread_num; i++)
        {
            GraphBase* graph = graph_engine.create_graph_handle();
            EdgeCursor* edge_cursor = graph->get_edge_iter();
            edge_cursor->set_key_range(graph_engine.get_edge_range(i));

            edge found;
            edge_cursor->next(&found);

            while (found.src_id != -1)
            {
                bool swapped = false;
                while (!swapped)
                {
                    swapped = compare_and_swap(deg[found.src_id],
                                               deg[found.src_id],
                                               deg[found.src_id] + 1);
                }
                edge_cursor->next(&found);
            }

            graph->close();
        }
    }
    else
    {
#pragma omp parallel for num_threads(thread_num)
        for (int i = 0; i < thread_num; i++)
        {
            GraphBase* graph = graph_engine.create_graph_handle();
            NodeCursor* node_cursor = graph->get_node_iter();
            node_cursor->set_key_range(graph_engine.get_key_range(i));

            node found = {0};
            node_cursor->next(&found);

            int count = 0;
            while (found.id != -1)
            {
                deg[found.id] = found.out_degree;
                node_cursor->next(&found);
            }

            graph->close();
        }
    }

    for (int iter = 0; iter < max_iters; iter++)
    {
        double error = 0;
#pragma omp parallel for reduction(+ : error) num_threads(thread_num)
        for (int i = 0; i < num_nodes; i++)
        {
            if (src[i] != std::numeric_limits<float>::infinity() &&
                dst[i] != std::numeric_limits<float>::infinity())
            {
                error += fabs(src[i] * deg[i] / kDamp - dst[i]);
            }
            src[i] = kDamp * dst[i] / deg[i];
            dst[i] = (1.0f - kDamp) / num_nodes;
        }
        printf(" %2d    %f\n", iter, error);
        if (iter > 0 && error < epsilon) break;

#pragma omp parallel for num_threads(thread_num)
        for (int i = 0; i < thread_num; i++)
        {
            GraphBase* graph = graph_engine.create_graph_handle();
            EdgeCursor* edge_cursor = graph->get_edge_iter();
            edge_cursor->set_key_range(graph_engine.get_edge_range(i));

            edge found;
            edge_cursor->next(&found);

            while (found.src_id != -1)
            {
                bool swapped = false;
                while (!swapped)
                {
                    swapped =
                        compare_and_swap(dst[found.dst_id],
                                         dst[found.dst_id],
                                         src[found.src_id] + dst[found.dst_id]);
                }
                edge_cursor->next(&found);
            }

            graph->close();
        }
    }
    return dst;
}

int main(int argc, char* argv[])
{
    cout << "Running PageRank" << endl;
    PageRankOpts pr_cli(argc, argv, 0, 0);
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
    opts.stat_log = pr_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";  // tc_cli.get_conn_config();
    opts.type = pr_cli.get_graph_type();

    const int THREAD_NUM = 8;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};
    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    graphEngine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    // Now run PR
    t.start();
    pvector<ScoreT> score = pagerank(
        graphEngine, THREAD_NUM, pr_cli.iterations(), opts, pr_cli.tolerance());
    t.stop();
    cout << "PR  completed in : " << t.t_micros() << endl;
    double sum = 0;
    for (int i = 0; i < 30399; i++)
    {
        if (score[i] != std::numeric_limits<float>::infinity()) sum += score[i];
    }
    cout << sum << '\n';

    // for (int i = 30000; i < 30100; i++)
    // {
    //     cout << score[i] << '\n';
    // }
    graphEngine.close_graph();
}