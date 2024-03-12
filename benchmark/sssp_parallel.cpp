#include <chrono>
#include <iostream>
#include <vector>

#include "benchmark_definitions.h"
#include "bitmap.h"
#include "command_line.h"
#include "csv_log.h"
#include "graph_engine.h"
#include "omp.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "times.h"

const edgeweight_t DistInf = numeric_limits<edgeweight_t>::max() / 2;
const size_t kMaxBin = numeric_limits<size_t>::max() / 2;
const size_t kBinSizeThreshold = 1000;
const int THREAD_NUM = omp_get_max_threads();

inline void RelaxEdges(GraphBase *g,
                       node_id_t u,
                       edgeweight_t delta,
                       pvector<edgeweight_t> &dist,
                       vector<vector<node_id_t>> &local_bins)
{
    std::vector<edge> out_edges = g->get_out_edges(u);
    for (edge e : out_edges)
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

void PrintStep(size_t step, long double seconds, size_t count = -1)
{
    if (count != -1)
        printf("%5zu%11" PRId64 "  %10.5Lf\n", step, count, seconds);
    else
        printf("%5zu%23.5Lf\n", step, seconds);
}

pvector<edgeweight_t> DeltaStep(GraphEngine &graph_engine,
                                node_id_t source,
                                edgeweight_t delta,
                                node_id_t maxNodeID,
                                edge_id_t num_edges)
{
    pvector<edgeweight_t> dist(maxNodeID, DistInf);
    dist[source] = 0;
    pvector<node_id_t> frontier(num_edges);
    // two element arrays for double buffering curr=iter&1, next=(iter+1)&1
    size_t shared_indexes[2] = {0, kMaxBin};
    size_t frontier_tails[2] = {1, 0};
    frontier[0] = source;
    Times t;
    t.start();
#pragma omp parallel num_threads(THREAD_NUM)
    {
        GraphBase *graph = graph_engine.create_graph_handle();
        vector<vector<node_id_t>> local_bins(0);
        size_t iter = 0;
        while (shared_indexes[iter & 1] != kMaxBin)
        {
            size_t &curr_bin_index = shared_indexes[iter & 1];
            size_t &next_bin_index = shared_indexes[(iter + 1) & 1];
            size_t &curr_frontier_tail = frontier_tails[iter & 1];
            size_t &next_frontier_tail = frontier_tails[(iter + 1) & 1];
            // #pragma omp for nowait schedule(dynamic, 64)
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
                    // #pragma omp critical
                    next_bin_index = min(next_bin_index, i);
                    break;
                }
            }
#pragma omp barrier
#pragma omp single nowait
            {
                t.stop();
                PrintStep(curr_bin_index, t.t_millis(), curr_frontier_tail);
                t.start();
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
    return dist;
}

int main(int argc, char *argv[])
{
    std::cout << "Running SSSP" << std::endl;
    SSSPOpts sssp_cli(argc, argv, 1);
    if (!sssp_cli.parse_args())
    {
        return -1;
    }

    cmdline_opts opts = sssp_cli.get_parsed_opts();
    opts.stat_log += "/" + opts.db_name;
    std::vector<node_id_t> random_nodes;
    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    graphEngine.calculate_thread_offsets();
    GraphBase *graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_secs() << std::endl;

    if (opts.start_vertex == -1)
    {
        graph->get_random_node_ids(random_nodes, opts.num_trials);
    }
    else
    {
        random_nodes.push_back(opts.start_vertex);
        opts.num_trials = 1;
    }

    node_id_t maxNodeID = graph->get_max_node_id();
    node_id_t num_edges = graph->get_num_edges();
    graph->close();

    long double total_time = 0;
    sssp_info info(0);
    for (int i = 0; i < opts.num_trials; i++)
    {
        t.start();
        DeltaStep(graphEngine,
                  random_nodes.at(i),
                  opts.delta_value,
                  maxNodeID,
                  num_edges);
        t.stop();

        info.time_taken = t.t_secs();
        total_time += info.time_taken;
        std::cout << "Single-Source Shortest Path completed in : "
                  << info.time_taken << std::endl;
        print_csv_info(opts.db_name, info, opts.stat_log);
    }
    std::cout << "Average time taken for " << opts.num_trials
              << " trials: " << total_time / opts.num_trials << std::endl;

    graphEngine.close_graph();
    return 0;
}