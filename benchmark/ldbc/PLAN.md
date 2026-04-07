# benchmark/ldbc — Implementation Plan

## Goal

A single benchmark binary (`fg_ldbc_bench`) that:

1. **Validates** — outputs JSON lines compatible with `validate_results.py` (`--validate`
   flag), replacing `test/validate_queries` as the source of truth for correctness.
2. **Benchmarks** — runs all query variants (point lookups, traversals, BI scans,
   cached OPT-1, parallel BI) using the `bench_harness.h` infrastructure and emits a
   CSV row per variant, suitable for direct comparison with NeuG results from
   `graphdb_compare.md`.

Both modes operate on the golden-image DBs built by `snb_bulk_load` (full SNB
schema: 11 vertex types, 18 edge types). No data loading in this binary.

---

## Files

```
benchmark/ldbc/
├── PLAN.md                  ← this file
├── CMakeLists.txt
├── bench_harness.h          ← timing infrastructure (BenchResult, run_timed, CSV, rss_mb)
├── ldbc_queries.h           ← all query implementations (JSON + timing modes)
└── fg_ldbc_bench.cpp        ← main binary
```

---

## File 1: `bench_harness.h`

Standalone header, no Flexograph dependencies.

### `BenchResult`

```cpp
struct BenchResult {
    double  p50_ms, p95_ms, p99_ms, max_ms;
    double  throughput_ops_per_sec;   // measure_n / total_wall_seconds
    int64_t n_measured;
};
```

### `run_timed`

```cpp
// Call fn() warmup_n times (results discarded), then measure_n times.
// fn() takes no arguments; return value is ignored.
// Uses std::chrono::steady_clock for nanosecond timestamps.
template<typename Fn>
BenchResult run_timed(Fn fn, int warmup_n, int measure_n);
```

Algorithm:
1. Loop `warmup_n` times: call `fn()`.
2. Allocate `std::vector<double> samples(measure_n)`.
3. Loop `measure_n` times: record `t0 = steady_clock::now()`, call `fn()`, store
   `duration_cast<nanoseconds>(now - t0).count() / 1e6` into `samples[i]`.
4. Sort `samples`.
5. Compute:
   - `p50 = samples[measure_n * 50 / 100]`
   - `p95 = samples[measure_n * 95 / 100]`
   - `p99 = samples[measure_n * 99 / 100]`
   - `max = samples.back()`
   - `throughput = measure_n / (sum_of_samples_ms / 1000.0)`

### CSV helpers

```cpp
// Writes header line to `out`.
void bench_csv_header(FILE *out = stdout);

// Writes one data row to `out`.
// experiment  — query name, e.g. "r1", "ic3", "bi1_parallel"
// graph_type  — "adj" | "splitekey"
// mode        — "col" | "emb"
// sf          — scale factor integer label (3, 10, 30, …)
// query_params — free-form string, e.g. "pid=0" or "warmup=50,queries=500"
void bench_csv_row(FILE *out,
                   const char *experiment,
                   const char *graph_type,
                   const char *mode,
                   int sf,
                   const char *query_params,
                   const BenchResult &r);
```

CSV columns:
```
experiment, graph_type, mode, sf, query_params,
p50_ms, p95_ms, p99_ms, max_ms, throughput_ops_per_sec, n_measured
```

### `rss_mb`

```cpp
// Current process RSS in MB.
// Implementation: parse /proc/self/status on Linux (VmRSS field).
// Returns 0 on non-Linux platforms.
size_t rss_mb();
```

---

## File 2: `ldbc_queries.h`

Contains every query implementation used by `fg_ldbc_bench`. Organised in six
sections; included once by `fg_ldbc_bench.cpp`.

### Section A — Helpers

- `jenc(node_id_t)` — JSON-encode a typed vertex ID as `"p<N>"`, `"q<N>"`,
  `"c<N>"`, `"f<N>"`, or `"<N>"` for other types. Used by `run_*` JSON functions.
- `jenc(node_id_t, OutOfBand sentinel)` — variant that encodes
  `OutOfBand_ID_MAX` as `"null"`.
- `is_person(node_id_t)`, `is_post(node_id_t)`, `is_comment(node_id_t)` — inline
  type tests.
- `resolve_person_country(GraphBase &g, node_id_t person)` — walks
  `person → isLocatedIn → city/place → isPartOf → country`, returns `VT_COUNTRY`
  node or `OutOfBand_ID_MAX` if not found.  Full schema version: also handles the
  case where the person is directly in a country (no intermediate city).

