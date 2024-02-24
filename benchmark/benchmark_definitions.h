#ifndef _BENCHMARK_DEFS
#define _BENCHMARK_DEFS

#include <sys/mman.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <sstream>

#include "common_defs.h"

typedef struct bfs_info
{
    int num_visited{};
    int sum_out_deg{};
    long double time_taken{};
} bfs_info;

typedef struct tc_info
{
    size_t cycle_count{};
    size_t trust_count{};
    long double trust_time{};
    long double cycle_time{};
} tc_info;

typedef struct cc_info
{
    int component_count{};
    long double time_taken{};
} cc_info;

typedef struct iter_info
{
    long double time_taken{};
} iter_info;

typedef struct sssp_info
{
    long double time_taken{};
} sssp_info;

typedef struct pr_map
{
    int id{};
    float p_rank[2]{};
} pr_map;

typedef struct pr_iter_map
{
    node_id_t id{};
    degree_t in_deg{};
    degree_t out_deg{};
    float p_rank[2]{};
} pr_iter_map;

std::string generate_timestamp()
{
    auto start = std::chrono::system_clock::now();
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(
                     start.time_since_epoch())
                     .count();

    std::stringstream s;
    s << std::hex << epoch;
    return s.str();
}

template <typename T>
void print_map(const std::string &filename, size_t N, T *pr_map, int p_next)
{
    std::ofstream FILE;
    FILE.open(filename, std ::ios::out | std::ios::trunc);
    for (uint64_t i = 0; i < N; i++)
    {
        FILE << pr_map[i].id << "\t" << pr_map[i].p_rank[p_next] << "\n";
    }
    FILE.close();
}
/***************************
 *
 * PageRank mmap fucntions
 *
 *************************/

template <typename T>
void make_pr_mmap(size_t N, T **ptr)
{
    *ptr = (T *)mmap(nullptr,
                     sizeof(T) * N,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS,
                     -1,
                     0);  // Should I make this file-backed?
    if (ptr == MAP_FAILED)
    {
        perror("mmap failed");
        exit(1);
    }
    assert(ptr != NULL);
    int ret =
        madvise(*ptr,
                sizeof(pr_map) * N,
                MADV_SEQUENTIAL);  // we are guranteed to read this sequentially
    if (ret != 0)
    {
        fprintf(stderr, "madvise failed with error code: %d\n", errno);
        exit(1);
    }
    assert(ptr != NULL);
}
/**
 * @brief This function takes two parameters - the number of nodes in
 * the graph, and a pointer to the mmap region. We then allocate a memory
 * region of size N*sizeof(pr_map) and initialize the memory region with 1/N.
 *
 * @param N
 */
template <typename T>
void init_pr_map(size_t N, T *ptr)
{
    make_pr_mmap(N, &ptr);
    float init_val = 1.0f / static_cast<float>(N);

#pragma omp parallel for
    for (uint64_t i = 0; i < N; i++)
    {
        ptr[i].p_rank[0] = init_val;
        ptr[i].p_rank[1] = 0.0f;
    }
}

template <typename T>
void delete_map(size_t N, T *ptr)
{
    munmap(ptr, sizeof(pr_map) * N);
}
#endif