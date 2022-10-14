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
#include "standard_graph.h"
#include "times.h"

/**
 * Connected Components for undirected graph
 */

std::vector<node_id_t> connected_components(const GraphBase& graph)
{
    std::vector<node_id_t> labels =
        std::vector<node_id_t>(graph.get_num_nodes());
    int comp_num = 0;
    for (node_id_t i = 0; i < graph.get_num_nodes(); i++)
    {
        labels[i] = -1;
    }

    for (node_id_t v = 0; v < graph.get_num_nodes(); v++)
    {
        if (labels[v] == -1)
        {
            comp_num += 1;
            std::queue<node_id_t> frontier;
            frontier.push(v);
            labels[v] = v;

            while (!frontier.empty())
            {
                node_id_t u = frontier.pop();
                for (node_id_t k : graph.get_out_nodes_id(u))
                {
                    if (labels[k] == -1)
                    {
                        labels[k] = v;
                        frontier.push(k);
                    }
                }
            }
        }
    }

    cout << comp_num << " components counted";
    return labels;
}