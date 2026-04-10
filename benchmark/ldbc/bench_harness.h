// bench_harness.h — LDBC benchmark timing infrastructure
//
// Standalone header: no Flexograph dependencies.
// Provides: BenchResult, run_timed<Fn>, bench_csv_header/row, rss_mb().
//
// Usage:
//   #include "bench_harness.h"
//   auto result = run_timed([&]{ my_query(); }, 50, 500);
//   bench_csv_header();
//   bench_csv_row(stdout, "r1", "splitekey", "col", 3, "pid=0", result);

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <vector>

// ─── BenchResult ──────────────────────────────────────────────────────────────

struct BenchResult {
    double  p50_ms  = 0.0;
    double  p95_ms  = 0.0;
    double  p99_ms  = 0.0;
    double  max_ms  = 0.0;
    double  throughput_ops_per_sec = 0.0;  // measure_n / total_wall_seconds
    int64_t n_measured = 0;
};

// ─── run_timed ────────────────────────────────────────────────────────────────

// Call fn() warmup_n times (results discarded), then measure it measure_n times.
// Returns BenchResult with percentile latencies and throughput.
template<typename Fn>
BenchResult run_timed(Fn fn, int warmup_n, int measure_n)
{
    using Clock = std::chrono::steady_clock;
    using Ns    = std::chrono::nanoseconds;

    // Warmup
    for (int i = 0; i < warmup_n; ++i)
        fn();

    // Measure
    std::vector<double> samples(measure_n);
    for (int i = 0; i < measure_n; ++i) {
        auto t0 = Clock::now();
        fn();
        double ms = std::chrono::duration_cast<Ns>(Clock::now() - t0).count() / 1e6;
        samples[i] = ms;
    }

    std::sort(samples.begin(), samples.end());

    double total_ms = std::accumulate(samples.begin(), samples.end(), 0.0);

    BenchResult r;
    r.n_measured              = measure_n;
    r.p50_ms                  = samples[measure_n * 50 / 100];
    r.p95_ms                  = samples[measure_n * 95 / 100];
    r.p99_ms                  = samples[measure_n * 99 / 100];
    r.max_ms                  = samples.back();
    r.throughput_ops_per_sec  = (total_ms > 0.0) ? (measure_n / (total_ms / 1000.0)) : 0.0;
    return r;
}

// run_timed with a single call (for non-repeatable queries like writes or long BI scans).
// Returns a BenchResult with n_measured=1 and all percentiles set to elapsed_ms.
inline BenchResult run_once(double elapsed_ms)
{
    BenchResult r;
    r.n_measured             = 1;
    r.p50_ms                 = elapsed_ms;
    r.p95_ms                 = elapsed_ms;
    r.p99_ms                 = elapsed_ms;
    r.max_ms                 = elapsed_ms;
    r.throughput_ops_per_sec = (elapsed_ms > 0.0) ? (1000.0 / elapsed_ms) : 0.0;
    return r;
}

// ─── CSV helpers ──────────────────────────────────────────────────────────────

// CSV columns:
//   experiment, graph_type, mode, sf, query_params,
//   p50_ms, p95_ms, p99_ms, max_ms, throughput_ops_per_sec, n_measured
inline void bench_csv_header(FILE *out = stdout)
{
    fprintf(out, "experiment,graph_type,mode,sf,query_params,"
                 "p50_ms,p95_ms,p99_ms,max_ms,throughput_ops_per_sec,n_measured\n");
}

inline void bench_csv_row(FILE *out,
                          const char *experiment,
                          const char *graph_type,
                          const char *mode,
                          int         sf,
                          const char *query_params,
                          const BenchResult &r)
{
    fprintf(out, "%s,%s,%s,%d,%s,%.6f,%.6f,%.6f,%.6f,%.3f,%lld\n",
            experiment, graph_type, mode, sf, query_params,
            r.p50_ms, r.p95_ms, r.p99_ms, r.max_ms,
            r.throughput_ops_per_sec,
            (long long)r.n_measured);
}

// ─── ParamCycle ───────────────────────────────────────────────────────────────

// Stateful cycling iterator over a const vector.
// Each call to next() returns the next element, wrapping around.
// Thread-unsafe — use from a single benchmark thread only.
template<typename T>
struct ParamCycle {
    const std::vector<T>& vec;
    mutable size_t idx = 0;
    explicit ParamCycle(const std::vector<T>& v) : vec(v) {}
    T next() const { return vec[idx++ % vec.size()]; }
    bool   empty() const { return vec.empty(); }
    size_t size()  const { return vec.size(); }
};

// ─── rss_mb ───────────────────────────────────────────────────────────────────

// Current process RSS in MB.
// Parses /proc/self/status on Linux (VmRSS field). Returns 0 elsewhere.
inline size_t rss_mb()
{
#if defined(__linux__)
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    char line[256];
    size_t rss = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            // Format: "VmRSS:    1234 kB"
            unsigned long kb = 0;
            if (sscanf(line + 6, " %lu", &kb) == 1)
                rss = kb / 1024;
            break;
        }
    }
    fclose(f);
    return rss;
#else
    return 0;
#endif
}
