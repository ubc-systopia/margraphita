# next_vs_cosine — is the DB fetch the dominant cost?

Supervisor question: per item, does reading from the DB cost much more than
scoring? The ratio decides whether a prefetch design can pay off:

| ratio (fetch / cosine) | meaning |
|---|---|
| < 5 | fetch is cheap; prefetch won't help much |
| 5–20 | prefetch might help |
| > 20 | fetch dominates; prefetch is essential |

Code: `next_vs_cosine.cpp` (built from `benchmark/linkpred/CMakeLists.txt`,
target `next_vs_cosine`, because `add_subdirectory(microbenchmarks)` is commented
out in `benchmark/CMakeLists.txt`).

```bash
next_vs_cosine -p <db_dir> -m <db_name> -g adj -r --props [--dims=64,128] [--out=FILE.csv] [--calls=N] [--rounds=R] [--vecs=V]
```

With `--props` the cosine dim is **probed from the DB** (the embedding loaded
last; each DB holds one at a time), so the like-for-like ratio is fetch of a
D-d blob vs a D-d cosine. `--dims` adds extra cosine dims. For logged runs over
several graphs use `run_next_vs_cosine.sh`; output lands in
`/drives/ml-pytorch-data/linkpred/flexography/microbench/next_vs_cosine/`
(layout in `next_vs_cosine_output_README.md`, copied there as `README.md`).

The table below is the first run (fixed 64 / 128-d cosine, DBs held 16-d).

## What each row measures

| Row | Operation |
|---|---|
| node_cursor_next | one `NodeCursor::next()` (sequential node walk) |
| node_prop_fetch | one `get_node_properties(id)` incl. copy + free, ids in DB order |
| node_prop_fetch_random | same, ids shuffled (random point lookups) |
| in_cursor_next_seq / out_cursor_next_seq | one `InCursor`/`OutCursor::next()` over a full pass, walking the returned neighbour list |
| in_cursor_next_rand / out_cursor_next_rand | `reset` + `set_key_range({id,id})` + `next()` at a random id, walking the neighbour list |
| in/out_cursor_next_rand_hit_pct | % of those reads that returned the requested node (rest: no neighbours on that cursor); not ns |
| in/out_cursor_next_rand_d<L>_<H> | same, only nodes with in/out degree in [L, H] (`_d<L>p` = L and up) |
| cosine_<D>d_seq | one cosine on the DB's **real** embeddings (read once, untimed), one per node, DB order |
| cosine_<D>d_random | same vectors, same shuffle as the random DB ops |
| cosine_<D>d_compute_only | one cosine on the first 4,096 real embeddings, cache-hot: arithmetic cost alone |
| cosine_<D>d_synthetic_* | only for `--dims` extras that differ from the DB's dim: random vectors |
| ratio_<op>/cosine_<D>d_seq or _random | DB vs in-memory: op over the cosine with the same access pattern and data size |
| prefetch_<op>/cosine_<D>d_compute_only | can prefetching hide it? < 1 one scorer outlasts the fetch; > 1 need ~that many fetches in flight |

Like-for-like rows: `db_dim == D`, e.g. `ratio_node_prop_fetch_random/cosine_<D>d_random`
(DB vs memory) and `prefetch_node_prop_fetch_random/cosine_<D>d_compute_only` (prefetch).

Each op: one untimed warm-up pass, then a loop of ≥ `--calls` calls; median of
`--rounds`. Single thread. Cursor rows walk the returned neighbour list (edge
sink), so their cost includes consuming it.

> The results tables below are from **earlier versions** (old row names, and a
> cosine on a fixed cache-hot set of 4,096 vectors, which inflated the ratios on
> big graphs). Rerun with `run_microbench_sweep.sh` and replace them.

## Results (2026-11-03, flexograph-perf container, adj layout)

| Operation | s12_e8 (4,091 nodes) | s20_e8 (1,048,515 nodes) |
|---|---|---|
| single_next | 73.0 | 70.3 |
| single_fetch (DB order) | 137.1 | 131.3 |
| single_fetch_random | 234.0 | **771.1** |
| single_cosine_128d | 92.7 | 90.4 |
| single_cosine_64d | 48.5 | 47.0 |
| ratio (next / cosine_128d) | 0.79 | 0.78 |
| ratio (next / cosine_64d) | 1.50 | 1.49 |
| ratio (fetch / cosine_128d) | 1.48 | 1.45 |
| ratio (fetch / cosine_64d) | 2.83 | 2.79 |
| ratio (fetch_random / cosine_128d) | 2.52 | **8.53** |
| ratio (fetch_random / cosine_64d) | 4.82 | **16.39** |

## Reading

- **Sequential access (cursor `next`, fetch in DB order): ratio < 5 at both
  sizes.** The fetch is not dominant; prefetching a sequential scan won't help
  much. Graph size barely matters because WiredTiger reads ahead along the B-tree.
- **Random access is where it changes: at s20, a random fetch costs ~5.9× a
  sequential one, and the ratio moves into the 5–20 band** (8.5 vs 128-d,
  16.4 vs 64-d). s12 stays < 5 because its whole table sits in cache.
- So prefetching is worth designing for **random-access workloads on graphs
  bigger than cache** (neighbor sampling, FoF / k-hop candidates, walks), not
  for exhaustive sequential scans.