### Section B — JSON `run_*` functions (18 total)

Verbatim from `test/validate_queries.cpp` after the fixes applied in this session:

| Function | Source fix applied |
|---|---|
| `run_r1` | none |
| `run_r2` | none |
| `run_r3` | none |
| `run_x1` | none |
| `run_x2` | none |
| `run_x3` | none |
| `run_x5` | none |
| `run_a1` | none |
| `run_a2` | none |
| `run_a3` | none |
| `run_bi1` | EMBEDDED path uses sequential counter scan; COLUMNAR path uses colgroup cursor |
| `run_bi2` | same as bi1 |
| `run_bi12` | EMBEDDED sequential scan |
| `run_ic3` | **forward-only FoF expansion** (bug fixed this session) |
| `run_ic5` | none |
| `run_ic7` | none |
| `run_ic8` | none |
| `run_ic9` | none |

All output JSON to `stdout`. All accept `GraphBase &` and typed vertex ID
parameters. None call `exit()` — control returns to caller after printing.

### Section C — Timing query functions

Adapted from `test/ldbc_snb_queries.cpp`. Each function:
- Takes `GraphBase &graph` and query-specific parameters.
- Takes `double *out_ms = nullptr`; if non-null, stores wall-clock elapsed ms.
- Prints a one-line result summary to `stderr`.
- Returns a result struct or void (function-specific).

No `TIME_START`/`TIME_END` macros — those are private to `ldbc_snb_queries.cpp`.
Timing uses `std::chrono::steady_clock` directly.

Functions included (names match `ldbc_snb_queries.cpp`):

```
r1_person_profile, r2_friends_sorted_by_date, r3_bfs_shortest_path
x1_post_profile, x2_post_author, x3_ic2_friends_recent_posts
x5_count_likes_in_range
a1_degree_count, a2_knows_in_date_range, a3_posts_liked_in_range
ic3_fof_by_country, ic5_forums_by_friend_membership
ic7_message_likes, ic8_latest_replies, ic9_friends_messages_before
bi1_posting_summary, bi2_message_count_two_windows
bi12_message_distribution_fast
```

Write queries (`w1_insert_person`, `w2_insert_knows`, `w3_insert_post_with_creator`,
`x4_insert_likes`) are defined here too, but called only once in the bench binary
(non-repeatable).

### Section D — NeighborCache

Verbatim from `test/ldbc_snb_queries.cpp`:

```cpp
struct NeighborCache {
    // Per-vertex-type CSR arrays for out- and in-neighbors.
    // Indexed by VTYPE_OF(node_id) for the type, VCOUNTER_OF(node_id) for offset.
    struct TypeAdj {
        std::vector<uint32_t>  off;   // size n_nodes+1
        std::vector<node_id_t> nbr;   // flat neighbor list
        const node_id_t* begin(node_id_t id) const noexcept;
        const node_id_t* end  (node_id_t id) const noexcept;
    };
    TypeAdj out_adj[N_VTYPES];
    TypeAdj in_adj[N_VTYPES];
    // Convenience: out_begin/out_end/in_begin/in_end
};

NeighborCache build_neighbor_cache(GraphBase &graph, double *out_ms = nullptr);
```

`build_neighbor_cache` does one sequential `EdgeCursor` scan over all edges, then
two `std::sort` calls per type. Schema-agnostic: works on full 11-type DB.
Memory: ~8 bytes/edge × number_of_edges (both directions stored).

### Section E — OPT-1 cached variants (5 functions)

Verbatim from `test/ldbc_snb_queries.cpp`. Drop-in replacements for the
corresponding Section C functions with WiredTiger topology calls replaced by
`NeighborCache` pointer walks. Property fetches (colgroup cursors) unchanged.

```
x3_ic2_friends_recent_posts_cached
ic7_message_likes_cached
ic9_friends_messages_before_cached
ic5_forums_by_friend_membership_cached
ic3_fof_by_country_cached
```

`ic3_fof_by_country_cached` takes `const NeighborCache &` only (no `GraphBase &`):
all FoF expansion and country resolution use the cache.

### Section F — Parallel BI variants (3 functions, require `<omp.h>`)

Verbatim from `test/ldbc_snb_queries.cpp`:

