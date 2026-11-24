#!/usr/bin/env python3
"""py_khop_baseline.py — in-memory Python baseline for linkpred candidate
generation, to validate the C++ linkpred rules on the same kron graphs.

Rules implemented (all with v > u, v != u):
  khop2    1 <= dist(u, v) <= 2               (nonzeros of A + A·A)
  fof      v reached by u -> w -> v           (nonzeros of A·A)
  uniform  fixed-K random ids in (u, n), no edge rejection
  oracle   all positive edges (perfect recall; pair-centric ceiling)

Modes (--mode, comma list):
  twohop          PyG TwoHop (A·A) ∪ A
  khop_subgraph   PyG k_hop_subgraph(u, 2) per source (slow)
  fanout          PyG NeighborLoader (sampled 2-hop per seed; needs pyg-lib)
  fanout_manual   hand-rolled fan-out sampling (no pyg-lib needed)
  sparse_chunked  hand-rolled A[rows] @ A, row-chunked (--chunk). Gives khop2+fof
  networkx        NetworkX BFS depth 2 + common_neighbors
  networkit       NetworKit MissingLinksFinder(2) + CommonNeighbors/Jaccard index (C++, OpenMP)
  networkit_stream  same NetworKit calls, per-node findFromNode + batched runOn, bounded memory
                  (for graphs past s12; gen_ms / score_ms = time inside NetworKit)
  uniform         fixed-K per-source uniform random (C++ uniformRandom)
  oracle          all positive edges as candidates (pair-centric)
  ogb_eval        OGB Hits@K / MRR against a fixed negative set (pair-centric)

Timing policy — matches what the C++ bench reports:
  gen_score_ms  fused generation+scoring. For khop2/fof this is one sparse
                matmul (A@A produces candidates AND their CN scores at once);
                splitting would double the work or report a number the
                algorithm never actually computes.
  rank_ms       top-k heap / sort.
  total_ms      gen_score + rank (end-to-end for the mode).
  gen_ms        ONLY for uniform — the RNG draw phase, truly independent.
  score_ms      ONLY for uniform — per-pair CN. Empty for all fused modes.

Code layout: this file is the CLI (args + dispatch); the modes live in
khop_baseline/, one file per library (modes_pyg, modes_torch, modes_networkx,
modes_cugraph), with shared helpers in khop_baseline/common.py and OGB-style
evaluation in khop_baseline/ogb_eval.py.

Results layout (default, override with --results-dir):
  results/py_baseline/
    aggregate.csv          appended across all runs, fixed schema
    aggregate.jsonl        same rows, one JSON object per line
    logs/
      {timestamp}_{graph}[_{tag}].log   full stdout of each invocation

For accurate per-mode peak RSS, run one mode per process (see bulk loop
example at bottom of this docstring) — ru_maxrss is monotonic within a
process, so multi-mode runs report cumulative peak, not per-mode peak.

Target numbers from the C++ runs on s12_e8:
  khop2    pairs_scored=1,077,491  true_edges=32,447
  fof      pairs_scored=1,061,287  true_edges=16,243
  uniform  pairs_scored≈1,040,783  true_edges≈2,800   (K=263, seed=42)
  oracle   pairs_scored=32,447     true_edges=32,447

References:
  PyG loading CSV:      https://pytorch-geometric.readthedocs.io/en/latest/tutorial/load_csv.html
  PyG Data:             https://pytorch-geometric.readthedocs.io/en/latest/generated/torch_geometric.data.Data.html
  k_hop_subgraph:       https://pytorch-geometric.readthedocs.io/en/stable/modules/utils.html#torch_geometric.utils.k_hop_subgraph
  TwoHop transform:     https://pytorch-geometric.readthedocs.io/en/stable/generated/torch_geometric.transforms.TwoHop.html
  NeighborLoader:       https://pytorch-geometric.readthedocs.io/en/stable/modules/loader.html#torch_geometric.loader.NeighborLoader
  negative_sampling:    https://pytorch-geometric.readthedocs.io/en/stable/modules/utils.html#torch_geometric.utils.negative_sampling
  SEAL_OGB (k_hop_subgraph for LP): https://github.com/facebookresearch/SEAL_OGB
  OGB linkprop docs:    https://ogb.stanford.edu/docs/linkprop/
  OGB evaluator source: https://github.com/snap-stanford/ogb/blob/master/ogb/linkproppred/evaluate.py
  torch.sparse.mm:      https://pytorch.org/docs/stable/generated/torch.sparse.mm.html
  torch.sparse_coo_tensor: https://pytorch.org/docs/stable/generated/torch.sparse_coo_tensor.html
  torch.searchsorted:   https://pytorch.org/docs/stable/generated/torch.searchsorted.html
  torch.argsort (stable): https://pytorch.org/docs/stable/generated/torch.argsort.html
  NetworkX BFS:         https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.shortest_paths.unweighted.single_source_shortest_path_length.html
  NetworkX common_neighbors: https://networkx.org/documentation/stable/reference/generated/networkx.classes.function.common_neighbors.html
  resource.getrusage:   https://docs.python.org/3/library/resource.html#resource.getrusage
  argparse:             https://docs.python.org/3/library/argparse.html

Single run:
  python3 py_khop_baseline.py --edges .../graph_s12_e8 \\
      --mode sparse_chunked --cand khop2,fof
  python3 py_khop_baseline.py --edges .../graph_s12_e8 \\
      --mode uniform --samples 263 --seed 42

Bulk loop (one mode per process → clean RSS):
  for g in s12_e8 s16_e8 soc-livejournal com-friendster; do
    for m in sparse_chunked uniform; do
      ~/pyg/bin/python py_khop_baseline.py \\
        --edges /path/to/$g/graph_$g \\
        --mode $m --cand khop2,fof --samples 263 \\
        --results-dir benchmark/linkpred/output/python_baselines/py_baseline \\
        --tag bulk_v1
    done
  done
"""
import argparse
import csv
import datetime
import json
import os
import resource
import socket
import subprocess
import sys
import time

