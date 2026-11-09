#!/usr/bin/env python3
"""edge_loader.py — read a graph into a symmetrized COO edge_index.

Two functions with identical return contract:

    load_edges(path)              — read from a raw text file ('u v' per line)
    load_edges_from_flexo(...)    — subprocess to dump_edges, read from a FlexoGraph DB

Both return (edge_index [2, |E|] int64, n).
Downstream code (StubGraphStore, StubFeatureStore) doesn't care which is used.
"""
import os
import subprocess

import torch


# Default location of the compiled dump_edges tool. Override via
# FLEXO_DUMP_BIN env var, or pass dump_binary= explicitly.
_DEFAULT_DUMP_BIN = (
    "/FlexoGraph/pull-property/margraphita/build/release/benchmark/linkpred/dump_edges"
)


# ─── Raw text file loader (already validated) ──────────────────────────

def load_edges(path, directed=False):
    """Read 'u v' per line -> (edge_index [2, |E|] int64, n).

    directed=False  symmetrize (add reverse edges), dedupe, drop self-loops
    directed=True   keep direction as written, dedupe, drop self-loops

    Skips blank lines and lines starting with # or %.
    """
    src, dst = [], []
    with open(path) as f:
        for line in f:
            if not line.strip() or line[0] in "#%":
                continue
            p = line.split()
            src.append(int(p[0]))
            dst.append(int(p[1]))

    s = torch.tensor(src, dtype=torch.int64)
    d = torch.tensor(dst, dtype=torch.int64)

    if directed:
        ei = torch.stack([s, d])
    else:
        ei = torch.stack([torch.cat([s, d]), torch.cat([d, s])])

    n = int(ei.max()) + 1
    key = torch.unique(ei[0] * n + ei[1])
    ei = torch.stack([key // n, key % n])
    ei = ei[:, ei[0] != ei[1]]
    return ei, n


# ─── FlexoGraph DB loader ──────────────────────────────────────────────

def load_edges_from_flexo(db_dir, db_name, graph_type="adj",
                          directed=False, dump_binary=None):
    """Read edges from a FlexoGraph DB via the dump_edges tool.

    Args:
      db_dir        path to the parent directory, e.g.
                    /drives/ssd2/flexograph_rd/kron/s12_e8
      db_name       DB subdirectory name, e.g. adj_r_s12_e8
      graph_type    "adj" or "split_ekey"
      directed      False (default) symmetrizes; True keeps direction
      dump_binary   path to compiled dump_edges. Falls back to
                    FLEXO_DUMP_BIN env var, then _DEFAULT_DUMP_BIN.

    Returns:
      (edge_index [2, |E|] int64, n)
      — same contract as load_edges(path).

    Raises:
      FileNotFoundError  if dump_binary does not exist
      subprocess.CalledProcessError  if the tool exits non-zero
    """
    if dump_binary is None:
        dump_binary = os.environ.get("FLEXO_DUMP_BIN", _DEFAULT_DUMP_BIN)

    if not os.path.exists(dump_binary):
        raise FileNotFoundError(
            f"dump_edges not found at {dump_binary}. "
            f"Build it with: cmake --build build/release --target dump_edges, "
            f"or set FLEXO_DUMP_BIN."
        )

    args = [dump_binary, db_dir, db_name, graph_type]
    if directed:
        args.append("--directed")

    raw = subprocess.check_output(args, text=True)

    # Parse header + edge lines. Header is "# n=X m=Y directed=Z".
    n = None
    src, dst = [], []
    for line in raw.splitlines():
        line = line.strip()
        if not line:
            continue
        if line.startswith("#"):
            for tok in line[1:].split():
                if tok.startswith("n="):
                    n = int(tok[2:])
            continue
        u, v = line.split()
        src.append(int(u))
        dst.append(int(v))

    if not src:
        # Empty edge list — return an empty 2x0 tensor.
        return torch.empty((2, 0), dtype=torch.int64), n or 0

    s = torch.tensor(src, dtype=torch.int64)
    d = torch.tensor(dst, dtype=torch.int64)

    if directed:
        ei = torch.stack([s, d])
    else:
        ei = torch.stack([torch.cat([s, d]), torch.cat([d, s])])

    if n is None:
        n = int(ei.max()) + 1

    key = torch.unique(ei[0] * n + ei[1])
    ei = torch.stack([key // n, key % n])
    ei = ei[:, ei[0] != ei[1]]
    return ei, n