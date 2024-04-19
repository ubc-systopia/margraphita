#include <chrono>
#include <iostream>
#include <list>
#include <queue>
#include <set>
#include <vector>

#include "command_line.h"
#include "common_util.h"
#include "csv_log.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "omp.h"
#include "pvector.h"
#include "times.h"

const int THREAD_NUM = omp_get_max_threads();
const edgeweight_t DistInf = INT32_MAX;
std::vector<node_id_t> random_nodes;

pvector<edgeweight_t> sssp(GraphBase *graph,
                           node_id_t source,
                           node_id_t max_node_id)
{
    pvector<edgeweight_t> oracle_dist(max_node_id, DistInf);
    oracle_dist[source] = 0;
    typedef pair<edgeweight_t, node_id_t> WN;
    std::priority_queue<WN, vector<WN>, greater<>> mq;
    mq.emplace(0, source);
    while (!mq.empty())
    {
        edgeweight_t tent_dist = mq.top().first;
        node_id_t u = mq.top().second;
        mq.pop();
        if (tent_dist == oracle_dist[u])
        {
            for (edge e : graph->get_out_edges(u))
            {
                if (tent_dist + e.edge_weight < oracle_dist[e.dst_id])
                {
                    oracle_dist[e.dst_id] = tent_dist + e.edge_weight;
                    // cerr << e.edge_weight << '\n';
                    mq.emplace(tent_dist + e.edge_weight, e.dst_id);
                }
            }
        }
    }
    return oracle_dist;
}

int main(int argc, char *argv[])
{
    std::cout << "Running SSSP" << std::endl;
    CmdLineApp sssp_cli(argc, argv);
    if (!sssp_cli.parse_args())
    {
        return -1;
    }

    cmdline_opts opts = sssp_cli.get_parsed_opts();
    opts.stat_log += "/" + opts.db_name;

    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    graphEngine.calculate_thread_offsets();
    GraphBase *graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_secs() << std::endl;

    t.start();
    if (opts.start_vertex == -1)
    {
        graph->get_random_node_ids(random_nodes, opts.num_trials);
    }
    else
    {
        random_nodes.push_back(opts.start_vertex);
        opts.num_trials = 1;
    }
    t.stop();
    std::cout << "Random nodes selected in " << t.t_secs() << std::endl;

    t.start();
    node_id_t maxNodeID = graph->get_max_node_id();
    node_id_t num_nodes = graph->get_num_nodes();
    t.stop();
    std::cout << "Max node ID and number of nodes fetched in " << t.t_secs()
              << std::endl;
    for (auto v : random_nodes) std::cout << v << std::endl;

    long double total_time = 0;
    sssp_info info(0);
    for (int i = 0; i < opts.num_trials; i++)
    {
        t.start();
        sssp(graph, random_nodes.at(i), maxNodeID);
        t.stop();

        info.time_taken = t.t_secs();
        total_time += info.time_taken;
        std::cout << "Single-Source Shortest Path completed in : "
                  << info.time_taken << std::endl;
        print_csv_info(opts.db_name, info, opts.stat_log);
    }
    std::cout << "Average time taken for " << opts.num_trials
              << " trials: " << total_time / opts.num_trials << std::endl;

    graph->close(false);
    graphEngine.close_graph();
    return 0;
}