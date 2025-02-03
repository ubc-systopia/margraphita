#ifndef _THREAD_UTILS_H_
#define _THREAD_UTILS_H_

#ifdef MACOSX
#include "/usr/local/opt/libomp/include/omp.h"
#else
#include <omp.h>
#endif

#include <wiredtiger.h>

#include <cassert>

#include "../src/common_util.h"

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
                              edge_id_t num_edges,
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

  for (int i = 0; i < node_ranges.size() - 1; i++)
  {
    key_pair first = {node_ranges[i].start, 0},
             last = {node_ranges[i].end, node_ranges[i].start + 1};
    edge_range er(first, last);
    edge_offsets.push_back(er);
  }
  key_pair first = {node_ranges[node_ranges.size() - 1].start, 0},
           last = {node_ranges[node_ranges.size() - 1].end, num_nodes};
  edge_range er(first, last);
  edge_offsets.push_back(er);
}
#endif