"""backend — PyG GraphStore and FeatureStore implementations for FlexoGraph.

Exports:
  load_edges              read edges from a raw text file
  load_edges_from_flexo   read edges from a FlexoGraph DB via dump_edges
  StubFeatureStore        minimal random-feature FeatureStore
  FlexoFeatureStore       real node features (embeddings) exported from a FlexoGraph DB
  export_features_from_flexo  write a DB's node features to .npy via dump_edges
  StubGraphStore          minimal COO-edge-index GraphStore
"""
from .edge_loader import load_edges, load_edges_from_flexo, export_features_from_flexo
from .stub_feature_store import StubFeatureStore, FlexoFeatureStore
from .stub_graph_store import StubGraphStore

__all__ = [
    "load_edges",
    "load_edges_from_flexo",
    "StubFeatureStore",
    "FlexoFeatureStore",
    "export_features_from_flexo",
    "StubGraphStore",
]