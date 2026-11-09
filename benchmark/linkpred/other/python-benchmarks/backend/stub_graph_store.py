#!/usr/bin/env python3
"""stub_graph_store.py — minimal PyG GraphStore wrapping a COO edge_index.

Proves the GraphStore contract:
  - _get_edge_index(attr)  return [2, |E|] int64 COO tensor
  - get_all_edge_attrs()   advertise edge types, layout, size
  - csc()                  column-compressed form for NeighborSampler

Notes on csc():
  NeighborLoader((fs, gs)) makes PyG's NeighborSampler call graph_store.csc()
  and unpack it as `row, colptr, perm` (row first). For a homogeneous graph
  (edge_type=None) these are plain tensors, not per-edge-type dicts.

Refs:
  GraphStore:    https://pytorch-geometric.readthedocs.io/en/stable/modules/data.html#torch_geometric.data.GraphStore
  EdgeAttr:      https://pytorch-geometric.readthedocs.io/en/stable/modules/data.html#torch_geometric.data.EdgeAttr
  EdgeLayout:    https://pytorch-geometric.readthedocs.io/en/stable/modules/data.html#torch_geometric.data.EdgeLayout
"""
import torch
from torch_geometric.data.graph_store import (
    GraphStore, EdgeAttr, EdgeLayout,
)


class StubGraphStore(GraphStore):
    """COO edge_index in RAM. Read-only."""

    EDGE_TYPE = None          # homogeneous graph

    def __init__(self, edge_index: torch.Tensor, n: int):
        super().__init__()
        self.edge_index = edge_index          # [2, |E|] int64
        self.n = n
        self._attrs = [EdgeAttr(
            edge_type=self.EDGE_TYPE,
            layout=EdgeLayout.COO,
            is_sorted=False,
            size=(n, n),
        )]

    # ─── required by GraphStore ABC
    def _get_edge_index(self, edge_attr: EdgeAttr):
        return self.edge_index

    def _put_edge_index(self, *a):
        raise NotImplementedError("StubGraphStore is read-only")

    def _remove_edge_index(self, *a):
        raise NotImplementedError("StubGraphStore is read-only")

    def get_all_edge_attrs(self):
        return self._attrs

    # ─── called by NeighborSampler for the tuple (fs, gs) path
    def csc(self):
        """Return (row, colptr, perm), the order NeighborSampler unpacks.

        row:    source node of each edge, edges sorted by destination
        colptr: [n + 1] offsets into row per destination node
        perm:   original edge index of each sorted edge
        """
        row, col = self.edge_index
        order = torch.argsort(col, stable=True)
        row_sorted = row[order]
        col_sorted = col[order]

        counts = torch.bincount(col_sorted, minlength=self.n)
        colptr = torch.zeros(self.n + 1, dtype=torch.int64)
        colptr[1:] = torch.cumsum(counts, dim=0)

        return row_sorted, colptr, order