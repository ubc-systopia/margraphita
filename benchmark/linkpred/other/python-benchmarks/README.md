# Python baselines (PyG)

`khop_pyg.py` — k-hop candidate generation + CN / Jaccard scoring with PyTorch
Geometric (`k_hop_subgraph` per source node), to compare against the C++
`run_reference_khop_structural_bfs` on the same kron graphs.

Runs on the WSL host, **outside** the `flexograph-perf` container (the container
has no pip / torch). Same machine, so don't run it while a C++ benchmark is
running.

## Setup (once)

WSL's Python has no `python3-venv`, so the venv is made with `virtualenv`:

```bash
python3 -m pip install --user virtualenv
python3 -m virtualenv ~/pyg
~/pyg/bin/pip install torch --index-url https://download.pytorch.org/whl/cpu
~/pyg/bin/pip install torch_geometric pandas networkx scipy
```

Tested with torch 2.14.1+cpu, torch_geometric 2.8.1, pandas 2.3.3, Python 3.10.

## Paths

The container's `/drives/...` folders are WSL folders under `/mnt/c/`:

| container | WSL host |
|---|---|
| `/drives/hdd_main/kron_graphs/<g>/graph_<g>` (raw edge lists) | `/mnt/c/FlexoGraphData/hdd_main/kron_graphs/<g>/graph_<g>` |
| `/drives/ml-pytorch-data/...` (results) | `/mnt/c/ml-pytorch/data/...` |

## Run

```bash
~/pyg/bin/python -W ignore khop_pyg.py /mnt/c/FlexoGraphData/hdd_main/kron_graphs/s12_e8/graph_s12_e8 --k 2 --method both --topk 10
```

Whole-graph version (one sparse `A @ A`, k = 2 only):

```bash
~/pyg/bin/python -W ignore khop_pyg.py /mnt/c/FlexoGraphData/hdd_main/kron_graphs/s12_e8/graph_s12_e8 --impl wholegraph --method both --topk 10
```

Options: `--impl pernode|wholegraph` (default pernode), `--k N` (hops, default
2; wholegraph is k = 2 only), `--method cn|jaccard|both`, `--topk N`, `--toy`
(tiny built-in graph instead of a file). Run each `--impl` in its own process:
peak memory is per process.

| `--impl` | how | cost |
|---|---|---|
| `pernode` | `k_hop_subgraph(u, 2)` per u + Python set intersections | naive: each call scans the whole edge list |
| `wholegraph` | sparse `A @ A`: non-zeros = 2-hop pairs, values = CN; plus A for direct neighbours | vectorized and multithreaded, but holds every 2-hop pair in memory |

## Expected output on s12_e8 (k = 2)

Must match the C++ reference
(`fg_linkpred_bench_db ... --impl=reference --candidates=khop --khop=2`):

| | CN | Jaccard |
|---|---|---|
| pairs_scored | 1,077,491 | 1,077,491 |
| true_edges | 32,447 | 32,447 |
| top-10 | (0,16) 73, (0,1) 72, (0,32) 67, (0,8) 58, (0,2) 55, (0,256) 55, (0,4) 51, (0,2048) 50, (0,64) 47, (0,511) 47 | ten pairs at 1.0, all label 0 (degree-1 ties) |

Both `--impl` values give these exact counts and top-10 lists. Measured
2026-11-09 (file load not included):

| `--impl` | CN | Jaccard | peak memory |
|---|---|---|---|
| pernode | 1.93 s | 3.28 s | 350 MB |
| wholegraph | 0.25 s | 0.21 s | 553 MB |

For comparison, the C++ single-thread DB references on s12: khop2 BFS 1.13 s,
nested counted 0.24 s (CN).

## What the loader does

The kron edge lists are tab-separated and list most edges once, while the DBs
are ingested undirected. `load_edges` therefore reads with `sep=r"\s+"` and
calls `to_undirected`; without it the counts do not match the C++.
`k_hop_subgraph` uses `flow="target_to_source"` (out-neighbours, like
`get_out_nodes_id`).

## C++ side, for the comparison

Inside the container:

```bash
docker exec flexograph-perf bash -c 'cd /tmp/lpcsr && ./fg_linkpred_bench_db -p /drives/ssd2/flexograph_rd/kron/s12_e8 -m adj_r_s12_e8 -g adj -r --topk=10 --candidates=khop --khop=2 --method=cn --impl=reference --topk-out=tk_cn.csv'
```
