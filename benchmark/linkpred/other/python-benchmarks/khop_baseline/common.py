"""Shared pieces: results writer, graph loading, scoring and timing helpers.
"""
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


# ------------------------------------------------------------------ results writer

CSV_FIELDS = [
    "timestamp", "graph", "n", "n_edges", "directed", "tag",
    "mode", "cand", "pairs_scored", "true_edges", "p_at_k",
    "gen_score_ms", "rank_ms", "total_ms", "gen_ms", "score_ms",
    "load_ms", "build_ms", "peak_rss_mb", "threads", "omp_threads",
    "hits_at_k", "mrr", "mean_rank",
    "host", "git_commit", "torch_version", "seed", "samples",
]


class Tee:
    """Buffer stdout so it can be saved to a per-run log at end (even on crash)."""
    def __init__(self):
        self.original = sys.stdout
        self.buffer = []
    def write(self, s):
        self.original.write(s)
        self.buffer.append(s)
    def flush(self):
        self.original.flush()
    def save(self, path):
        try:
            with open(path, "w") as f:
                f.write("".join(self.buffer))
        except OSError as e:
            print(f"[warn] could not write log {path}: {e}", file=self.original)


def sanitize(name):
    return (name.replace("/", "_").replace("\\", "_")
                .replace(" ", "_").replace(":", "_"))


def get_git_commit():
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=os.path.dirname(os.path.abspath(__file__)),
            stderr=subprocess.DEVNULL,
        ).decode().strip()
    except Exception:
        return "unknown"


def ensure_results_dir(base):
    base = os.path.abspath(base)
    os.makedirs(base, exist_ok=True)
    os.makedirs(os.path.join(base, "logs"), exist_ok=True)
    return base


def append_row(results_dir, row, fields=CSV_FIELDS):
    """Append one row to aggregate.csv and aggregate.jsonl.
    Single write per file so concurrent invocations don't interleave."""
    csv_path = os.path.join(results_dir, "aggregate.csv")
    is_new = not os.path.exists(csv_path)
    cells = []
    for f in fields:
        v = row.get(f, "")
        if v is None:
            v = ""
        if isinstance(v, str) and any(c in v for c in [",", '"', "\n"]):
            v = '"' + v.replace('"', '""') + '"'
        cells.append(str(v))
    line = ",".join(cells) + "\n"
    with open(csv_path, "a") as f:
        if is_new:
            f.write(",".join(fields) + "\n")
        f.write(line)

    jsonl_path = os.path.join(results_dir, "aggregate.jsonl")
    clean = {k: ("" if v is None else v) for k, v in row.items()}
    with open(jsonl_path, "a") as f:
        f.write(json.dumps(clean, default=str) + "\n")


# ------------------------------------------------------------------ loading

def load_graph(path, directed=False):
    """Edge list 'u v' per line -> deduped, self-loops removed, symmetrized.
    directed=True keeps direction (matches C++ get_out_nodes_id / DIRECTED=1).

    Refs:
      PyG edge_index COO: https://pytorch-geometric.readthedocs.io/en/latest/get_started/introduction.html
      PyG CSV tutorial:   https://pytorch-geometric.readthedocs.io/en/latest/tutorial/load_csv.html
    """
    t0 = time.perf_counter()
    src, dst = [], []
    with open(path) as f:
        for line in f:
            if not line.strip() or line[0] in "#%":
                continue
            p = line.split()
            src.append(int(p[0])); dst.append(int(p[1]))
    s = torch.tensor(src, dtype=torch.int64)
    d = torch.tensor(dst, dtype=torch.int64)
    if directed:
        ei = torch.stack([s, d])
    else:
        ei = torch.cat([torch.stack([s, d]), torch.stack([d, s])], dim=1)
    ei = ei[:, ei[0] != ei[1]]
    n = int(ei.max()) + 1
    key = torch.unique(ei[0] * n + ei[1])
    ei = torch.stack([key // n, key % n])
    return ei, n, time.perf_counter() - t0


def adjacency(ei, n):
    """Sparse adjacency (n, n), float32, coalesced.
    Refs:
      torch.sparse_coo_tensor: https://pytorch.org/docs/stable/generated/torch.sparse_coo_tensor.html
      .coalesce():             https://pytorch.org/docs/stable/generated/torch.Tensor.coalesce.html
    """
    vals = torch.ones(ei.shape[1], dtype=torch.float32)
    return torch.sparse_coo_tensor(ei, vals, (n, n)).coalesce()


def neighbour_sets(ei, n):
    """Python sets of out-neighbours — for CN by set intersection."""
    nset = [set() for _ in range(n)]
    for a, b in ei.t().tolist():
        nset[a].add(b)
    return nset


# ------------------------------------------------------------------ helpers

def topk_pairs(u, v, score, label, k):
    """Top-k by score desc, ties by (u, v) asc — matches the C++ heap order.
    torch.argsort(..., stable=True): https://pytorch.org/docs/stable/generated/torch.argsort.html
    """
    order = torch.argsort(v, stable=True)
    order = order[torch.argsort(u[order], stable=True)]
    order = order[torch.argsort(-score[order], stable=True)][:k]
    return [(int(u[i]), int(v[i]), float(score[i]), int(label[i])) for i in order]


def score_pairs(u, v, nset, k):
    """CN via set intersection + label check + top-k."""
    cn = torch.tensor([len(nset[a] & nset[b]) for a, b in zip(u.tolist(), v.tolist())],
                      dtype=torch.float32)
    lab = torch.tensor([1 if b in nset[a] else 0 for a, b in zip(u.tolist(), v.tolist())],
                       dtype=torch.int64)
    return int(u.numel()), int(lab.sum()), topk_pairs(u, v, cn, lab, k)


def peak_mb():
    """Peak RSS (MB). resource.getrusage(RUSAGE_SELF).ru_maxrss:
    https://docs.python.org/3/library/resource.html#resource.getrusage"""
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024.0


def make_scorer(method, nset):
    if method == "cn":
        return lambda u, v: float(len(nset[u] & nset[v]))
    if method == "jaccard":
        def s(u, v):
            inter = len(nset[u] & nset[v])
            union = len(nset[u] | nset[v])
            return inter / union if union > 0 else 0.0
        return s
    raise ValueError(f"unknown method: {method}")


def mk_timings(t0, t1, t2, gen_ms=None, score_ms=None):
    """Standard timings dict. t0=start, t1=end gen+score, t2=end rank."""
    return dict(
        gen_score_ms=(t1 - t0) * 1e3,
        rank_ms=(t2 - t1) * 1e3,
        total_ms=(t2 - t0) * 1e3,
        gen_ms=gen_ms,
        score_ms=score_ms,
    )
