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

    args = [dump_binary, db_dir, db_name, graph_type]
    if not os.path.exists(dump_binary):
        # 2026-11-24: on the WSL host (torch/PyG) the tool lives in the container
        container = os.environ.get("FLEXO_CONTAINER")
        if not container:
            raise FileNotFoundError(
                f"dump_edges not found at {dump_binary}. "
                f"Build it with: cmake --build build/release --target dump_edges, "
                f"set FLEXO_DUMP_BIN, or set FLEXO_CONTAINER to run it via docker exec."
            )
        args = ["docker", "exec", container] + args
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

# ─── Node features from a FlexoGraph DB (2026-11-24) ───────────────────
# dump_edges --features=OUT.npy --features-only writes the DB's node properties (e.g. the
# FastRP embeddings fg_fastrp stores with FG_OUT=db) as a float32 [max_id + 1, dim] matrix,
# row = node id, the same indexing load_edges_from_flexo uses. torch/PyG live on the WSL host
# and the tool lives in the container, so if the binary is not on this machine the call goes
# through `docker exec $FLEXO_CONTAINER` and paths are mapped between FLEXO_HOST_ROOT (host)
# and FLEXO_CONTAINER_ROOT (container).
_HOST_ROOT = os.environ.get("FLEXO_HOST_ROOT", os.path.expanduser("~/flexography-clone-structural"))
_CONT_ROOT = os.environ.get("FLEXO_CONTAINER_ROOT", "/FlexoGraph")


def export_features_from_flexo(db_dir, db_name, out_npy, graph_type="adj",
                               dump_binary=None, container=None):
    """Write the DB's node features to out_npy (a host path). Returns out_npy.

    db_dir is a container path (as for load_edges_from_flexo). out_npy must be under
    FLEXO_HOST_ROOT when going through docker, so the container can write it.
    """
    if dump_binary is None:
        dump_binary = os.environ.get("FLEXO_DUMP_BIN", _DEFAULT_DUMP_BIN)
    out_npy = os.path.abspath(out_npy)
    os.makedirs(os.path.dirname(out_npy), exist_ok=True)
    if os.path.exists(dump_binary):                      # running where Flexograph is built
        args = [dump_binary, db_dir, db_name, graph_type,
                f"--features={out_npy}", "--features-only"]
    else:                                                # host -> container
        container = container or os.environ.get("FLEXO_CONTAINER", "flexograph-fresh")
        if not out_npy.startswith(_HOST_ROOT + "/"):
            raise ValueError(f"out_npy must be under {_HOST_ROOT} to be visible in the container")
        cont_out = _CONT_ROOT + out_npy[len(_HOST_ROOT):]
        args = ["docker", "exec", container, dump_binary, db_dir, db_name, graph_type,
                f"--features={cont_out}", "--features-only"]
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL)
    return out_npy
