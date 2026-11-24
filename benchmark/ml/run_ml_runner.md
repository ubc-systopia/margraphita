# Modular ML runner

Copy the three files into `benchmark/ml/` without replacing scripts. Add `add_subdirectory(ml)` once to `benchmark/CMakeLists.txt` (not the root CMakeLists). The existing GraphAPI target must already include ml_access.cpp and fastrp.cpp.

From margraphita:

```bash
cmake --build build/release --target fg_ml_runner -j4
```

Use actual DB_DIR and DB_NAME values. Start with a small undirected scratch DB.

```bash
./build/release/benchmark/ml/fg_ml_runner -p "$DB_DIR" -m "$DB_NAME" -g adj -r \
  --embedding=fastrp --allow-scratch-writes --write-db=after \
  --dim=128 --iters=4 --threads=4 --task=linkpred --candidates=fof \
  --method=cosine --batch-size=256 --max-pairs=10000 --out=generated.csv

./build/release/benchmark/ml/fg_ml_runner -p "$DB_DIR" -m "$DB_NAME" -g adj -r \
  --embedding=stored --dim=128 --task=linkpred --candidates=fof \
  --method=cosine --batch-size=256 --max-pairs=10000 --out=stored.csv

diff generated.csv stored.csv
```

Add `-d` for directed databases. Generation overwrites vertex properties; use disposable copies only. Stored mode opens read-only and assumes all requested properties contain raw Float32 embeddings of the specified dimension.

Options: `--candidates=fof|khop|uniform|exhaustive`, `--hops=2`, `--draws=10000`, `--seed=42`, `--method=cosine|dot`, `--retrieval=direct|sorted|dedup`, `--batch-size=256` (pairs, two vectors per pair), `--top-k=100`, `--max-pairs=0` (unlimited), `--out=ml_predictions.csv`. FastRP defaults to 128 dimensions, 4 iterations, 4 workers, cursor adjacency, original variant, density .01; `--write-db=after|fused`. Threads applies to FastRP only; prediction is serial in this first version.

Existing candidate generator semantics are preserved: FoF/k-hop/exhaustive use u<v even for directed graphs, uniform draws can repeat pairs and skip missing IDs using has_node (including the known directed sink limitation). Existing edges are retained and labeled using has_edge. This is a reconstruction workload, not held-out accuracy evaluation. Max-pairs caps scoring, not all candidate-generation work. CSV contains global top-K, not all scores. Logs separate retrieval, scoring/label/ranking and workload time. No external NumPy backend yet.

Built and run on s12 (2026-11-24). Verify generated/stored CSV equality, nonzero scored count, and unchanged topology on scratch DB before larger runs.

## Direct implementation integration

Use `--impl=direct --variant=cursor --direction=out` with either embedding mode. Existing FoF, k-hop and exhaustive specialized functions are called unchanged with one checkpoint handle per worker. K-hop also supports `--variant=outcursor_nested --hops=2`; exhaustive supports `--variant=tiled`. Default remains pipeline. Direct uses legacy property reads, not ML batches. Batch size and dimension do not control direct scoring. Direct rejects `--max-pairs`, uniform and non-default retrieval strategies. Use small scratch graphs first; exhaustive is quadratic. Compare full pipeline and direct runs with matching out direction and candidate semantics, not capped runs. Directed labels and candidate sets may differ; investigate rather than assume equality. Built and run on s12: FoF 1,061,287 pairs, same as the pipeline.

## Fetch counters

`--count-fetches` (pipeline only) adds a line after the run: `batch_size`, `batches`, `requested_vectors` (2 per pair),
`unique_vectors_in_batches` (distinct IDs summed per batch) and `node_props_searches` (WiredTiger `cursor: search calls` on
`table:node_props`, from per-table statistics; the flag adds `statistics=(fast)` to the connection config). Counting is done
outside the timed retrieval. Without the flag, output and timing are unchanged. s12 FoF, batch 64: direct and sorted do 2,122,574
searches, dedup 1,080,294.

## Source vertex selection (pipeline only)

| option | default | meaning |
|---|---|---|
| `--source-selection=all\|uniform` | `all` | `all`: every vertex is a source, unchanged code path (generators get `sources=nullptr`). `uniform`: reservoir sample of node IDs (`linkpred::sample_source_vertices`, sorted) |
| `--source-count=N` | – | required with `uniform`; more than the vertex count uses all vertices (warning) |
| `--sampling-seed=S` | `42` | seed of the source sample (separate from `--seed`, which is FastRP and uniform pairs) |
| `--source-out=FILE` | – | write the selected source IDs, one per line |

Applies to `fof`, `khop`, `exhaustive`; rejected for `--candidates=uniform` and `--impl=direct`.
Logs: `source_selection=…`, `sources_selected`, `min_source`/`max_source`, `sampling_s` (not part of `workload_s`),
and for every run `sources_with_candidates` and `pairs_per_active_source`.

Sampled sources explore their whole neighbourhood: a source u gets every candidate v, including v < u (the all-source
path keeps only v > u). On undirected graphs a pair whose two ends are both sampled would then appear twice; the generators
(`linkpred::ReverseFilter` in `linkpred_candidates.h`) skip (u,v) when v is also a selected source and v < u, so it is kept once,
as (v,u). This uses the set of selected IDs, not a set of pairs. It is valid because FoF, k-hop and exhaustive are symmetric on
undirected graphs (u→w→v iff v→w→u; dist(u,v)=dist(v,u)). On directed DBs (`-d`) the filter is off: (u,v) and (v,u) are
different targets and both are kept. The log reports `reverse_duplicates_skipped` and `reverse_filter=undirected|off_directed`.

