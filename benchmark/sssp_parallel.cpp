#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <vector>

#include "adj_list.h"
#include "bitmap.h"
#include "command_line.h"
#include "common_util.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "standard_graph.h"
#include "times.h"

const edgeweight_t DistInf = INT32_MAX;
const node_id_t kMaxBin = numeric_limits<size_t>::max() / 2;
const size_t kBinSizeThreshold = 1000;

inline void RelaxEdges(const GraphBase *g,
                       node_id_t u,
                       edgeweight_t delta,
                       pvector<edgeweight_t> &dist,
                       vector<vector<node_id_t>> &local_bins)
{
    for (edge e : g->get_out_edges(u))
    {
        edgeweight_t old_dist = dist[e.dst_id];
        edgeweight_t new_dist = dist[u] + e.edge_weight;
        while (new_dist < old_dist)
        {
            if (compare_and_swap(dist[e.dst_id], old_dist, new_dist))
            {
                node_id_t dest_bin = new_dist / delta;
                if (dest_bin >= local_bins.size())
                    local_bins.resize(dest_bin + 1);
                local_bins[dest_bin].push_back(e.dst_id);
                break;
            }
            old_dist =
                dist[e.dst_id];  // swap failed, recheck dist update & retry
        }
    }
}

pvector<edgeweight_t> DeltaStep(const GraphEngine &graph_engine,
                                node_id_t source,
                                edgeweight_t delta)
{
    pvector<edgeweight_t> dist(g.num_nodes(), DistInf);
    dist[source] = 0;
    pvector<node_id_t> frontier(g.num_edges_directed());
    // two element arrays for double buffering curr=iter&1, next=(iter+1)&1
    size_t shared_indexes[2] = {0, kMaxBin};
    size_t frontier_tails[2] = {1, 0};
    frontier[0] = source;
#pragma omp parallel
    {
        GraphBase *graph = graph_engine.create_graph_handle();
        vector<vector<NodeID>> local_bins(0);
        size_t iter = 0;
        while (shared_indexes[iter & 1] != kMaxBin)
        {
            size_t &curr_bin_index = shared_indexes[iter & 1];
            size_t &next_bin_index = shared_indexes[(iter + 1) & 1];
            size_t &curr_frontier_tail = frontier_tails[iter & 1];
            size_t &next_frontier_tail = frontier_tails[(iter + 1) & 1];
#pragma omp for nowait schedule(dynamic, 64)
            for (size_t i = 0; i < curr_frontier_tail; i++)
            {
                node_id_t u = frontier[i];
                if (dist[u] >=
                    delta * static_cast<edgeweight_t>(curr_bin_index))
                    RelaxEdges(graph, u, delta, dist, local_bins);
            }
            while (curr_bin_index < local_bins.size() &&
                   !local_bins[curr_bin_index].empty() &&
                   local_bins[curr_bin_index].size() < kBinSizeThreshold)
            {
                vector<node_id_t> curr_bin_copy = local_bins[curr_bin_index];
                local_bins[curr_bin_index].resize(0);
                for (node_id_t u : curr_bin_copy)
                    RelaxEdges(graph, u, delta, dist, local_bins);
            }
            for (size_t i = curr_bin_index; i < local_bins.size(); i++)
            {
                if (!local_bins[i].empty())
                {
#pragma omp critical
                    next_bin_index = min(next_bin_index, i);
                    break;
                }
            }
#pragma omp barrier
#pragma omp single nowait
            {
                curr_bin_index = kMaxBin;
                curr_frontier_tail = 0;
            }
            if (next_bin_index < local_bins.size())
            {
                size_t copy_start = fetch_and_add(
                    next_frontier_tail, local_bins[next_bin_index].size());
                copy(local_bins[next_bin_index].begin(),
                     local_bins[next_bin_index].end(),
                     frontier.data() + copy_start);
                local_bins[next_bin_index].resize(0);
            }
            iter++;
#pragma omp barrier
        }
#pragma omp single
        {
            cout << "took " << iter << " iterations" << endl;
        }
        graph->close();
    }
    graph_engine.close_graph();
    return dist;
}