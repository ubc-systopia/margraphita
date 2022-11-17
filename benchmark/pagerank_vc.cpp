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
                         double epsilon = 0)
{
    GraphBase* g = graph_engine.create_graph_handle();
    node_id_t num_nodes = g->get_num_nodes();
    g->close();

    pvector<ScoreT> src(num_nodes, 0);
    pvector<ScoreT> dst(num_nodes, 1 / num_nodes);
    pvector<node_id_t> deg(num_nodes, 0);

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

            adjlist found = {0};
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

            graph->close();
        }
        printf(" %2d    %lf\n", iter, error);
        if (error < epsilon) break;
    }
    return dst;
}

int main(int argc, char* argv[])
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
    graphEngine.calculate_thread_offsets_edge_partition();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    // Now run PR
    t.start();
    pvector<ScoreT> score = pagerank(
        graphEngine, THREAD_NUM, pr_cli.iterations(), pr_cli.tolerance());
    t.stop();
    cout << "PR  completed in : " << t.t_micros() << endl;
    double sum = 0;
    for (int i = 0; i < 30399; i++)
    {
        if (score[i] != std::numeric_limits<float>::infinity()) sum += score[i];
    }
    cout << sum << '\n';
    graphEngine.close_graph();
}