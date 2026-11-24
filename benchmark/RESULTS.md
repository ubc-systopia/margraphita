# Where results live

Index of benchmark results and logs (updated 2026-11-24). Paths are relative to the repo root unless they start with `/` or `~`.
Inside the container the repo is `/FlexoGraph/pull-property/margraphita`. Result folders are not tracked in git.

## Link prediction: `benchmark/linkpred/output/`

| Folder | What | Made by | Deck slides |
|---|---|---|---|
| `structural/direct_<cand>_<variant>/metrics.csv` (+ `logs/`) | Every structural run (CN, Jaccard): time, load, peak RSS, pairs, P@10, direction, eval | `fg_linkpred_bench_db` via `other/runners/run_lp_all.sh` | results 1–8, every‑run grids |
| `embedding/direct_<cand>_<variant>/metrics.csv` (+ `logs/`) | Every embedding run (cosine, dot) | same, `SET=emb` | results 7, embedding grid |
| `sweeps/sample_candidates_struct/<graph>_<date>/` | Structural sweeps per graph (17 Nov, 23 Nov, kron s21–s23 24 Nov) | `run_lp_all.sh` | results, directed‑both |
| `sweeps/sample_candidates_emb/<graph>_<date>/` | Embedding sweeps per graph | `run_lp_all.sh SET=emb` | results 7 |
| `sweeps/sample_candidates_struct_holdout/` | Holdout vs recon (s12) | `run_lp_all.sh` with a split file | goals |
| `sweeps/sample_candidates/`, `sweeps/sample_candidates_cache/` | 17 Nov kron + facebook sweeps, cache sweep | `run_sample_candidates_cache_sweep.sh` | Nov 17 sc‑*, om‑* |
| `sweeps/fastrp/<date>/` | FastRP timings and embedding fill‑ins; `s17_*`, `ws_*` = 24 Nov `fastrp_sweep.sh` | `other/runners/run_fastrp_sweep.sh`, `benchmark/ml/scripts/fastrp_sweep.sh` | fastrp‑*, embeddings, coverage grid |
| `sweeps/networkit/` | NetworKit comparison | runner in `other/` | Nov 17 nk‑* |
| `sweeps/ext4_reload/` | WSL ext4 vs /mnt/c reload timing | `~/flexography-clone-structural/ext4_reload.sh` | setup slides |
| `sweeps/rerun_logs/` | Console logs of the directed, web‑Stanford, kron s21–s23 and soc‑linkedin reruns | runners | results validity, updates |
| `memcap/s20_{1g,2g,4g}/` | Memory‑cap runs | docker `--memory` runs | updates, goals |
| `splits/` | Holdout split files | split tool | goals |
| `ingest/loader_check/s17/` | Sorted vs original loader: table dumps and 28‑config comparison | `loader_*_check.sh` | faster loader section |
| `ingest/logs/<db>/` | Copies of ingest logs (`*_ingest_sorted.out`, `*_insert.log`); live copies stay in `/drives/hdd_main/logs` | `kron_big_ingest.sh`, `insert_kron.py` | loader, updates (s21–s25) |
| `python_baselines/py_{baseline,cugraph,networkx}/` | Python baselines (PyG, NetworkX, NetworKit, cuGraph) | `other/python-benchmarks/py_khop_baseline.py` | Nov 17 python slides |
| `python_baselines/py_baseline_other_results/` | Earlier py_baseline aggregate (10 Nov) | same | – |
| `scratch_tests/` | One‑off test logs | manual | – |
| `fastrp/` | Empty | – | – |

## ML API and runner: `benchmark/ml/output/<date>/`

| Folder (2026‑11‑24) | What | Deck slides |
|---|---|---|
| `structural_check/` | Pipeline vs direct, structural CN/Jaccard/cosine, reference checks, md5 before/after | ML API: runner |
| `fetch_counters/` | Direct / sorted / dedup retrieval with fetch counts | ML API: dedup |
| `source_selection/` | Sampled sources, reverse duplicates, every‑vertex checks | ML API: sources |
| `sampling/` | `ml_sampling` vs stored adjacency (web‑Stanford, s12) | ML API: sampling |
| `adj_lookup/` | has_node() guard removal, before/after | ML API: engine fix |

## Microbenchmarks: `benchmark/microbenchmarks/output/`

| Folder | What | Made by | Deck slides |
|---|---|---|---|
| `ml_access/<date>/` | First ml_access_bench sweep | `~/flexography-clone-structural/ml_access_sweep.sh` | ML access summary |
| `ml_access_verify/<date>/` | Verification sweep (stats, splits) | `~/flexography-clone-structural/ml_access_verify.sh` | mlb‑* findings |

Figures and tables from these: `plots/ml_access_plots/` (script `plots/plot_ml_access_verify.py`).
Older microbenchmark notes: `benchmark/microbenchmarks/*_output_README.md`.

## Outside the repo (not moved)

| Path | What |
|---|---|
| `outputs/` (repo root) | 2021–22 PageRank / BFS / TC results (older engine) |
| `/mnt/c/ml-pytorch/data/linkpred/` (`/drives/ml-pytorch-data/linkpred`) | Older PyTorch link‑prediction runs, microbench logs (320 MB) |
| `/mnt/c/FlexoGraphData/hdd_main/logs/` (`/drives/hdd_main/logs`) | Live ingest logs; copied into `ingest/logs/` above |
| `~/flexography-clone-structural/ldbc/` | LDBC SF3 data and loaded DBs (data, not results) |
| `~/flexography-clone-structural/ml_check/` | Probe programs used for one‑off checks |
