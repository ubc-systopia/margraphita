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
#include "common.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "standard_graph.h"
#include "times.h"

const edgeweight_t DistInf = INT64_MAX;

pvector<edgeweight_t> sssp(const GraphEngine &graph_engine, node_id_t source)
{
    GraphBase *graph = graph_engine.create_graph_handle();
    pvector<edgeweight_t> oracle_dist(g.num_nodes(), DistInf);
    oracle_dist[source] = 0;
    typedef pair<edgeweight_t, node_id_t> WN;
    priority_queue<WN, vector<WN>, greater<WN>> mq;
    mq.push(make_pair(0, source));
    while (!mq.empty())
    {
        edgeweight_t tent_dist = mq.top().first;
        node_id_t u = mq.top().second;
        mq.pop();
        if (tent_dist == oracle_dist[u])
        {
            for (edge e : g->get_out_edges(u))
            {
                if (tent_dist + e.edge_weight < oracle_dist[e.dst_id])
                {
                    oracle_dist[e.dst_id] = tent_dist + e.edge_weight;
                    mq.push(make_pair(tent_dist + e.edge_weight, e.dst_id));
                }
            }
        }
    }
    graph->close();
    graphEngine.close_graph();
    return oracle_dist;
}