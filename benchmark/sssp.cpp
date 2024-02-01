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
#include <queue>
#include <set>
#include <vector>

#include "adj_list.h"
#include "bitmap.h"
#include "command_line.h"
#include "common_util.h"
#include "csv_log.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "standard_graph.h"
#include "times.h"

const edgeweight_t DistInf = INT32_MAX;

pvector<edgeweight_t> sssp(GraphBase *graph, node_id_t source)
{
    pvector<edgeweight_t> oracle_dist(graph->get_num_nodes(), DistInf);
    oracle_dist[source] = 0;
    typedef pair<edgeweight_t, node_id_t> WN;
    std::priority_queue<WN, vector<WN>, greater<WN>> mq;
    mq.push(make_pair(0, source));
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
                    cerr << e.edge_weight << '\n';
                    mq.push(make_pair(tent_dist + e.edge_weight, e.dst_id));
                }
            }
        }
    }
    return oracle_dist;
}

template <typename Graph>
node_id_t find_random_start(Graph &graph)
{
    while (1)
    {
        node random_start = graph->get_random_node();
        std::cout << random_start.id << std ::endl;
        std::cout << "random start vertex is " << random_start.id << std::endl;
        if (graph->get_out_degree(random_start.id) != 0)
        {
            return random_start.id;
        }
        else
        {
            std::cout << "out degree was zero. finding a new random vertex."
                      << std::endl;
        }
    }
}

int main(int argc, char *argv[])
{
    std::cout << "Running SSSP" << std::endl;
    CmdLineApp sssp_cli(argc, argv);
    if (!sssp_cli.parse_args())
    {
        return -1;
    }
    graph_opts opts;
    get_graph_opts(sssp_cli, opts);
    std::string sssp_log = opts.stat_log =
        sssp_cli.get_logdir() + "/" + opts.db_name;

    const int THREAD_NUM = 1;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};
    int num_trials = sssp_cli.get_num_trials();

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    GraphBase *graph = graphEngine.create_graph_handle();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    for (int i = 0; i < num_trials; i++)
    {
        node_id_t start_vertex = sssp_cli.start_vertex();
        if (start_vertex == -1)
        {
            start_vertex = find_random_start(graph);
        }
        sssp_info info(0);

        t.start();
        sssp(graph, start_vertex);
        t.stop();

        info.time_taken = t.t_micros();
        std::cout << "Single-Source Shortest Path completed in : "
                  << info.time_taken << std::endl;
        print_csv_info(opts.db_name, info, sssp_log);
    }

    graph->close();
    graphEngine.close_graph();

    return 0;
}