s12, 100 sources, seed 7: FoF 44,322 pairs (479 skipped), 2-hop 45,002 (484), 3-hop 276,511 (3,351), exhaustive 404,050 (4,950).

```bash
./build/release/benchmark/ml/fg_ml_runner -p "$DB_DIR" -m "$DB_NAME" -g adj -r --embedding=stored \
  --candidates=fof --source-selection=uniform --source-count=100 --sampling-seed=7 \
  --source-out=sources.txt --count-fetches --out=sampled.csv
```

Validation: `scripts/validate_source_selection.sh [DB_DIR] [DB_NAME] [COUNT] [SEED]` (inside the container; defaults: s12
scratch copy, 100, 7, plus the directed web-Stanford scratch copy). It rebuilds the edge set from the DB (`khop --hops=1`),
computes the reference FoF / 2-hop / 3-hop / exhaustive sets in Python and checks: no unordered pair lost or duplicated,
exhaustive = k(n-1) - k(k-1)/2, selecting every vertex reproduces the all-source set, and the directed run keeps ordered pairs.
Passed on s12 2026-11-24 (11/11). Default and `all` runs are byte-identical to before (1,061,287 pairs, 16,243 positives).

## Structural prediction

`--prediction=structural|embedding` (default `embedding`, unchanged). Structural methods: `--method=cn|jaccard|cosine`
(cosine = CN / sqrt(|N(u)| |N(v)|), out-neighbours). CN and Jaccard reuse `linkpred::score_pair`; cosine reuses
`linkpred::common_neighbors`. Structural mode opens the DB without node properties: no FastRP, no stored embeddings,
no `node_props` table needed. It rejects `--embedding=fastrp`, `--count-fetches` and `--retrieval` other than direct.
In the log, `retrieval_s` is neighbour-list retrieval (two `get_out_nodes_id` + sort per pair) and
`scoring_label_ranking_s` is scoring, `has_edge` label and top-K.

| | FoF | k-hop | exhaustive | uniform pairs |
|---|---|---|---|---|
| pipeline cn / jaccard / cosine | yes | yes | yes | yes |
| `--impl=direct` cn / jaccard | yes (`run_direct_*_structural_*`) | yes | yes | no (full workload only) |
| `--impl=direct` cosine | rejected: the direct functions' `score_pair` treats every non-`cn` method as Jaccard | | | |

Source selection and the undirected reverse-duplicate filter apply to structural runs exactly as to embedding runs.

```bash
./build/release/benchmark/ml/fg_ml_runner -p "$DB_DIR" -m "$DB_NAME" -g adj -r --prediction=structural \
  --candidates=fof --method=jaccard --out=structural.csv
```

### Validation (s12, 2026-11-24, scratch copies; DB without stored embeddings for structural runs)

Pipeline vs specialized direct, 1 thread, top-100 (pipeline is serial; seconds are `workload_s`):

| candidates | method | pairs | positives | pipeline s (nbr / score) | direct s | top-100 |
|---|---|---|---|---|---|---|
| FoF | CN | 1,061,287 | 16,243 | 2.96 (2.05 / 0.59) | 1.91 | identical |
| FoF | Jaccard | 1,061,287 | 16,243 | 3.00 (2.08 / 0.59) | 1.84 | identical |
| FoF | cosine | 1,061,287 | 16,243 | 2.80 (1.95 / 0.55) | – | – |
| 2-hop | CN | 1,077,491 | 32,447 | 3.15 (2.09 / 0.61) | 2.03 | identical |
| 2-hop | Jaccard | 1,077,491 | 32,447 | 3.39 (2.25 / 0.65) | 1.98 | identical |
| 2-hop | cosine | 1,077,491 | 32,447 | 2.94 (1.96 / 0.55) | – | – |
| exhaustive | CN | 8,366,095 | 32,447 | 18.0 (12.8 / 3.7) | 9.12 | identical |
| exhaustive | Jaccard | 8,366,095 | 32,447 | 17.5 (12.4 / 3.6) | 8.46 | identical |
| exhaustive | cosine | 8,366,095 | 32,447 | 16.4 (11.7 / 3.4) | – | – |
| uniform, 100k draws | CN / Jaccard / cosine | 99,977 | 381 | 0.37 | – | – |

- Top-100: same pairs, same scores and order (CN cutoff 27 with 9 tied rows, Jaccard cutoff 0.5 with 81 tied rows; ties resolved the same).
- Every FoF score (1,061,287 pairs, all three methods) and label matches a Python reference built from the DB's edges.
  Scores in the CSV are rounded by `TopK` (`quantize_score`, ~9 digits), so compare with tolerance.
- Peak RSS: pipeline ~1.5 GB, direct ~0.62 GB (s12).
- Sampled sources (100, seed 7): structural and embedding runs score identical pair sets: FoF 44,322, 2-hop 45,002,
  3-hop 276,511, exhaustive 404,050.
- Regressions: embedding FoF default still byte-identical (1,061,287 / 16,243); fetch counters 2,122,574 / 2,122,574 / 1,080,294
  for direct / sorted / dedup; `validate_source_selection.sh` 11/11; exhaustive every-vertex == all-source (8,366,095);
  embedding mode on a DB without embeddings fails cleanly (`Missing or invalid vector: 0`).
- Directed (web-Stanford, every vertex a source, `khop --hops=1`): 2,312,497 ordered pairs = all edges, filter off,
  639,722 pairs kept in both orders (319,861 reciprocal edges).
- Table files (`*.wt` data) unchanged by all runs; only `WiredTiger.wt` (WiredTiger's own metadata) changes on open/close.
