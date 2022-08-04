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
#include "pvector.h"
#include "standard_graph.h"
#include "times.h"

typedef float ScoreT;
const float kDamp = 0.85;

pvector<ScoreT> PageRankPullGS(const GraphEngine& graph_engine,
                               int thread_num,
                               int max_iters,
                               double epsilon = 0)
{
    GraphBase* g = myEngine.create_graph_handle();

    const ScoreT init_score = 1.0f / g->get_num_nodes();
    const ScoreT base_score = (1.0f - kDamp) / g->get_num_nodes();

    pvector<ScoreT> scores(g->get_num_nodes(), init_score);
    pvector<ScoreT> outgoing_contrib(g->get_num_nodes());

    g->close();

#pragma omp parallel for
    for (int i = 0; i < thread_num; i++)
    {
        GraphBase* graph = graph_engine.create_graph_handle();
        NodeCursor* node_cursor = graph->get_node_cursor();
        node_cursor->set_key_range(graph_engine.get_key_range(i));

        node found = {0};

        node_cursor->next(&found);

        while (found.id != -1)
        {
            outgoing_contrib[found.id] = init_score / found.out_degree;
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
            InCursor* in_cursor = graph->get_innbd_cursor();
            in_cursor->set_key_range(graph_engine.get_key_range(i));

            adjlist found = {0};

            in_cursor->next(&found);

            while (found.id != -1)
            {
                ScoreT incoming_total = 0;
                for (node_id_t v : found.edgelist)
                {
                    incoming_total += outgoing_contrib[v];
                }

                ScoreT old_score = scores[found.id];
                scores[found] = base_score + kDamp * incoming_total;
                error += fabs(scores[found.id] - old_score);
                outgoing_contrib[found.id] =
                    scores[found.id] / graph.get_out_degree(found.id);

                in_cursor->next(&found);
            }

            graph->close();
        }
        printf(" %2d    %lf\n", iter, error);
        if (error < epsilon) break;
    }
    return scores;
}