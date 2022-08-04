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
 * This runs the Triangle Counting on the graph -- both Trust and Cycle counts
 */

std::vector<node_id_t> intersection_id(std::vector<node_id_t> A,
                                       std::vector<node_id_t> B)
{
    std::sort(A.begin(), A.end(), id_compare);
    std::sort(B.begin(), B.end(), id_compare);
    std::vector<node_id_t> ABintersection;
    std::vector<node_id_t>::iterator A_iter = A.begin();
    std::vector<node_id_t>::iterator B_iter = B.begin();

    while (A_iter != std::end(A) && B_iter != std::end(B))
    {
        if (*A_iter == *B_iter)
        {
            ABintersection.push_back(*A_iter);
            ++A_iter;
            ++B_iter;
        }
        else if (*A_iter < *B_iter)
        {
            ++A_iter;
        }
        else
        {
            ++B_iter;
        }
    }
    return ABintersection;
}

int64_t trust_tc_iter_paralle(const GraphEngine& graph_engine, int thread_num)
{
    int64_t count = 0;

#pragma imp parallel for reduction(+ : count)
    for (int i = 0; i < thread_num; i++)
    {
        GraphBase* graph = graph_engine.create_graph_handle();
        OutCursor* out_cursor = graph->get_outnbd_iter();
        adjlist found = {0};

        out_cursor->set_key_range(graph_engine.get_key_range(i));

        out_cursor->next(&found);
        while (found.node_id != 1)
        {
            std::vector<node_id_t> out_nbrhood = found.edgelist;
            for (node_id_t node : out_nbrhood)
            {
                std::vector<node_id_t> node_out_nbrhood =
                    graph->get_out_nodes_id(node);
                std::vector<node_id_t> intersect =
                    intersection_id(out_nbrhood, node_out_nbrhood);
                count += intersect.size();
            }
            out_cursor->next(&found);
        }
    }

    return count;
}

int64_t cycle_tc_iter_paralle(const GraphEngine& graph_engine, int thread_num)
{
    int64_t count = 0;

#pragma imp parallel for reduction(+ : count)
    for (int i = 0; i < thread_num; i++)
    {
        GraphBase* graph = graph_engine.create_graph_handle();
        InCursor* in_cursor = graph->get_innbd_iter();
        OutCursor* out_cursor = graph->get_outnbd_iter();
        adjlist found = {0};
        adjlist found_out = {0};

        in_cursor->set_key_range(graph_engine.get_key_range(i));
        out_cursor->set_key_range(graph_engine.get_key_range(i));

        in_cursor->next(&found);
        out_cursor->next(&found_out);

        while (found.node_id != 1)
        {
            std::vector<node_id_t> in_nbrhood = found.edgelist;
            std::vector<node_id_t> out_nbrhood = found_out.edgelist;
            for (node_id_t node : out_nbrhood)
            {
                if (found.node_id < node)
                {
                    std::vector<node_id_t> node_out_nbrhood =
                        graph->get_out_nodes_id(node);
                    std::vector<node_id_t> intersect =
                        intersection_id(in_nbrhood, node_out_nbrhood);

                    for (node_id_t itsc : intersect)
                    {
                        if (found.node_id < itsc)
                        {
                            count += 1;
                        }
                    }
                }
            }
            in_cursor->next(&found);
            out_cursor->next(&found_out);
        }
    }

    return count;
}
