#ifndef _THREAD_UTILS_H_
#define _THREAD_UTILS_H_

#include <omp.h>
#include <wiredtiger.h>

#include <cassert>

#include "common.h"
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
                              std::vector<node_id_t> &node_offsets,
                              std::vector<edge_range> &edge_offsets,
                              WT_CURSOR *ecur)
{
    /*
    Steps:
    1. calculate ndoe offsets
    2. use node offsets to find the nearest edge
    3. assign those edges
     */
    // Calculate the node offsets.
    node_id_t node_offset = 0;
    int status;

    for (int i = 0; i < thread_max; i++)
    {
        node_offsets.push_back(node_offset);
        node_offset += num_nodes / thread_max;
    }

    key_pair first, last;

    ecur->reset(ecur);
    // first edge
    ecur->set_key(ecur, 0, -1);
    ecur->search_near(ecur, &status);
    assert(status >= 0);  // There cannot be an edge smaller than (0,-1)
    ecur->get_key(ecur, &first.src_id, &first.dst_id);  // actual first edge.

    for (auto offset : node_offsets)
    {
        ecur->set_key(ecur, offset + 1, -1);  // set cursor to (id+1, -1)
        ecur->search_near(ecur, &status);
        if (status <= 0)
        {
            ecur->get_key(ecur, &last.src_id, &last.dst_id);
        }
        edge_offsets.push_back(edge_range(first, last));

        ecur->search_near(ecur, &status);
    }
}
#endif