# Link prediction — walkthrough

How a run flows, top to bottom:
**flags → open DB → (preload) → pick variation → pick runner → generate candidates + score + label → top-k → metrics → output.**

Three engines, picked with `--impl`:
- **direct** (default) — hand-written threaded loops: `linkpred_exhaustive.h`, plus `linkpred_fof.h` / `linkpred_khop.h` for `--candidates=fof|khop`.
- **pipeline** (`linkpred_pipeline.h`) — one threaded runner that plugs in any per-u generator from `linkpred_candidates.h`. Needs `--candidates=...`. Structural (cn / jaccard) only for now.
- **reference** (`linkpred_reference.h`) — plain single-threaded loops, for checking results.

`--variant` picks the data-access pattern within an engine, and `--candidates` picks which pairs are scored. The allowed (impl, family, variant, candidates) combinations are listed in `impl_matrix` in `linkpred_options.h` (also printed by `--help`).

See [0. Methodology](#0-methodology--which-runs-to-compare) before comparing numbers.

## Contents

| # | File | Blurb |
|---|------|-------|
| 0 | [Methodology](#0-methodology--which-runs-to-compare) | → every run path, which give identical results, which timings are fair to compare |
| 1 | [fg_linkpred_bench_db.cpp](#1-fg_linkpred_bench_dbcpp) | → `main`: parse flags, open DB, pick variation, time trials, write output |
| 2 | [linkpred_options.h](#2-linkpred_optionsh) | → link-pred flags and their checks |
| 3 | [linkpred_exhaustive.h](#3-linkpred_exhaustiveh) | → benchmarked hand-coded strategy: score every pair u < v (3 variations) |
| 4 | [linkpred_reference.h](#4-linkpred_referenceh) | → plain single-threaded versions (`--impl=reference`) + `fg_linkpred_bench_reference.cpp`, for cross-checking |
| 5 | [linkpred_scores.h](#5-linkpred_scoresh) | → score math: CN, Jaccard, dot, cosine |
| 6 | [linkpred_topk.h](#6-linkpred_topkh) | → keep the best k pairs, merge threads, tie rule |
| 7 | [linkpred_eval.h](#7-linkpred_evalh) | → P@k and R@k |
| 8 | [linkpred_types.h](#8-linkpred_typesh) | → `RunResult` (what a run returns) |
| 9 | [linkpred_output.h](#9-linkpred_outputh) | → stdout summary + the two CSV writers |
| 10 | [linkpred_candidates.h](#10-linkpred_candidatesh) | → which pairs get scored: per-u generators (for threads) + whole-graph versions |
| 11 | [linkpred_pipeline.h](#11-linkpred_pipelineh) | → threaded runner that plugs in any per-u generator (`--candidates`) |
| 12 | [candidate_generation.cpp](#12-candidate_generationcpp) | → same strategies on a generic graph, for understanding |
| 13 | linkpred_fof.h / linkpred_khop.h | → hand-written baselines for --candidates=fof / khop (--impl=direct, the default) |
| 14 | [tests/](#14-tests) | → unit tests (no DB) and candidate tests (tiny DB) |
| 15 | [run_s12_lp.sh](#15-run_s12_lpsh) | → sweep script over graphs |
| 16 | [CMakeLists.txt](#16-cmakeliststxt) | → build targets |
| 17 | [old/, notes.md](#17-old-notesmd) | → earlier versions, design notes |


## 0. Methodology — which runs to compare

A run is defined by **five choices**. Two runs can only be compared if you know which of these differ.

| Choice | Flag | Changes the pairs scored? | Changes the scores? | Changes timing? |
|---|---|---|---|---|
| Score method | `--method=cn\|jaccard\|cosine\|dot` | no | **yes** | yes |
| Candidate set | `--candidates=` (none = exhaustive) `exhaustive\|fof\|khop` + `--khop=N` | **yes** | no (same pair → same score) | yes |
| Data access | `--variant=` (`all` = cursor for structural, pvector for embedding) | no | no | **yes** |
| Engine | `--impl=direct\|pipeline\|reference` (default direct) | no | no | **yes** |
| Run settings | `--threads`, `--accum`, `--cursor-scope`, `--warmup` (→ `cache=`) | no | no | **yes** |

Rule of thumb: **results** (`pairs_scored`, `true_edges`, top-k, P@k, R@k) depend only on *method + candidate set*. Everything else should only change **time**.

### 0.1 Every run path
The last column is what the run writes in the CSV `variation` column (the `method` column says which score), so every CSV row can be traced back to its row here.

| # | Flags (plus `--method`) | Code that runs | Threads | Candidates | CSV `variation` |
|---|---|---|---|---|---|
| **Exhaustive, structural (cn / jaccard)** |||||
| E1 | *(none)* | `run_multithread_exhaustive_structural` (`linkpred_exhaustive.h`) | N | all u < v | `cursor` |
| E1t | `--variant=tiled [--tile=T]` | `run_multithread_exhaustive_tiled` (`linkpred_exhaustive.h`) | N | all u < v | `tiledT` (`direct_exhaustive_tiledT` with `--candidates=exhaustive`) |
| E1c | `--variant=csr` | `preload_csr` + `run_multithread_exhaustive_structural_preloaded_adj` (`linkpred_exhaustive.h`) | N | all u < v | `csr` (`direct_exhaustive_csr` with `--candidates=exhaustive`) |
| E2 | `--impl=reference` | `run_reference_exhaustive_structural_cursor` (`linkpred_reference.h`) | 1 | all u < v | `cursor_ref` |
| E2b | `--impl=reference --variant=preloaded_ids` | `run_reference_exhaustive_structural_preloaded_ids` | 1 | all u < v | `structural_preloaded_ids_ref` |
| E2c | `--impl=reference --variant=preloaded_adj` | `run_reference_exhaustive_structural_csr` | 1 | all u < v | `structural_preloaded_adj_ref` |
| E2t | `--impl=reference --variant=tiled [--tile=T]` | `run_reference_exhaustive_structural_tiled` | 1 | all u < v | `tiledT_ref` |
| E3 | `--impl=pipeline --candidates=exhaustive` | `run_pipeline_structural` + `ExhaustiveCandidates` | N | all u < v | `pipe_exhaustive` |
| E4 | `... --cursor-scope=block` | same as E3, cursors per block | N | all u < v | `pipe_exhaustive_blk` |
| E6 | `--candidates=exhaustive` | **same code as E1** | N | all u < v | `direct_exhaustive` |
| **Exhaustive, embedding (cosine / dot)** |||||
| M1 | `--variant=cursor` | `run_multithread_exhaustive_embedding_cursor` | N | all u < v | `cursor` |
| M1t | `--variant=tiled [--tile=T]` | `run_multithread_exhaustive_tiled` | N | all u < v | `tiledT` |
| M2 | *(none)* or `--variant=pvector` | `preload_embeddings` + `run_multithread_exhaustive_embedding_pvector` | N | all u < v | `pvector` |
| M3 | `--impl=reference --variant=cursor` | `run_reference_exhaustive_embedding_cursor` | 1 | all u < v | `cursor_ref` |
| M4 | `--impl=reference` (`--variant=pvector`) | `run_reference_exhaustive_embedding_pvector` | 1 | all u < v | `pvector_ref` |
| M5 | `--impl=reference --variant=cursor_cached` | `run_reference_exhaustive_embedding_cursor_cached` | 1 | all u < v | `cursor_cached_ref` |
| M6 | `--impl=reference --variant=preloaded_db` | `run_reference_exhaustive_embedding_preloaded_db` | 1 | all u < v | `preloaded_db_ref` |
| M7 | `--impl=reference --variant=tiled [--tile=T]` | `run_reference_exhaustive_embedding_tiled` | 1 | all u < v | `tiledT_ref` |
| **Friends of friends, structural** |||||
| F1 | `--impl=pipeline --candidates=fof` | `run_pipeline_structural` + `FofCandidates` | N | 2-step walks, v > u | `pipe_fof` |
| F2 | `... --cursor-scope=block` | same as F1, generator per block | N | 2-step walks | `pipe_fof_blk` |
| F3 | `--impl=reference --candidates=fof` | `run_reference_fof_structural_cursor` | 1 | 2-step walks | `fof_ref` |
| F4 | `--candidates=fof` | `run_multithread_fof_structural` (`linkpred_fof.h`) | N | 2-step walks | `direct_fof` |
| **Within k hops, structural** |||||
| K1 | `--impl=pipeline --candidates=khop --khop=K` | `run_pipeline_structural` + `KHopCandidates` | N | dist 1..K, v > u | `pipe_khopK` |
| K2 | `... --cursor-scope=block` | same as K1, generator per block | N | dist 1..K | `pipe_khopK_blk` |
| K3 | `--impl=reference --candidates=khop --khop=K` | `run_reference_khop_structural_cursor_bfs` | 1 | dist 1..K | `khopK_ref` |
| K4 | `--candidates=khop --khop=K` | `run_multithread_khop_structural_bfs` (`linkpred_khop.h`) | N | dist 1..K | `direct_khopK` |

`--accum=shared` adds `_shared` to any threaded row (not to `_ref` rows). Not available: `--candidates` with cosine / dot (the embedding pipelines in `linkpred_pipeline.h` exist but aren't wired in), `--impl=pipeline` without `--candidates`, and fof / khop with a reference variant other than `cursor`. Anything not in `impl_matrix` is rejected with a list of what is supported.

Old flags still work: `--reference` = `--impl=reference`, `--variation=X` and `--ref-variant=X` = `--variant=X` (`structural_preloaded_ids` → `preloaded_ids`, …), `--k=N` = `--khop=N`.

**Benchmarking uses the direct and reference paths only.** The pipeline rows (E3, E4, F1, F2, K1, K2) showed odd behaviour and are not used for benchmarking; they stay in the code for comparison but their timings shouldn't be quoted. The direct rows (E1, E1t, E1c, E6, M1, M1t, M2, F4, K4) are the ones to time; the reference rows (E2*, M3–M7, F3, K3) are the single-threaded cross-checks.

### 0.2 Runs that must give IDENTICAL results
Same method + same candidate set ⇒ same `pairs_scored`, `true_edges`, top-k (pairs, scores, order), P@k, R@k — for any `--threads`, `--accum`, `--cursor-scope`, `--warmup`. If not, one path has a bug.

| Group | Rows | Checked |
|---|---|---|
| Exhaustive structural | E1 = E1t = E1c = E2 = E2t = E2b = E2c = E3 = E4 = E6 | test DB, cn + jaccard; s12 (E1 vs E3; E1 vs E1c, cn + jaccard, top-k identical) |
| FoF structural | F1 = F2 = F3 = F4 | test DB, cn + jaccard |
| k-hop structural (same K) | K1 = K2 = K3 = K4 | test DB, K = 1, 2, 3 |
| Exhaustive embedding | M1 = M1t = … = M7 (s12 cosine: M3–M7 agree in the reference runner) | **not yet verified** (needs a DB with embeddings) — expected equal: same pairs, same float math, assuming every node's embedding has the same length |

**Never identical**, by design: different candidate sets (exhaustive vs fof vs khop, or khop with different K) score different pairs; different methods give different scores.

### 0.3 Fair TIMING comparisons
Only compare `time_ms` when **everything except the thing you're testing** is the same: method, candidate set, graph, `--threads`, `--topk`, and the same `cache=` label (use `--warmup=1` and compare `cache=warm` rows). Run the two sides **interleaved** (A B A B…) in the same session; medians of ≥ 3 trials.

| Question | Compare | Hold equal |
|---|---|---|
| Does the pipeline cost anything? | E1 vs E3 (also F4 vs F1, K4 vs K1) | method, threads, warm |
| Does cursor lifetime matter? | E3 vs E4 (F1 vs F2, K1 vs K2) | everything else |
| False sharing? | any threaded row vs the same row + `--accum=shared` | everything else |
| Work split (steal vs contiguous) | `test_partition_compare` (pipeline, exhaustive) | built in |
| Embedding storage | M1 vs M2 — compare **both** `time_ms` and `total_ms` | method, threads, warm |
| DB vs in-memory adjacency (structural ceiling) | E1 vs E1c — compare **both** `time_ms` and `total_ms` | method, threads, warm |
| **DB with reads amortized vs in-memory** (the fair DB number) | E1t vs E1c, M1t vs M2 | method, threads, warm, `--tile` |
| Thread scaling | the same row at different `--threads` | everything else |

**Not fair to compare:**
- **`--impl=reference` rows (E2*, M3–M6, F3, K3) against threaded rows** — single-threaded, written for readability; for checking results only.
- **Different candidate sets** (E vs F vs K) — different amounts of work. If you must, compare time per pair (`time_ms / pairs_scored`) and P@k, not `time_ms`.
- **Structural vs embedding** — different work per pair.
- **`cache=cold` vs `cache=warm`** rows.
- **`load_ms` across variations** — pvector's includes the embedding preload, csr's the adjacency preload; others only DB open.
- **Runs from different sessions / times of day** — times drift with machine load (seen: the same path 1476 → 1597 ms across rounds).

Also note: the release build is compiled with `-pg` (gprof), which adds a small cost to every non-inlined call in all paths.

---

## 1. fg_linkpred_bench_db.cpp
The entry point. `main`:
1. Parses link-pred flags (`parse_lp_options`, default `--impl=direct`), passes the rest to `CmdLineApp` (`-p`, `-m`, `-g`, `-r`, …). `resolve_variant_all` turns `--variant=all` into the family default.
2. Opens the DB read-only; one handle for `main` (`g`) + one per thread (`g_thread_handles`).
3. For `--variant=pvector`, preloads embeddings (`preload_embeddings`); for `--variant=csr`, preloads the adjacency (`preload_csr`). Time so far = `load_ms`.
4. Runs `--warmup=N` untimed runs, then per trial `run_selected` picks the function and times it (`time_ms`):
   - `--impl=reference` → `linkpred_reference.h` (by `--candidates`, then `--variant`)
   - `--impl=direct` + `--candidates` → `linkpred_fof.h` / `linkpred_khop.h` / `linkpred_exhaustive.h`
   - `--impl=pipeline` → `run_pipeline_structural` + the chosen generator
   - otherwise (direct, no `--candidates`) → structural / pvector / cursor from `linkpred_exhaustive.h`
5. Prints the summary and appends CSV rows (`linkpred_output.h`).

## 2. linkpred_options.h
`Options` struct + `parse_lp_options` → `validate_lp_options` → `adjust_lp_options`. Flags:

| Flag | Field | Meaning |
|---|---|---|
| `--method=cn\|jaccard\|cosine\|dot` | `method` | scoring function (cn/jaccard structural, cosine/dot embedding) |
| `--topk=K` | `topk` | how many best pairs to keep |
| `--impl=direct\|pipeline\|reference` | `impl` | engine (default direct; the reference runner defaults to reference) |
| `--variant=NAME` | `variant` | data access: `all`, `cursor`, `csr`, `preloaded_ids`, `preloaded_adj`, `cursor_cached`, `preloaded_db`, `pvector` (valid names depend on impl + method) |
| `--candidates=exhaustive\|fof\|khop` | `candidates` | structural only; required for pipeline |
| `--khop=N` | `k_hops` | hop count for `--candidates=khop` (default 2); does not turn khop on by itself |
| `--cursor-scope=thread\|block` | `cursor_scope` | pipeline only: cursor lifetime |
| `--threads=N` | `threads` | worker threads (forced to 1 for reference) |
| `--num-trials=N` | `num_trials` | repeat the algorithm N times |
| `--warmup=N` | `warmup` | untimed runs before the trials (→ `cache=warm`) |
| `--accum=local\|shared` | `accum` | where threads keep results; `local` = per-thread stack (no false sharing), `shared` = write per-pair (for A/B) |
| `--source=NAME` | `source` | embedding source label for the CSV |
| `--out=FILE` | `out_csv` | metrics CSV (appended) |
| `--topk-out=FILE` | `topk_out` | top-k pairs CSV (appended) |

Deprecated aliases: `--reference`, `--variation=`, `--ref-variant=`, `--k=` (see 0.1).

Validation checks each value, then looks up (impl, family, variant, candidates) in `impl_matrix`; an unsupported combination prints the supported ones. `method_is_structural(o)` / `method_is_embedding(o)` classify a method.

## 3. linkpred_exhaustive.h
Candidate strategy **exhaustive**: every pair (u, v) with u < v, each once; label = 1 if v is an out-neighbour of u. Holds all three benchmarked variations:

- **structural** (cn / jaccard): reads both neighbour lists per pair.
- **tiled** (`--variant=tiled`, `--tile=T`, default 64; structural and embedding): fixes the read pattern of the cursor paths. Those read `v`'s adjacency / embedding **once per pair**, so each stored list is read ~n/2 times (s12: 8.37M reads of 4,091 lists — "reading the whole DB over and over"). Tiled takes a block of T u's, reads them once, then streams every `v` once and scores it against all u's in the block: each list is read ~n/T times, still straight from the DB, no full preload. Prints `[tiled] v_reads=…` to stderr. Results match E1 / M1 exactly (s12, cn + jaccard + cosine, top-k identical).

  s12, 8 threads, warm (single runs, 2026-11-03, host build):

  | | v reads | time_ms |
  |---|---|---|
  | cn cursor (E1) | 8,366,095 (per pair) | 1898 |
  | cn tiled, T=64 | 132,736 | **121** |
  | cn tiled, T=512 | 18,384 | 239 |
  | cn tiled, T=4096 | 4,090 | 637 |
  | cn csr (in memory) | 0 (preload) | 121 |
  | cosine cursor (M1) | 8,366,095 | 672 |
  | cosine tiled, T=64 | 132,736 | **70** |
  | cosine pvector (in memory) | 0 (preload) | 67 |

  Single-threaded references (s12, 16-d FastRP, container build, single runs; all agree on pairs / true edges / top-k): cn `structural` 6484 ms, `structural_preloaded_ids` 5141, **`structural_tiled64` 693**, `structural_preloaded_adj` 525; cosine `cursor` 2276, `preloaded_db` 1402, `cursor_cached` 865, **`embedding_tiled64` 239**, `pvector` 149. The reference tiled versions stream with two cursors (block of u from cursor a, v from cursor b) and hold only the current block — no node-ID preload, so they are tiled, not tiled + `preloaded_ids`.

  With reads amortized, the DB path is **as fast as the in-memory ceiling** on s12; the old cursor gap (~16× / ~10×) was the redundant reads, not the DB. Bigger T reads less but leaves fewer blocks than threads (s12 at T=4096 is one block, one thread), so T=64 is fastest here; on bigger graphs try T up to n / (threads × a few). Cursor timings (E1, E3, E4, E6, M1, M3, M6) include the redundant reads — don't quote them as "DB cost".

- **structural csr** (`--variant=csr`): adjacency preloaded once into a flat CSR (`preload_csr`: `nodes[]`, `offsets[]`, `edges[]`, each list sorted), then the same block-stealing loop with zero DB calls. The in-memory ceiling for structural scoring — the linkpred equivalent of `benchmark/microbenchmarks/csr_seek_scan.cpp`, and the structural partner of pvector. Blocks are 64 node indices (cursor uses 64-ID ranges); same pair set, so results match E1. Threaded counterpart of the reference `preloaded_adj`.

  First numbers (s12_e8, 8.37M pairs, cn, same session):

  | | cursor (E1) | csr (E1c) | |
  |---|---|---|---|
  | 8 threads, `time_ms` (cold) | 1678 | 88 | ~19× |
  | 1 thread, `time_ms` (warm, 2 trials) | 6028 / 6053 | 675 / 583 | ~9–10× |
  | `load_ms` (DB open + preload) | ~1250–1450 | ~1080–1380 | preload is small at s12 (4,091 nodes, 64,894 adjacency entries) |

  Single runs, not interleaved medians — redo per 0.3 before quoting.
- **embedding cursor** (cosine / dot): reads v's embedding from the DB per pair.
- **embedding pvector**: embeddings preloaded once (`preload_embeddings`), scored from memory.

Threads take 64-node blocks via block stealing (`atomic<size_t> next_block.fetch_add(1)`). Each thread gets its own handle from `g_thread_handles[tid]`. `--accum=local|shared` picks where threads keep results. Defines `g_thread_handles`, which the pipeline also uses.

## 4. linkpred_reference.h
One simple nested loop per variant — no blocks, no OpenMP. Named `run_reference_<candidates>_<scoring>`: exhaustive (structural, structural_preloaded_ids, structural_preloaded_adj, four embedding variants), plus `run_reference_fof_structural_cursor` and `run_reference_khop_structural_cursor_bfs`. Run with `--impl=reference` (forces 1 thread, CSV variation `*_ref`). Same `pairs_scored`, `true_edges`, top-k as the threaded path → use it to check correctness. Not for timing.

`fg_linkpred_bench_reference` runs every applicable reference variant on one graph and checks they agree (exit 1 on mismatch). `--variant=X` runs just one (for profiling; `runners/run_perf_ref.sh` does this per variant). With `--candidates=fof|khop` it runs only that reference.

## 5. linkpred_scores.h
Pure math, no DB:

| Function | Inputs | Returns |
|---|---|---|
| `common_neighbors(Nu, Nv)` | two sorted neighbour lists | count of shared nodes |
| `jaccard(Nu, Nv)` | two sorted neighbour lists | CN / union size |
| `score_pair(method, Nu, Nv)` | method name + two lists | cn or jaccard |
| `dot(eu, ev)` | two `vector<float>` embeddings | dot product |
| `cosine(eu, ev)` | two `vector<float>` embeddings | cosine similarity |
| `dot_from_bytes`, `cosine_from_bytes` | raw DB blobs | same, from bytes |

Larger = more likely a link.

## 6. linkpred_topk.h
- `ScoredPair` = `{u, v, score, label}` — one scored candidate.
- `ranks_higher(a, b)` — higher score wins; ties → smaller (u, v). Makes multi-threaded results repeatable.
- `TopK(k)` — bounded heap keeping the best k. `offer(ScoredPair)` inserts if it beats the current min; O(log k) when it does, O(1) when it doesn't.
- `merge_topk(per_thread_heaps, k)` — combine per-thread heaps into one.

## 7. linkpred_eval.h
`precision_at_k(top, all_true_count)` — true links in top-k ÷ k. `recall_at_k(top, all_true_count)` — true links in top-k ÷ all true links seen.

## 8. linkpred_types.h
`RunResult`: `pairs_scored` (pairs visited), `true_edges` (edges among them), `top_k` (the top-k `ScoredPair`s).

## 9. linkpred_output.h
- `print_result(o, r, time_ms, load_ms, total_ms)` — the `LinkPredDB ...` line + top-k list on stdout.
- `append_result_csv(file, o, ...)` — one metrics row per trial.
- `append_topk_csv(file, o, ...)` — one row per top-k pair.

Marks `reference=`, `impl=`, `cursor_scope=`, `cache=` and `accum=shared` so every row records which configuration produced it. `variation_label` builds the CSV `variation` value (see 0.1).

## 10. linkpred_candidates.h
**Candidate generation** = "which pairs (u, v) get scored?" This file answers that on the Flexograph DB using cursors, separately from scoring and threading. The benchmark uses it through the pipeline (`--impl=pipeline --candidates=...`).

Two parts.

### Part A — per-u generators (for threaded runners)
Answer "which v's for **this** u?". The runner owns the loop over u and gives each thread its own block of u's.

| Struct | Candidates for u | Built from |
|---|---|---|
| `ExhaustiveCandidates(g)` | every node v > u | inner cursor loop of `structural_worker_cursor` |
| `FofCandidates(g, skip_edges=false)` | v reached by u → w → v (friends of friends) | `friends_of_friends_each`, for one u |
| `KHopCandidates(g, k, skip_edges=false)` | v within k hops (BFS to depth k) | `distance_bounded_candidates_each`, for one u |

Parameters:

- `GraphBase* g` — **this thread's** DB handle (`g_thread_handles[tid]`). Cursors aren't thread-safe, so each thread builds its own generator on its own handle. Never pass the main `g` here from a worker.
- `bool skip_edges` (default **false** here) — drop direct neighbours (existing edges) when true. Default **false** so the runner sees real edges — they're the ground-truth labels. Passing `true` gives non-edge candidates only (the "People You May Know" set).
- `int k` (KHopCandidates only) — max hops from u. With `skip_edges=false`, emits dist 1..k; with `skip_edges=true`, dist 2..k (skips the direct neighbours).
- `emit(node_id_t v)` — the callback. Called once per candidate v. Returns bool; `false` stops the current u early (the runner's outer u loop continues).

Usage:
```cpp
ExhaustiveCandidates cands(my_g);              // one per thread, on that thread's handle
cands.for_each(u, [&](node_id_t v) {           // lazy: called once per candidate v
    /* score, label, offer to top-k */
    return true;                               // false = stop this u early
});

```





## 14. tests/

- `test_linkpred_common.cpp`: scores, top-k, ties, merge, P@k / R@k. No DB.
- `test_linkpred_candidates.cpp`: builds a 7-node graph in `./lp_cand_db` and checks each candidate strategy.
- Planned: `test_linkpred_pipeline.cpp` — small-DB equivalence tests against the hand-coded paths.

### test_partition_compare.cpp — one-off A/B: block stealing vs contiguous

A throwaway comparison, not a regression test. Runs structural exhaustive
(`ExhaustiveCandidates`) through both partition strategies in `linkpred_pipeline.h`:

| Strategy | Function | How u's are split across threads |
|---|---|---|
| steal | `run_pipeline_structural` | threads grab the next 64-node block on demand (`atomic fetch_add`) |
| contiguous | `run_pipeline_structural_contiguous` | each thread gets one fixed run of blocks up front |

Both call the same `pipeline_structural_block`, so only the split differs. The test
runs each once untimed (warm-up), then times each, and exits 1 if `pairs_scored`,
`true_edges` or any top-k row differ.

**Build:** `make test_partition_compare`
**Run:**
```bash
./benchmark/linkpred/test_partition_compare -p /drives/ssd2/flexograph_rd/kron/s12_e8 -m adj_r_s12_e8 -g adj -r --method=cn --threads=20 --topk=10
```

**Result (s12_e8, cn, 20 threads) — passed:**
```
strategy            pairs   true_edges    time_ms
steal             8366095        32447   1463.118
contiguous        8366095        32447   1739.790
speedup                                      1.19x
OK — both partitions produced identical metrics and top-k.
```
- **Correct:** both strategies scored the same pairs, found the same true edges and kept the same top-k.
- **Block stealing is ~19% faster.** Exhaustive work is triangular (small u has many more v's
  than large u), so fixed ranges leave the threads with the low IDs doing most of the work.
  Stealing keeps all threads busy until the end.
- **Takeaway:** block stealing is the better default; contiguous is worth keeping only as this comparison.
- One run on one graph; larger graphs (s16+) or hub-heavy k-hop would likely widen the gap.










----- reference stuff that can be incorperated into the steps.md over time 

structural
  Startup:  open 2 cursors                              (~us)
  Memory:   O(deg) per thread                           (~KB)
  Per pair: 65 ns cursor + 200 ns DB + 100 ns CPU       = 365 ns
  Total:    8.37M * 365 ns                              = ~3.0 s (measured 5.9 s, rest is
                                                          startup, tail effects, thread overhead)

structural_preloaded_ids
  Startup:  open cursor, walk all nodes                 (~0.3 ms)
            fill nodes[] (4091 * 8 B = 32 KB)
  Memory:   O(n) node IDs + O(deg) current pair         (~32 KB)
  Per pair: 1 ns index + 200 ns DB + 100 ns CPU         = 301 ns
  Total:    8.37M * 301 ns + 0.3 ms                     = ~2.5 s (measured 5.3 s)

structural_preloaded_adj
  Startup:  open cursor, walk all nodes                 (~1.2 ms)
            for each node: get_out_nodes_id + sort
            fill nodes[] + adj[]
  Memory:   O(n + m) adjacency                          (~520 KB on s12_e8)
  Per pair: 2 ns index + 0 ns DB + 100 ns CPU           = 102 ns
  Total:    8.37M * 102 ns + 1.2 ms                     = ~0.85 s (measured ~1.5 s)



  ---- this should be added to represent the variations implemented for ease 
  impl       | family      | variant      | candidates   | function
-----------|-------------|--------------|--------------|----------
direct     | structural  | cursor       | ""           | run_multithread_exhaustive_structural
direct     | structural  | cursor       | exhaustive   | run_multithread_exhaustive_structural
direct     | structural  | cursor       | fof          | run_multithread_fof_structural
direct     | structural  | cursor       | khop(k)      | run_multithread_khop_structural_bfs
direct     | embedding   | cursor       | ""           | run_multithread_exhaustive_embedding_cursor
direct     | embedding   | pvector      | ""           | run_multithread_exhaustive_embedding_pvector
pipeline   | structural  | cursor       | exhaustive   | run_pipeline_structural + ExhaustiveCandidates
pipeline   | structural  | cursor       | fof          | run_pipeline_structural + FofCandidates
pipeline   | structural  | cursor       | khop(k)      | run_pipeline_structural + KHopCandidates
pipeline   | embedding   | cursor       | ""           | run_pipeline_embedding_cursor + ExhaustiveCandidates
pipeline   | embedding   | pvector      | ""           | run_pipeline_embedding_pvector + ExhaustiveCandidates
reference  | structural  | cursor       | ""           | run_reference_exhaustive_structural_cursor
reference  | structural  | preloaded_ids | ""         | run_reference_exhaustive_structural_preloaded_ids
reference  | structural  | preloaded_adj | ""         | run_reference_exhaustive_structural_csr
reference  | embedding   | cursor       | ""           | run_reference_exhaustive_embedding_cursor
reference  | embedding   | cursor_cached | ""          | run_reference_exhaustive_embedding_cursor_cached
reference  | embedding   | preloaded_db | ""           | run_reference_exhaustive_embedding_preloaded_db
reference  | embedding   | pvector      | ""           | run_reference_exhaustive_embedding_pvector
## FoF candidate fraction and wedge counting (2026-11-03)

**What wedge counting means.** A *wedge* is a two-step path u→w→v; w is a neighbour of both u and v.
On these (symmetric) kron graphs CN(u, v) = |N(u) ∩ N(v)| is exactly the number of wedges from u to v, so it can be counted while walking from u
instead of computed by reading N(v) and intersecting:

```
for w in N(u):            # read N(u) once
    for v in N(w):        # read each N(w) once
        if v > u: cnt[v] += 1      # one wedge u→w→v found
# now cnt[v] = CN(u, v) for every FoF candidate v; N(v) was never read
```

- Plain FoF does the same walk to find candidates, then reads N(v) per candidate and intersects (≈ one random
  adjacency read per pair: ~1B on s20). Wedge FoF drops that step; same candidates, same scores.
- Jaccard = cnt[v] / (|N(u)| + deg(v) − cnt[v]); deg(v) comes from one NodeCursor pass before scoring.
- Cost: a per-thread `cnt` array of n × 4 bytes plus a `touched` list to reset it.
- Structural only. Cosine(u, v) is computed from v's vector, so embeddings always need a fetch of v.
- Standard technique (wedge / triangle counting, sparse A·A); the result here is what it saves on the DB.

`SET=fof runners/run_lp_all.sh`, direct 20 threads (medians of 3 rounds; s19/s20 one round).
`fof` reads N(v) for every candidate; `fof_wedge` (M7) counts CN along u→w→v and never reads N(v).
Pairs, true edges and P@10 are identical between fof and fof_wedge on every graph.

| graph | FoF pairs | % of all pairs | true edges in FoF (exhaustive) | cn fof | cn fof_wedge | speedup | exhaustive csr / tiled64 |
|---|---|---|---|---|---|---|---|
| s12 | 1.06M | 12.7% | 16,243 (32,447) | 248 ms | 25 ms | 9.9× | 48 / 61 ms |
| s16 | 36.2M | 1.69% | 113,942 (523,456) | 10.3 s | 0.44 s | 23.7× | 10.5 / 14.9 s |
| s17 | 84.3M | 0.98% | 177,346 | 23.8 s | 0.90 s | 26.6× | – |
| s19 | 436M | 0.32% | 403,609 | 152.7 s | 4.99 s | 30.6× | – |
| s20 | 992.5M | 0.18% | 609,094 | 405.4 s | 11.4 s | 35.5× | – |

Jaccard times match cn within ~5%. Reference (1 thread) cn: s16 50.0 → 5.3 s, s17 130.9 → 12.5 s.
Notes: FoF is only 2-hop pairs, so its true-edge count is a recall ceiling (50% of s12 edges, 22% of s16).
Plain FoF is already no faster than exhaustive csr on s16 — the N(v) reads dominate; wedge counting is what makes FoF pay off.

## Embedding over FoF / k-hop candidates (2026-11-03)

`--candidates=fof|khop --method=cosine|dot` (direct: `run_multithread_fof_embedding` / `_khop`
in linkpred_fof.h / linkpred_khop.h; reference: `run_reference_fof_embedding_cursor` / `_khop_embedding`).
Same walk / BFS as the structural path; the scoring line reads v's embedding
(`get_node_properties(v)`) instead of N(v). No counted (wedge) version: cosine needs v's vector.
`--variant=all` resolves to cursor for these (pvector is exhaustive only).

s12, 20 threads, one run each (direct and reference agree on pairs, true edges and top-k):

| candidates | pairs | existing edges inside | cosine direct | cosine ref (1 thread) | P@10 |
|---|---|---|---|---|---|
| fof | 1,061,287 | 16,243 | 124 ms | 361 ms | 0 |
| khop2 | 1,077,491 | 32,447 | 117 ms | 442 ms | 0.4 |

For comparison on s12: exhaustive cosine tiled64 23 ms, pvector 12 ms; CN FoF + lookup 267 ms.
Not run yet: s16+ (`SET=fof runners/run_lp_all.sh` now includes `cosine direct fof`).
