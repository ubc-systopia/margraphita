#include <bins.h>
#include <omp.h>
#include <stdlib.h>

#include <algorithm>
#include <vector>

#include "benchmark_definitions.h"
#include "command_line.h"
#include "common.h"
#include "csv_log.h"
#include "times.h"

#define ITER 10
#define THREADNUM_MAX 16
#define FANOUT_MAX 16384

#define BIN_SIZE 2048
#define BIN_NUM_BUCKETS 16
#define BIN_THREAD_LOCAL_BUFFER_SIZE 256

vector<int> a;
vector<vector<int>> *updates;

void init(int n)
{
    a.resize(n);
    fill(a.begin(), a.end(), 0);
}

void print()
{
    for (int i = 0; i < a.size(); i++)
    {
        printf("%d ", a[i]);
        if ((i + 1) % 10 == 0)
        {
            printf("\n");
        }
    }
}

void verify(int n, int t)
{
    int sum = 0;
    for (int i = 0; i < a.size(); i++)
    {
        sum += a[i];
    }
    printf("%d\n", sum);
    // assert(sum == n*t);
}

void generate_updates(int n, int num_threads, int fanout)
{
    srand(time(NULL));
    int updates_per_thread = n / num_threads * fanout;
    updates =
        new vector<vector<int>>(num_threads, vector<int>(updates_per_thread));

    for (int i = 0; i < num_threads; i++)
    {
        for (int j = 0; j < updates_per_thread; j++)
        {
            (*updates)[i][j] = rand() % n;  // Generate updates
        }
    }
}

void cleanup_updates() { free(updates); }

long double CAS(int n, int num_threads, int fanout)
{
    generate_updates(n, num_threads, fanout);
    int updates_per_thread = n / num_threads * fanout;

    Times t;
    t.start();
    for (int iter = 0; iter < ITER; iter++)
    {
#pragma omp parallel for num_threads(num_threads)
        for (int i = 0; i < num_threads; i++)
        {
            for (int j = 0; j < updates_per_thread; j++)
            {
                bool swapped = false;
                int to_update = (*updates)[i][j];
                while (!swapped)
                {
                    swapped = compare_and_swap(
                        a[to_update], a[to_update], a[to_update] + 1);
                }
            }
        }
    }
    t.stop();
    cleanup_updates();
    return t.t_micros();
}

inline bool try_gather(blaze::Bins *bins)
{
    blaze::Bin *full_bin = bins->get_full_bin();
    if (!full_bin) return false;
    uint64_t *bin = full_bin->get_bin();
    uint64_t bin_size = full_bin->get_size();
    int idx = full_bin->get_idx();
    for (int i = 0; i < idx; i++)
    {
        uint32_t dst = (uint32_t)(bin[i] >> 32);
        uint32_t val = (uint32_t)(bin[i] & 0x00000000ffffffff);
        a[dst] = a[dst] + val;
    }
    full_bin->reset();
    return true;
}

long double bin_method(int n,
                       int num_threads,
                       int num_buckets,
                       int bucket_size,
                       int local_buf_size,
                       int fanout)
{
    omp_set_nested(1);
    int numthread_scatter = num_threads / 2;
    int numthread_gather = num_threads - numthread_scatter;

    int bin_count = numthread_scatter;
    int bin_buf_size = bucket_size;
    blaze::Bins bins(
        n, numthread_scatter, bucket_size, num_buckets, local_buf_size, 0.0);

    generate_updates(n, numthread_scatter, fanout);
    int updates_per_thread = n / numthread_scatter * fanout;

    Times t;
    t.start();
    for (int iter = 0; iter < ITER; iter++)
    {
        std::atomic<bool> done = false;
        int finished_count = 0;
#pragma omp parallel default(none) shared(numthread_scatter,  \
                                          numthread_gather,   \
                                          bins,               \
                                          done,               \
                                          n,                  \
                                          finished_count,     \
                                          updates_per_thread, \
                                          updates) num_threads(2)
#pragma omp single
        {
#pragma omp task
#pragma omp parallel for num_threads(numthread_scatter)
            for (int i = 0; i < numthread_scatter; i++)
            {
                for (int j = 0; j < updates_per_thread; j++)
                {
                    bins.append(i, (*updates)[i][j], 1);
                }
#pragma omp critical
                {
                    bins.flush(i);
                    finished_count++;
                    if (finished_count == numthread_scatter)
                    {
                        bins.flush_all();
                        atomic_store(&done, true);
                    }
                }
            }

#pragma omp task
#pragma omp parallel for num_threads(numthread_gather)
            for (int j = 0; j < numthread_gather; j++)
            {
                bool binning_done = false;
                while (true)
                {
                    bool job_exists = try_gather(&bins);
                    if (binning_done && !job_exists)
                    {
                        break;
                    }
                    if (atomic_load(&done))
                    {
                        binning_done = true;
                    }
                }
            }

#pragma omp taskwait
            {
                bins.reset();
            }
        }
    }
    t.stop();
    cleanup_updates();
    return t.t_micros();
}

int main(int argc, char *argv[])
{
    CmdLineApp cli(argc, argv);
    if (!cli.parse_args())
    {
        return -1;
    }

    for (int threadNum = 2; threadNum <= THREADNUM_MAX; threadNum *= 2)
    {
        int nodeCount = 10000;
        for (int fanout = 16384; fanout <= FANOUT_MAX; fanout *= 2)
        {
            init(nodeCount);
            long double time_bin = bin_method(nodeCount,
                                              threadNum,
                                              BIN_NUM_BUCKETS,
                                              BIN_SIZE,
                                              BIN_THREAD_LOCAL_BUFFER_SIZE,
                                              fanout);
            verify(nodeCount, threadNum);
            printf("%Lf\n", time_bin);

            init(nodeCount);
            long double time_CAS = CAS(nodeCount, threadNum, fanout);
            verify(nodeCount, threadNum);
            printf("%Lf\n", time_CAS);
            microbenchmark_info info(
                time_CAS, time_bin, threadNum, nodeCount, fanout);

            print_csv_info(info, cli.get_logdir());
        }
    }
}
