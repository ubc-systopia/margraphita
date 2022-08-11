#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <vector>

#include "adj_list.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"

/**
 * Betweeness Centrality - Vertex Centric
 * http://snap.stanford.edu/class/cs224w-readings/brandes01centrality.pdf
 */

std::vector<int64_t> betweeness_centrality(const GraphBase& graph)
{
    std::vector<int64_t> bc = std::vector<node_id_t>(graph.get_num_nodes());
    for (int i = 0; i < graph.get_num_nodes(); i++)
    {
        bc[i] = 0;
    }

    for (node_id_t s = 0; s < graph.get_num_nodes(); s++)
    {
        std::stack<node_id_t> S;
        std::queue<node_id_t> Q;
        std::vector<std::vector<node_id_t>> P =
            std::vector<node_id_t>(graph.get_num_nodes());

        for (int w = 0; w < graph.get_num_nodes(); w++)
        {
            std::vector<node_id_t> empty_list;
            bc[w] = empty_list;
        }
    }
}