import torch

from khop_baseline.common import *        # noqa: F401,F403  results writer, loading, helpers
from khop_baseline.modes_pyg import run_twohop, run_pyg, run_fanout_pyg
from khop_baseline.modes_torch import run_sparse, run_fanout, run_uniform, run_oracle
from khop_baseline.modes_networkx import run_networkx
from khop_baseline.modes_networkit import run_networkit, run_networkit_stream
from khop_baseline.modes_cugraph import run_cugraph_native
from khop_baseline.ogb_eval import gen_uniform_negatives, run_ogb_eval


# ------------------------------------------------------------------ main

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--edges", required=True)
    ap.add_argument("--mode", default="sparse_chunked",
                help="twohop,khop_subgraph,fanout,fanout_manual,sparse_chunked,"
                     "networkx,networkit,networkit_stream,uniform,oracle,ogb_eval,cugraph_native")
    ap.add_argument("--cand", default="khop2,fof")
    ap.add_argument("--method", default="cn", choices=["cn", "jaccard"])
    ap.add_argument("--batch", type=int, default=1024)
    ap.add_argument("--chunk", type=int, default=65536)
    ap.add_argument("--s1", type=int, default=10)
    ap.add_argument("--s2", type=int, default=5)
    ap.add_argument("--samples", type=int, default=263)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--topk", type=int, default=10)
    ap.add_argument("--ogb-k", type=int, default=10)
    ap.add_argument("--ogb-neg-ratio", type=int, default=1)
    ap.add_argument("--ogb-neg-file", default="")
    ap.add_argument("--threads", type=int, default=0)
    ap.add_argument("--directed", action="store_true")
    ap.add_argument("--n", type=int, default=0)
    ap.add_argument("--results-dir", default="./results/py_baseline")
    ap.add_argument("--tag", default="")
    ap.add_argument("--no-results", action="store_true")
    a = ap.parse_args()
    if a.threads:
        torch.set_num_threads(a.threads)

    results_dir = None if a.no_results else ensure_results_dir(a.results_dir)
    run_ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    graph_name = os.path.basename(a.edges)
    tag_safe = sanitize(a.tag) if a.tag else ""

    tee = Tee()
    sys.stdout = tee
    log_path = None
    if results_dir:
        log_name = f"{run_ts}_{sanitize(graph_name)}" + (f"_{tag_safe}" if tag_safe else "")
        log_path = os.path.join(results_dir, "logs", log_name + ".log")

    try:
        _run(a, run_ts, graph_name, tag_safe, results_dir)
    finally:
        sys.stdout = tee.original
        if log_path:
            tee.save(log_path)
            print(f"[results] log      -> {log_path}")
        if results_dir:
            print(f"[results] aggregate -> {os.path.join(results_dir, 'aggregate.csv')}")
            print(f"[results] jsonl     -> {os.path.join(results_dir, 'aggregate.jsonl')}")


