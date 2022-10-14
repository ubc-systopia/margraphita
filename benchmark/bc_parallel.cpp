#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <vector>

#include "adj_list.h"
#include "bitmap.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "source_picker.h"
#include "standard_graph.h"
#include "times.h"

using namespace std;
typedef float ScoreT;
typedef double CountT;

void PBFS(const GraphBase &g,
          node_id_t source,
          pvector<CountT> &path_counts,
          Bitmap &succ,
          vector<SlidingQueue<node_id_t>::iterator> &depth_index,
          SlidingQueue<node_id_t> &queue)
{
    pvector<node_id_t> depths(g.get_num_nodes(), -1);
    depths[source] = 0;
    path_counts[source] = 1;
    queue.push_back(source);
    depth_index.push_back(queue.begin());
    queue.slide_window();
    const node_id_t *g_out_start = g.get_out_nodes_id(0).begin();

#pragma omp parallel
    {
        node_id_t depth = 0;
        QueueBuffer<node_id_t> lqueue(queue);
        while (!queue.empty())
        {
            depth++;

#pragma omp for schedule(dynamic, 64) nowait
            for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++)
            {
                node_id_t u = *q_iter;
                for (node_id_t &v : g.get_out_nodes_id(u))
                {
                    if ((depths[v] == -1) &&
                        (compare_and_swap(
                            depths[v], static_cast<node_id_t>(-1), depth)))
                    {
                        lqueue.push_back(v);
                    }
                    if (depths[v] == depth)
                    {
                        succ.set_bit_atomic(&v - g_out_start);
#pragma omp atomic
                        path_counts[v] += path_counts[u];
                    }
                }
            }
            lqueue.flush();
#pragma omp barrier
#pragma omp single
            {
                depth_index.push_back(queue.begin());
                queue.slide_window();
            }
        }
    }
    depth_index.push_back(queue.begin());
}

pvector<ScoreT> Brandes(const GraphEngine &graph_engine,
                        SourcePicker &sp,
                        int num_iters)
{
    GraphBase g = *graph_engine.create_graph_handle();
    pvector<ScoreT> scores(g.get_num_nodes(), 0);
    pvector<CountT> path_counts(g.get_num_nodes());
    Bitmap succ(g.get_num_edges());
    vector<SlidingQueue<node_id_t>::iterator> depth_index;
    SlidingQueue<node_id_t> queue(g.get_num_nodes());
    const node_id_t *g_out_start = g.get_out_nodes_id(0).begin();

    for (int iter = 0; iter < num_iters; iter++)
    {
        node_id_t source = sp.PickNext();
        path_counts.fill(0);
        depth_index.resize(0);
        queue.reset();
        succ.reset();
        PBFS(g, source, path_counts, succ, depth_index, queue);
        pvector<ScoreT> deltas(g.get_num_nodes(), 0);

        for (int d = depth_index.size() - 2; d >= 0; d--)
        {
#pragma omp parallel for schedule(dynamic, 64)
            for (auto it = depth_index[d]; it < depth_index[d + 1]; it++)
            {
                node_id_t u = *it;
                ScoreT delta_u = 0;
                for (node_id_t &v : g.get_out_nodes_id(u))
                {
                    if (succ.get_bit(&v - g_out_start))
                    {
                        delta_u +=
                            (path_counts[u] / path_counts[v]) * (1 + deltas[v]);
                    }
                }
                deltas[u] = delta_u;
                scores[u] += delta_u;
            }
        }
    }

    // normalize scores
    ScoreT biggest_score = 0;
#pragma omp parallel for reduction(max : biggest_score)
    for (node_id_t n = 0; n < g.get_num_nodes(); n++)
        biggest_score = max(biggest_score, scores[n]);

#pragma omp parallel for
    for (node_id_t n = 0; n < g.get_num_nodes(); n++)
        scores[n] = scores[n] / biggest_score;

    return scores;
}