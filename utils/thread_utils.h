#ifndef _THREAD_UTILS_H_
#define _THREAD_UTILS_H_

#include <omp.h>

/**
 * @brief We use this function to assign a thread to a specific subset of the
 * node list. This is important for the deterministic run of benchmarks.
 *
 * @param thread_max This should be defined as the maximum number of threads
 * that can be used.
 * @param num_nodes The number of vertices in the graph.
 * @param node_offset_array The array of node offsets.
 */
void calculate_node_offsets(int thread_max,
                            int num_nodes,
                            int node_offset_array[])
{
    int node_offset = 0;
    for (int i = 0; i < thread_max; i++)
    {
        node_offset_array[i] = node_offset;
        node_offset += num_nodes / thread_max;
    }
}

#endif