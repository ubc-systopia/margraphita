"""backend — PyG GraphStore and FeatureStore implementations for FlexoGraph.

Exports:
  load_edges              read edges from a raw text file
  load_edges_from_flexo   read edges from a FlexoGraph DB via dump_edges
  StubFeatureStore        minimal random-feature FeatureStore
  StubGraphStore          minimal COO-edge-index GraphStore
"""
from .edge_loader import load_edges, load_edges_from_flexo
from .stub_feature_store import StubFeatureStore
from .stub_graph_store import StubGraphStore

__all__ = [
    "load_edges",
    "load_edges_from_flexo",
    "StubFeatureStore",
    "StubGraphStore",
]