def _run(a, run_ts, graph_name, tag_safe, results_dir):
    # --- one-time build phases (timed separately so build cost isn't hidden)
    t0 = time.perf_counter()
    ei, n_inferred, _ = load_graph(a.edges, directed=a.directed)
    load_ms = (time.perf_counter() - t0) * 1e3
    n = a.n if a.n > 0 else n_inferred
    n_edges = ei.shape[1]

    modes = a.mode.split(",")
    need_adj = "sparse_chunked" in modes
    need_nset = any(m in modes for m in [
        "twohop", "khop_subgraph", "fanout", "fanout_manual", "networkx",
        "uniform", "oracle", "ogb_eval"
    ])

    t0 = time.perf_counter()
    A = adjacency(ei, n) if need_adj else None
    adj_build_ms = (time.perf_counter() - t0) * 1e3

    _ns = []
    def nset():
        if not _ns:
            _ns.append(neighbour_sets(ei, n))
        return _ns[0]

    t0 = time.perf_counter()
    if need_nset:
        nset()
    nset_build_ms = (time.perf_counter() - t0) * 1e3
    build_ms = adj_build_ms + nset_build_ms

    print(f"graph={graph_name} n={n} (inferred {n_inferred}) "
          f"edges={n_edges} load_ms={load_ms:.1f} build_ms={build_ms:.1f} "
          f"(adj={adj_build_ms:.1f} nset={nset_build_ms:.1f}) "
          f"threads={torch.get_num_threads()} "
          f"omp_threads={os.environ.get('OMP_NUM_THREADS', 'default')} "
          f"directed={a.directed} tag={tag_safe or '(none)'}")
    if results_dir:
        print(f"results_dir={results_dir}")

    # --- run metadata (captured once)
    host = socket.gethostname()
    git_commit = get_git_commit()
    torch_version = torch.__version__
    omp_threads = int(os.environ.get("OMP_NUM_THREADS", 0)) or torch.get_num_threads()

    # --- OGB negatives (cached)
    _negs = []
    def ogb_negs():
        if not _negs:
            if a.ogb_neg_file:
                nu, nv = [], []
                with open(a.ogb_neg_file) as f:
                    for line in f:
                        if not line.strip() or line[0] in "#%":
                            continue
                        p = line.split()
                        nu.append(int(p[0])); nv.append(int(p[1]))
                _negs.append((nu, nv))
            else:
                _negs.append(gen_uniform_negatives(ei, n, nset(),
                                                   ratio=a.ogb_neg_ratio, seed=a.seed))
        return _negs[0]

    def base_row(**over):
        row = dict(
            timestamp=run_ts, graph=graph_name, n=n, n_edges=n_edges,
            directed=int(a.directed), tag=tag_safe,
            load_ms=round(load_ms, 1), build_ms=round(build_ms, 1),
            peak_rss_mb=round(peak_mb()),
            threads=torch.get_num_threads(), omp_threads=omp_threads,
            host=host, git_commit=git_commit, torch_version=torch_version,
            seed=a.seed, samples=a.samples,
            # defaults
            gen_score_ms="", rank_ms="", total_ms="",
            gen_ms="", score_ms="", p_at_k="",
            hits_at_k="", mrr="", mean_rank="",
        )
        row.update(over)
        return row

    # --- dispatch per mode
    for mode in modes:
        cands = a.cand.split(",") if mode in ("sparse_chunked", "networkit", "networkit_stream") else ["khop2"]
        for cand in cands:
            try:
                if mode == "twohop":
                    pairs, edges, top, tm = run_twohop(ei, n, nset(), a.topk)
                elif mode == "khop_subgraph":
                    pairs, edges, top, tm = run_pyg(ei, n, nset(), a.topk, a.directed)
                elif mode == "fanout":
                    pairs, edges, top, tm = run_fanout_pyg(ei, n, nset(), a.s1, a.s2,
                                                           a.topk, a.batch, a.seed)
                    cand = f"khop2_fanout[{a.s1},{a.s2}]"
                elif mode == "sparse_chunked":
                    pairs, edges, top, tm = run_sparse(A, n, cand, a.chunk, a.topk)
                elif mode == "networkit":
                    pairs, edges, top, tm = run_networkit(ei, n, cand, a.topk, a.method, a.threads)
                elif mode == "networkit_stream":
                    pairs, edges, top, tm = run_networkit_stream(ei, n, cand, a.topk, a.method, a.threads)
                elif mode == "networkx":
                    pairs, edges, top, tm = run_networkx(ei, n, a.topk, a.method)
                elif mode == "cugraph_native":
                    pairs, edges, top, tm = run_cugraph_native(a.edges, cand, a.topk, a.directed)
                elif mode == "fanout_manual":
                    pairs, edges, top, tm = run_fanout(ei, n, a.s1, a.s2, a.topk, a.seed)
                    cand = f"khop2_fanout_manual[{a.s1},{a.s2}]"
                elif mode == "uniform":
                    pairs, edges, top, tm = run_uniform(ei, n, a.samples, a.seed,
                                                        a.topk, nset())
                    cand = f"uniform[K={a.samples},seed={a.seed}]"
                elif mode == "oracle":
                    pairs, edges, top, tm = run_oracle(ei, n, nset(), a.topk)
                    cand = "oracle"
                elif mode == "ogb_eval":
                    scorer = make_scorer(a.method, nset())
                    pos_u = ei[0].tolist(); pos_v = ei[1].tolist()
                    neg_u, neg_v = ogb_negs()
                    hits, mrr, mr, tm = run_ogb_eval(pos_u, pos_v, neg_u, neg_v,
                                                    scorer, k=a.ogb_k)
                    print(f"PyKhop mode=ogb_eval method={a.method} "
                          f"positives={len(pos_u)} negatives={len(neg_u)} "
                          f"Hits@{a.ogb_k}={hits:.4f} MRR={mrr:.4f} mean_rank={mr:.1f} "
                          f"gen_score_ms={tm['gen_score_ms']:.1f} "
                          f"rank_ms={tm['rank_ms']:.1f} total_ms={tm['total_ms']:.1f} "
                          f"peak_rss_mb={peak_mb():.0f}")
                    if results_dir:
                        append_row(results_dir, base_row(
                            mode="ogb_eval",
                            cand=f"ogb[Hits@{a.ogb_k}]",
                            pairs_scored=len(pos_u), true_edges=len(pos_u),
                            p_at_k=round(hits, 4),
                            gen_score_ms=round(tm["gen_score_ms"], 1),
                            rank_ms=round(tm["rank_ms"], 1),
                            total_ms=round(tm["total_ms"], 1),
                            gen_ms="", score_ms="",
                            hits_at_k=round(hits, 4), mrr=round(mrr, 4),
                            mean_rank=round(mr, 1),
                        ))
                    continue
                else:
                    raise SystemExit(f"unknown mode {mode}")
            except (RuntimeError, MemoryError) as e:
                print(f"PyKhop mode={mode} cand={cand} FAILED: {e}")
                print(f"PyKhop peak_rss_mb={peak_mb():.0f} (OOM at mode={mode})")
                if results_dir:
                    append_row(results_dir, base_row(
                        mode=mode, cand=cand,
                        pairs_scored="", true_edges="",
                    ))
                sys.exit(1)

            p_at_k = sum(t[3] for t in top) / max(1, len(top))
            gen_s = tm["gen_ms"] if tm["gen_ms"] is not None else ""
            score_s = tm["score_ms"] if tm["score_ms"] is not None else ""
            print(f"PyKhop mode={mode} cand={cand} pairs_scored={pairs} "
                  f"true_edges={edges} P@{a.topk}={p_at_k:.4f} "
                  f"gen_score_ms={tm['gen_score_ms']:.1f} "
                  f"rank_ms={tm['rank_ms']:.1f} total_ms={tm['total_ms']:.1f}"
                  + (f" gen_ms={tm['gen_ms']:.1f} score_ms={tm['score_ms']:.1f}"
                     if tm["gen_ms"] is not None else "")
                  + f" peak_rss_mb={peak_mb():.0f}")
            if results_dir:
                append_row(results_dir, base_row(
                    mode=mode, cand=cand,
                    pairs_scored=pairs, true_edges=edges,
                    p_at_k=round(p_at_k, 4),
                    gen_score_ms=round(tm["gen_score_ms"], 1),
                    rank_ms=round(tm["rank_ms"], 1),
                    total_ms=round(tm["total_ms"], 1),
                    gen_ms=(round(gen_s, 1) if gen_s != "" else ""),
                    score_ms=(round(score_s, 1) if score_s != "" else ""),
                ))


if __name__ == "__main__":
    main()
