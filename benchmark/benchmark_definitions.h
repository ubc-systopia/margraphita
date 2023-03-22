#ifndef _BENCHMARK_DEFS
#define _BENCHMARK_DEFS

#include <errno.h>
#include <sys/mman.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <sstream>

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

typedef struct cc_info
{
    int component_count;
    double time_taken;
    cc_info(int _val) : component_count(_val), time_taken(_val){};
} cc_info;

typedef struct iter_info
{
    double time_taken;
    iter_info(int _val) : time_taken(_val){};
} iter_info;

typedef struct sssp_info
{
    double time_taken;
    sssp_info(int _val) : time_taken(_val){};
} sssp_info;

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
    assert(ptr != NULL);
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

std::string generate_timestamp()
{
    auto start = std::chrono::system_clock::now();
    int epoch = std::chrono::duration_cast<std::chrono::seconds>(
                    start.time_since_epoch())
                    .count();

    std::stringstream s;
    s << std::hex << epoch;
    return s.str();
}

// void set_group_id()
// {
//     int gid = atoi(std::getenv("GRAPHS_GROUP_ID"));
//     if (gid == 0)
//     {
//         fprintf(stderr, "GRAPHS_GROUP_ID not set\n");
//         exit(1);
//     }
//     if (setgid(gid) != 0)
//     {
//         perror("setgid failed");
//         exit(1);
//     }
// }

template <typename T>
void get_graph_opts(T &cli, graph_opts &opts)
{
    opts.create_new = cli.is_create_new();
    opts.is_directed = cli.is_directed();
    opts.read_optimize = cli.is_read_optimize();
    opts.is_weighted = cli.is_weighted();
    opts.optimize_create = cli.is_create_optimized();
    opts.db_name = cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = cli.get_db_path();
    opts.conn_config = "cache_size=10GB";  // pr_cli.get_conn_config();
    opts.type = cli.get_graph_type();
}

#endif