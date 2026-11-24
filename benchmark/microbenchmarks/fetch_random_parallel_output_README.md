# fetch_random_parallel — output folder

Copied to `/drives/ml-pytorch-data/linkpred/flexography/microbench/fetch_random_parallel/README.md`.

Question answered: do random embedding fetches get cheaper per fetch when T run
at once (what a prefetcher does)? Single-thread costs live in the sibling
folder `../next_vs_cosine/`. Code: `benchmark/microbenchmarks/fetch_random_parallel.cpp`.

## Layout

```
fetch_random_parallel/
  README.md        this file
  results.csv      every run, appended (long format, one row per value)
  logs/
    <graph>_<adj|split_ekey>_dim<D>[_wt<size>]_<YYYYmmdd_HHMMSS>.log
```

`dim<D>` is the embedding dim the DB held during the run (probed from the
stored blobs); `_wt<size>` appears when the WiredTiger cache was shrunk.

## results.csv columns

| Column | Meaning |
|---|---|
| timestamp, db_dir, db_name | when, which DB |
| nodes, db_dim | node count, embedding dim stored in the DB |
| calls, rounds | total fetches per measurement (split over T threads); median of rounds |
| wt_cache | WiredTiger cache size, or `default` |
| operation, value | see below |

| operation | Meaning |
|---|---|
| cosine_<D>d_compute_only | ns for one cosine on the DB's real embeddings (first 4,096 nodes), cache-hot: a scorer reading a prefetch buffer |
| par<T>_ns_per_fetch | wall-clock ns / total fetches with T threads fetching shuffled ids at once |
| par<T>_speedup | par<first T> ns / par<T> ns |
| par<T>/cosine_<D>d_compute_only | effective fetch / one cosine; < 1 = T fetchers keep up with one scoring thread |

Reading: if `ns_per_fetch` keeps dropping as T grows, parallel fetching scales;
where it flattens, something shared (cache, locks, memory bandwidth) is the limit.

## How a run is produced

1. Load an embedding (`add_kron_emb.sh`, `dim` in `bench_params.json`).
2. `run_fetch_random_parallel.sh [graphs...]` (env: `GT`, `THREADS`, `ROUNDS`, `CALLS`, `WT_CACHE`).

Caveats: warm cache unless `WT_CACHE` is small and the OS page cache is dropped;
`libGraphAPI.a` release build has `-pg`; T = 1 here can differ from
next_vs_cosine's single_fetch_random (OpenMP region, strided ids), so compare
`par<T>` rows with each other.
