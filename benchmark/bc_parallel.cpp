#include <functional>
#include <iostream>
#include <vector>

#include "bitmap.h"
#include "command_line.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"

using namespace std;
typedef float ScoreT;
typedef double CountT;

void PBFS(const Graph &g,
          NodeID source,
          pvector<CountT> &path_counts,
          Bitmap &succ,
          vector<SlidingQueue<NodeID>::iterator> &depth_index,
          SlidingQueue<NodeID> &queue)
{
    pvector<NodeID> depths(g.num_nodes(), -1);
    depths[source] = 0;
    path_counts[source] = 1;
    queue.push_back(source);
    depth_index.push_back(queue.begin());
    queue.slide_window();
    const NodeID *g_out_start = g.out_neigh(0).begin();
#pragma omp parallel
    {
        NodeID depth = 0;
        QueueBuffer<NodeID> lqueue(queue);
        while (!queue.empty())
        {
            depth++;
#pragma omp for schedule(dynamic, 64) nowait
            for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++)
            {
                NodeID u = *q_iter;
                for (NodeID &v : g.out_neigh(u))
                {
                    if ((depths[v] == -1) &&
                        (compare_and_swap(
                            depths[v], static_cast<NodeID>(-1), depth)))
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

pvector<ScoreT> Brandes(const Graph &g,
                        SourcePicker<Graph> &sp,
                        NodeID num_iters)
{
    Timer t;
    t.Start();
    pvector<ScoreT> scores(g.num_nodes(), 0);
    pvector<CountT> path_counts(g.num_nodes());
    Bitmap succ(g.num_edges_directed());
    vector<SlidingQueue<NodeID>::iterator> depth_index;
    SlidingQueue<NodeID> queue(g.num_nodes());
    t.Stop();
    PrintStep("a", t.Seconds());
    const NodeID *g_out_start = g.out_neigh(0).begin();
    for (NodeID iter = 0; iter < num_iters; iter++)
    {
        NodeID source = sp.PickNext();
        cout << "source: " << source << endl;
        t.Start();
        path_counts.fill(0);
        depth_index.resize(0);
        queue.reset();
        succ.reset();
        PBFS(g, source, path_counts, succ, depth_index, queue);
        t.Stop();
        PrintStep("b", t.Seconds());
        pvector<ScoreT> deltas(g.num_nodes(), 0);
        t.Start();
        for (int d = depth_index.size() - 2; d >= 0; d--)
        {
#pragma omp parallel for schedule(dynamic, 64)
            for (auto it = depth_index[d]; it < depth_index[d + 1]; it++)
            {
                NodeID u = *it;
                ScoreT delta_u = 0;
                for (NodeID &v : g.out_neigh(u))
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
        t.Stop();
        PrintStep("p", t.Seconds());
    }
    // normalize scores
    ScoreT biggest_score = 0;
#pragma omp parallel for reduction(max : biggest_score)
    for (NodeID n = 0; n < g.num_nodes(); n++)
        biggest_score = max(biggest_score, scores[n]);
#pragma omp parallel for
    for (NodeID n = 0; n < g.num_nodes(); n++)
        scores[n] = scores[n] / biggest_score;
    return scores;
}

void PrintTopScores(const Graph &g, const pvector<ScoreT> &scores)
{
    vector<pair<NodeID, ScoreT>> score_pairs(g.num_nodes());
    for (NodeID n : g.vertices()) score_pairs[n] = make_pair(n, scores[n]);
    int k = 5;
    vector<pair<ScoreT, NodeID>> top_k = TopK(score_pairs, k);
    for (auto kvp : top_k) cout << kvp.second << ":" << kvp.first << endl;
}