```cpp
// Partition post_props:temporal colgroup scan across n_threads.
// Each thread opens independent session from shared conn.
void bi1_posting_summary_parallel(WT_CONNECTION *conn,
                                  graph_opts &ro_opts,
                                  node_id_t total_posts,
                                  int n_threads,
                                  double *out_ms = nullptr);

// Two-pass: Pass 1 filters qualifying posts (parallel colgroup scan),
// Pass 2 counts per-creator (parallel OUT_EDGES scan).
void bi12_message_distribution_fast_parallel(WT_CONNECTION *conn,
                                             graph_opts &ro_opts,
                                             node_id_t total_posts,
                                             int n_threads,
                                             int64_t max_date = INT64_MAX,
                                             int32_t min_length = 0,
                                             double *out_ms = nullptr);

// OPT-3 variant: dense vector<uint8_t> qualifying set instead of unordered_set.
void bi12_message_distribution_fast_v2(GraphBase &graph,
                                       node_id_t total_posts,
                                       int64_t max_date = INT64_MAX,
                                       int32_t min_length = 0,
                                       double *out_ms = nullptr);
```

---

## File 3: `fg_ldbc_bench.cpp`

### Command-line interface

```
fg_ldbc_bench <db_dir> [adj|splitekey] [--embedded] [--db-name=NAME]
              [--validate]
              [--warmup N=50]
              [--queries N=500]
              [--warmup-bi N=3]
              [--queries-bi N=10]
              [--threads N=OMP_MAX]
              [--sf N=0]
              [--no-cache]
              [--out FILE]
```

`--validate` is flag-compatible with `validate_results.py`'s `run_fg()` call:
```python
args = [bin_path, db_dir, gtype, f"--db-name={db_name}"]
if embedded: args.append("--embedded")
```
No extra flags are passed, so the binary always enters benchmark mode when called
by the existing validation script unless `--validate` is explicitly added.

To use `fg_ldbc_bench` as the validation binary, update `validate_results.py`:
```python
args = [bin_path, db_dir, gtype, f"--db-name={db_name}", "--validate"]
```

### `--validate` mode

1. Parse args; open DB with `GraphEngine(1, opts)`.
2. Derive `pid0 = MAKE_TYPED_ID(VT_PERSON, 0)`, `post0 = MAKE_TYPED_ID(VT_POST, 0)`,
   `country0 = MAKE_TYPED_ID(VT_COUNTRY, 0)`, `country1 = MAKE_TYPED_ID(VT_COUNTRY, 1)`.
3. Set `has_props = !emb` (COLUMNAR uses colgroup cursors, EMBEDDED uses blob scan).
4. Call all 18 `run_*` functions in the same order as `validate_queries.cpp`:
   `run_r1, run_r2, run_r3, run_x1, run_x2, run_x3, run_x5, run_a1, run_a2,
   run_a3, run_bi1, run_bi12, run_bi2, run_ic3, run_ic5, run_ic7, run_ic8,
   run_ic9` — JSON lines to stdout, timing/debug to stderr.
5. Exit 0.

### Benchmark mode (default)

```
Step 0  Open DB, scan nodes to get per-type counts (same logic as
        ldbc_snb_queries.cpp no-create path). Print counts to stderr.
        Print RSS baseline.

Step 1  Write queries — single timed call each.
        W1, W2, W3, X4 — call once, record wall-clock ms.
        Emit CSV row with n_measured=1, p50=p95=p99=max=elapsed.
        (Non-repeatable: inserts change DB state. Not idempotent.)

Step 2  Read / aggregate / IC queries — run_timed(fn, warmup_n, queries_n).
        Queries: R1, R2, R3, X1, X2, X3, X5, A1, A2, A3, IC3, IC5, IC7, IC8, IC9.
        Lambda captures &graph + fixed params (pid0 / post0).
        Emit CSV row per query.

Step 3  BI queries — run_timed(fn, warmup_bi_n, queries_bi_n).
        Queries: BI-1, BI-2, BI-12-fast.
        Emit CSV row per query.
        (Smaller warmup/queries because each call takes 100–2000 ms.)

Step 4  Create checkpoint: chkpt = engine.make_checkpoint().
        (Required before opening parallel read-only sessions in Step 6.)

Step 5  Build NeighborCache (unless --no-cache).
        Print build time and RSS delta.

Step 6  OPT-1 cached variants — run_timed(fn, warmup_n, queries_n).
        Queries: X3-cached, IC7-cached, IC9-cached, IC5-cached, IC3-cached.
        Lambda uses cache + graph (for property lookups).
        Emit CSV row per query.

Step 7  Parallel BI variants (COLUMNAR mode only, unless --no-cache blocks).
        BI-1-parallel, BI-12-fast-parallel, BI-12-fast-v2.
        Single timed call each (multi-second ops).
        Emit CSV row per variant.

Step 8  Print final RSS and total wall time to stderr.
```

