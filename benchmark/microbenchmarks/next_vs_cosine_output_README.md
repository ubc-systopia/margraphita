# next_vs_cosine — output folder

Copied to `/drives/ml-pytorch-data/linkpred/flexography/microbench/next_vs_cosine/README.md`.

Question answered: per item, is the DB fetch or the cosine score the bigger
cost? (ratio fetch/cosine: < 5 fetch is cheap; 5–20 prefetch might help;
> 20 fetch dominates.) Code and earlier results:
`benchmark/microbenchmarks/next_vs_cosine.{cpp,md}`.

## Layout

```
next_vs_cosine/
  README.md        this file
  results.csv      every run, appended (long format, one row per value)
  logs/
    <graph>_<adj|split_ekey>_dim<D>_<YYYYmmdd_HHMMSS>.log   full stdout/stderr of one run
```

`dim<D>` is the embedding dim the DB actually held during the run (probed
from the stored blobs). Each DB holds one embedding at a time: loading a new
one with `add_kron_emb.sh` overwrites the previous.

## results.csv columns

| Column | Meaning |
|---|---|
| timestamp | when the run finished |
| db_dir, db_name | which DB |
| nodes | node count |
| db_dim | embedding dim stored in the DB (floats per node) |
| blob_bytes | bytes per stored embedding (= 4 × db_dim) |
| calls, rounds, vecs | ≥ calls per op per round; median of rounds; vector count for `_compute_only` |
| wt_cache | WiredTiger cache size (`--wt-cache`), or `default` |
| operation | see below |
| value | ns per call, or a ratio for `ratio_*` rows |

| operation | Meaning |
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

## How a run is produced

1. Load an embedding into the DBs: `add_kron_emb.sh` (set `source` / `dim` in
   `bench_params.json`; loads both adj and split_ekey for each listed graph).
2. `run_next_vs_cosine.sh [graphs...]` (env: `GT`, `DIMS`, `ROUNDS`, `CALLS`).

Caveats: single thread (parallel fetch: `../fetch_random_parallel/`); warm cache
unless `WT_CACHE` is small and the OS page cache is dropped; cosine is scalar
with double accumulators (a SIMD version would raise the ratios);
`libGraphAPI.a` release build has `-pg`.
