#ifndef _BENCHMARK_DEFS
#define _BENCHMARK_DEFS

#include <sys/mman.h>

#include <cassert>
#include <cstdint>

typedef struct bfs_info
{
    int num_visited;
    int sum_out_deg;
    double time_taken;
    bfs_info(int _val)
        : num_visited(_val), sum_out_deg(_val), time_taken(_val){};
} bfs_info;

typedef struct tc_info
{
    int64_t cycle_count;
    int64_t trust_count;
    double trust_time;
    double cycle_time;
    tc_info(int _val)
        : cycle_count(_val),
          trust_count(_val),
          trust_time(_val),
          cycle_time(_val){};
} tc_info;

typedef struct wcc_lp_info
{
    int iterations;
    int64_t components;
    double time_taken;
    wcc_lp_info(int _val)
        : iterations(_val), components(_val), time_taken(_val){};
} wcc_lp_info;

typedef struct pr_map
{
    int id;
    float p_rank[2];
} pr_map;

typedef struct pr_iter_map
{
    int id;
    int in_deg;
    int out_deg;
    float p_rank[2];
} pr_iter_map;

template <typename T>
void make_pr_mmap(int N, T **ptr)
{
    *ptr = (T *)mmap(NULL,
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
    int ret =
        madvise(*ptr,
                sizeof(pr_map) * N,
                MADV_SEQUENTIAL);  // we are guranteed to read this sequentially
    if (ret != 0)
    {
        fprintf(
            stderr, "madvise failed with error code: %s\n", strerror(errno));
        exit(1);
    }
    assert(ptr != NULL);
}
#endif