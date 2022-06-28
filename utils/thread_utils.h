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
                              std::vector<key_range> &node_ranges,
                              std::vector<edge_range> &edge_offsets,
                              GraphType type,
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

    key_pair first = {0, 0}, last = {0, 0};

    for (auto x : node_ranges)
    {
        node_id_t out_of_band = -1;
        if (type != GraphType::EKey)
        {
            ecur->set_key(ecur, x.start - 1, out_of_band);
        }
        else
        {
            ecur->set_key(ecur, x.start, out_of_band);
        }

        int status;
        ecur->search_near(ecur, &status);
        if (status == 0)
        {
            ecur->get_key(ecur, &first.src_id, &first.dst_id);
        }
        else if (status < 0)
        {
            ecur->get_key(ecur, &first.src_id, &first.dst_id);
        }
        else if (status > 0)
        {
            ecur->get_key(ecur, &first.src_id, &first.dst_id);
        }
        edge_range er(first, last);
        edge_offsets.push_back(er);
    }
    // now for all positions from i+1 to thread_max, call prev to set positions
    // i
    for (int i = 1; i < thread_max; i++)
    {
        key_pair key = edge_offsets.at(i).start;
        ecur->set_key(ecur, key.src_id, key.dst_id);
        ecur->search(ecur);
        ecur->prev(ecur);
        ecur->get_key(ecur,
                      &edge_offsets.at(i - 1).end.src_id,
                      &edge_offsets.at(i - 1).end.dst_id);
    }
    // now set the last
    ecur->reset(ecur);
    ecur->prev(ecur);
    ecur->get_key(ecur,
                  &edge_offsets.at(thread_max - 1).end.src_id,
                  &edge_offsets.at(thread_max - 1).end.dst_id);
}
#endif