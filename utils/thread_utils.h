#ifndef _THREAD_UTILS_H_
#define _THREAD_UTILS_H_

#include <omp.h>
#include <wiredtiger.h>

#include <cassert>

#include "../src/common.h"

/**
 * @brief We use this function to assign a thread to a specific subset of the
 * node list. This is important for the deterministic run of benchmarks.
 *
 * @param thread_max This should be defined as the maximum number of threads
 * that can be used.
 * @param num_nodes The number of vertices in the graph.
 * @param node_offset_array The array of node offsets.
 */
void calculate_thread_offsets(int thread_max,
                              node_id_t num_nodes,
                              node_id_t num_edges,
                              std::vector<key_range> &node_ranges,
                              std::vector<edge_range> &edge_offsets,
                              GraphType type)
{
    /*
    Steps:
    1. calculate ndoe offsets
    2. use node offsets to find the nearest edge
    3. assign those edges
     */
    // Calculate the node offsets.
    node_id_t node_offset = 0;
    node_ranges.clear();
    edge_offsets.clear();

    for (int i = 0; i < thread_max; i++)
    {
        key_range temp;
        if (i == thread_max - 1)
        {
            temp = {node_offset, num_nodes};
        }
        else
        {
            temp = {node_offset, (node_offset + (num_nodes / thread_max) - 1)};
        }

        node_ranges.push_back(temp);
        node_offset += num_nodes / thread_max;
    }

    for (auto x : node_ranges)
    {
        key_pair first = {x.start, 0}, last = {x.end + 1, -1};
        edge_range er(first, last);
        edge_offsets.push_back(er);
    }
}
#endif