"""khop_baseline: in-memory Python baselines for linkpred candidate generation, split by library.

  common.py          results writer, graph loading, scoring helpers (shared)
  modes_pyg.py       PyG: twohop, khop_subgraph, fanout (NeighborLoader)
  modes_torch.py     plain PyTorch tensors: sparse_chunked, fanout_manual, uniform, oracle
  modes_networkx.py  NetworkX: networkx
  modes_networkit.py NetworKit: networkit
  modes_cugraph.py   cuGraph (GPU): cugraph_native
  ogb_eval.py        OGB-style evaluation against a fixed negative set

The CLI is ../py_khop_baseline.py.
"""

