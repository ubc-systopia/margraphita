#!/usr/bin/env python3
"""pyg_smoke_test.py — prove the GraphStore + FeatureStore + NeighborLoader pipeline.

Runs two checks:
  1. Materialized-Data path  (graph copied into a Data object)
  2. Tuple (fs, gs) path     (PyG's remote-backend interface: the loader
                              samples through GraphStore.csc() and fetches
                              features from the FeatureStore per batch)

Toggle EDGE_SOURCE below to compare the DB-backed source against the raw
text file. Both must produce identical |E| and n for the same graph.
"""
import os
import sys

import torch
from torch_geometric.data import Data
from torch_geometric.data.feature_store import TensorAttr
from torch_geometric.loader import NeighborLoader

from backend import (load_edges_from_flexo, StubFeatureStore, StubGraphStore,
                     FlexoFeatureStore, export_features_from_flexo)


# ─── Configuration ─────────────────────────────────────────────────────
EDGE_SOURCE = "db"      # "db" | "raw"

# raw-file config
RAW_EDGES_PATH = "/mnt/c/FlexoGraphData/hdd_main/kron_graphs/s12_e8/graph_s12_e8"

# DB config
DB_DIR     = "/drives/ssd2/flexograph_rd/kron/s12_e8"
DB_NAME    = "adj_r_s12_e8"
DB_TYPE    = "adj"
DB_DIRECTED = False

# features (2026-11-24): "random" = StubFeatureStore, "db" = FlexoFeatureStore with the
# node properties of FEAT_DB_NAME (a FastRP copy of the same graph) exported to FEAT_NPY
FEATURE_SOURCE = "db"   # "db" | "random"
FEAT_DB_NAME   = "adj_r_s12_e8_fastrp64"
FEAT_NPY       = os.path.expanduser("~/flexography-clone-structural/features/s12_e8_fastrp64.npy")
_FS = None


def make_feature_store(n):
    """StubFeatureStore or FlexoFeatureStore, per FEATURE_SOURCE (exported once)."""
    global _FS
    if FEATURE_SOURCE == "random":
        return StubFeatureStore(n)
    if _FS is None:
        export_features_from_flexo(DB_DIR, FEAT_DB_NAME, FEAT_NPY, graph_type=DB_TYPE)
        _FS = FlexoFeatureStore(FEAT_NPY, n)
    return _FS


def check_features(batch, fs):
    """Real features: every batch row must equal the DB row of its node id."""
    if FEATURE_SOURCE != "db":
        return
    want = torch.from_numpy(fs._np[batch.n_id.numpy()].copy())
    ok = torch.equal(batch.x, want)
    print(f"    x rows match DB features for all {batch.n_id.numel()} nodes: {'✅' if ok else '❌'}")


def load_edges_from_config():
    """Load edges from whichever source EDGE_SOURCE selects."""
    if EDGE_SOURCE == "raw":
        from backend import load_edges
        return load_edges(RAW_EDGES_PATH, directed=DB_DIRECTED)
    return load_edges_from_flexo(
        DB_DIR, DB_NAME, graph_type=DB_TYPE, directed=DB_DIRECTED
    )


# ─── Checks ────────────────────────────────────────────────────────────

def check_materialized(ei, n):
    """Standard NeighborLoader path via a materialized Data object."""
    fs = make_feature_store(n)
    gs = StubGraphStore(ei, n)

    data = Data(
        x=fs._get_tensor(TensorAttr(attr_name="x")),
        edge_index=gs._get_edge_index(gs.get_all_edge_attrs()[0]),
        num_nodes=n,
    )

    torch.manual_seed(0)
    seeds = torch.randperm(n)[:200]

    loader = NeighborLoader(
        data=data,
        num_neighbors=[10, 5],
        batch_size=32,
        input_nodes=seeds,
    )

    print("\n── materialized Data path ──")
    for i, batch in enumerate(loader):
        print(f"  batch {i}: nodes={batch.num_nodes}  edges={batch.num_edges}  "
              f"x={tuple(batch.x.shape)}  edge_index={tuple(batch.edge_index.shape)}")
        check_features(batch, fs)
        if i >= 2:
            break
    return True


def check_tuple_path(ei, n):
    """PyG's (FeatureStore, GraphStore) remote-backend path."""
    fs = make_feature_store(n)
    gs = StubGraphStore(ei, n)

    torch.manual_seed(0)
    seeds = torch.randperm(n)[:200]

    loader = NeighborLoader(
        data=(fs, gs),
        num_neighbors=[10, 5],
        batch_size=32,
        input_nodes=seeds,
    )

    print("\n── tuple (fs, gs) path ──")
    try:
        for i, batch in enumerate(loader):
            print(f"  batch {i}: nodes={batch.num_nodes}  edges={batch.num_edges}  "
                  f"x={tuple(batch.x.shape)}")
            check_features(batch, fs)
            if i >= 2:
                break
        print("  ✅ tuple path works")
        return True
    except Exception as e:
        print(f"  ❌ tuple path failed: {type(e).__name__}: {e}")
        return False


# ─── main ──────────────────────────────────────────────────────────────

def main():
    print(f"edge source: {EDGE_SOURCE}")
    if EDGE_SOURCE == "raw":
        print(f"  raw path: {RAW_EDGES_PATH}")
    else:
        print(f"  db_dir:   {DB_DIR}")
        print(f"  db_name:  {DB_NAME}")
        print(f"  type:     {DB_TYPE}")

    ei, n = load_edges_from_config()
    print(f"  n={n}  |E|={ei.shape[1]}")

    ok_data = check_materialized(ei, n)
    ok_tuple = check_tuple_path(ei, n)

    print()
    if ok_data:
        print("✅ materialized Data path works")
    if ok_tuple:
        print("✅ tuple (fs, gs) path works")

    return 0 if (ok_data and ok_tuple) else 1


if __name__ == "__main__":
    sys.exit(main())