## Caveats (each pushes the true ratio in the direction noted)

- **Stored embeddings are 16-d (64-byte blobs)** in these DBs; cosine is measured
  at 64 / 128-d. A 128-d blob is 512 bytes; the fetch is dominated by the B-tree
  lookup, so expect a modest increase → ratio slightly higher.
- **Cosine is scalar with double accumulators** (`cosine_from_ptrs`). A SIMD /
  float version would be several times faster → ratios several times higher.
- **`libGraphAPI.a` in the release build is compiled with `-pg`**, adding
  `mcount` to every library call → next / fetch slightly inflated.
- **Warm cache, one thread.** Cold reads, larger-than-memory graphs, or many
  threads contending for the cache → fetch slower → ratio higher.
- Fetch includes the `new` + `memcpy` + `delete[]` that `get_node_properties`
  forces; a zero-copy read API would lower it.

## Dimension sweep (2026-11-03, adj, FastRP loaded per dim, median of 3)

Each DB reloaded with FastRP 16 / 32 / 64 / 128 (`add_kron_emb.sh`), then
`run_next_vs_cosine.sh`; restored to 16 afterwards. Raw data:
`/drives/ml-pytorch-data/linkpred/flexography/microbench/next_vs_cosine/`
(`results.csv`, `logs/`, `sweep_20261103.log`). ns/call; ratios are like-for-like
(fetch of a D-d blob vs a D-d cosine).

| graph | dim | next | out_fetch | out_fetch+scan | fetch | fetch_random | cosine_D | fetch/cos | fetch_random/cos |
|---|---|---|---|---|---|---|---|---|---|
| s12 | 16 | 70.3 | 55.9 | 63.0 | 131.4 | 221.2 | 16.1 | 8.1 | 13.7 |
| s16 | 16 | 71.4 | 61.9 | 67.0 | 136.8 | 300.6 | 16.0 | 8.6 | 18.8 |
| s17 | 16 | 71.6 | 63.0 | 70.1 | 136.8 | 333.3 | 16.0 | 8.5 | 20.8 |
| s19 | 16 | 74.0 | 63.8 | 72.4 | 129.8 | 599.1 | 15.7 | 8.3 | **38.2** |
| s20 | 16 | 70.4 | 63.5 | 70.6 | 127.9 | 701.6 | 15.7 | 8.2 | **44.7** |
| s12 | 32 | 69.0 | 54.8 | 62.0 | 129.3 | 222.2 | 26.9 | 4.8 | 8.3 |
| s16 | 32 | 69.6 | 60.2 | 67.5 | 134.7 | 338.9 | 27.1 | 5.0 | 12.5 |
| s17 | 32 | 69.8 | 63.1 | 68.5 | 127.7 | 516.4 | 27.0 | 4.7 | 19.1 |
| s19 | 32 | 69.7 | 65.4 | 73.2 | 130.3 | 702.0 | 26.8 | 4.9 | **26.2** |
| s20 | 32 | 71.2 | 65.5 | 74.1 | 129.5 | 854.5 | 26.5 | 4.9 | **32.3** |
| s12 | 64 | 72.1 | 57.7 | 67.2 | 133.4 | 242.9 | 48.1 | 2.8 | 5.1 |
| s16 | 64 | 70.9 | 61.1 | 72.5 | 130.6 | 490.3 | 50.4 | 2.6 | 9.7 |
| s17 | 64 | 71.9 | 68.4 | 74.8 | 132.1 | 634.8 | 47.8 | 2.8 | 13.3 |
| s19 | 64 | 71.2 | 64.9 | 73.3 | 130.7 | 799.6 | 48.0 | 2.7 | 16.7 |
| s20 | 64 | 73.2 | 66.8 | 74.5 | 128.4 | 1206.5 | 47.2 | 2.7 | **25.5** |
| s12 | 128 | 70.9 | 56.0 | 63.8 | 134.5 | 255.5 | 93.0 | 1.4 | 2.8 |
| s16 | 128 | 73.5 | 66.6 | 71.2 | 151.1 | 687.9 | 94.8 | 1.6 | 7.3 |
| s17 | 128 | 69.3 | 62.4 | 71.3 | 141.1 | 690.2 | 89.1 | 1.6 | 7.7 |
| s19 | 128 | 69.7 | 65.8 | 73.0 | 139.4 | 985.0 | 87.9 | 1.6 | 11.2 |
| s20 | 128 | 70.5 | 64.9 | 71.8 | 147.8 | 1064.7 | 87.4 | 1.7 | 12.2 |

Reading:
- Sequential costs are flat across graph size and dim: next ≈ 70, out_fetch ≈ 55–68,
  scan ≈ 0.4 ns/edge, in-order fetch ≈ 130–150.
- Random fetch grows with graph size (s12 → s20: ~220 → 700–1200 ns) and somewhat
  with dim; cosine grows linearly with dim (16 → 128: ~16 → ~90 ns).
- So the ratio is highest for **small dims on big graphs**: random fetch / cosine
  reaches 38–45 at 16-d (s19–s20), 26–32 at 32-d, 25 at 64-d (s20), 12 at 128-d.
  Sequential fetch / cosine stays < 10 everywhere (< 5 from 32-d up).
- Prefetch matters for random access at scale, most at low dims; it is
  unnecessary for in-order scans.