### QueryTimes and CSV output

Each CSV row:
```
experiment,graph_type,mode,sf,query_params,p50_ms,p95_ms,p99_ms,max_ms,throughput_ops_per_sec,n_measured
r1,splitekey,col,3,pid=0,0.011,0.013,0.018,0.021,88235.0,500
ic3,splitekey,col,3,pid=0,2.220,2.350,2.480,2.710,450.1,500
bi1,splitekey,col,3,total_posts=2597141,97.3,98.1,99.2,101.0,10.3,10
bi1_parallel,splitekey,col,3,threads=28,61.2,62.0,63.1,64.0,16.3,1
```

`--out FILE` writes CSV to FILE instead of stdout. Header printed once at the top.

---

## File 4: `benchmark/ldbc/CMakeLists.txt`

```cmake
find_package(OpenMP REQUIRED)

add_executable(fg_ldbc_bench fg_ldbc_bench.cpp)

target_include_directories(fg_ldbc_bench PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test)   # for ldbc_snb_loader.h (used by Section C write queries)

target_link_libraries(fg_ldbc_bench PRIVATE
    ${NAME_LIB}
    ${wt_shared_lib}
    OpenMP::OpenMP_CXX)
```

Add to `benchmark/CMakeLists.txt`:
```cmake
add_subdirectory(ldbc)
```

---

## Validation integration

### Decision: use `fg_ldbc_bench --validate` as the validation binary (Option 1)

`validate_results.py`'s `run_fg()` is updated with a one-line change to append
`"--validate"` when calling `fg_ldbc_bench`:

```python
# validate_results.py — run_fg() change
args = [bin_path, db_dir, gtype, f"--db-name={db_name}", "--validate"]
if embedded:
    args.append("--embedded")
```

This makes `fg_ldbc_bench` the single binary for both validation and benchmarking.
`test/validate_queries.cpp` is retired — it can be deleted or left in place as a
reference, but it is no longer the `--fg-bin` passed to `validate_results.py`.

`ldbc_queries.h` becomes the canonical source of truth for all query logic. There
is no duplication: `fg_ldbc_bench.cpp` includes `ldbc_queries.h` directly.

To confirm 18/18 after implementation:

```bash
conda run -n puneetpy python3 neug/comp_script/validate_results.py \
    --data-dir neug/comp_script/neug-snb-sf3-flat \
    --neug-db  neug/comp_script/neug-snb-sf3-db \
    --fg-bin   flexograph_code/build/benchmark/ldbc/fg_ldbc_bench \
    --fg-adj-col  /drives/flexograph_golden_images/ldbc_sf3_v1/fg-adj-split-db \
    --fg-ekey-col /drives/flexograph_golden_images/ldbc_sf3_v1/fg-splitekey-split-db \
    --fg-adj-emb  /drives/flexograph_golden_images/ldbc_sf3_v1/fg-adj-embedded-db \
    --fg-ekey-emb /drives/flexograph_golden_images/ldbc_sf3_v1/fg-splitekey-embedded-db
```

---

## What changes in existing files

| File | Change |
|---|---|
| `benchmark/CMakeLists.txt` | Add `add_subdirectory(ldbc)` |
| `test/validate_queries.cpp` | **Optional refactor:** replace duplicate query implementations with `#include "../benchmark/ldbc/ldbc_queries.h"` + thin `main()` wrapper. Not required for correctness. |
| `neug/comp_script/validate_results.py` | **Optional:** append `"--validate"` in `run_fg()` to use `fg_ldbc_bench` directly. Not required if `validate_queries` is kept. |

---

## Implementation order

1. `bench_harness.h` — no dependencies, implement and test `run_timed` in isolation
2. `ldbc_queries.h` — merge Section B (copy from validate_queries.cpp), Section C
   (adapt from ldbc_snb_queries.cpp), Sections D–F (copy from ldbc_snb_queries.cpp)
3. `fg_ldbc_bench.cpp` — ties everything together
4. `CMakeLists.txt` + parent wiring
5. Build: `cd flexograph_code/build && make -j$(nproc) fg_ldbc_bench`
6. Validate: run with `--validate` against all four golden DBs → 18/18
7. Benchmark: run default mode → confirm CSV output and no crashes
