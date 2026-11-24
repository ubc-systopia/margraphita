"""PyTorch Geometric modes: twohop (TwoHop transform), khop_subgraph (k_hop_subgraph), fanout (NeighborLoader).

Every run_* returns (pairs, edges, top_k, timings_dict).
"""
from .common import *  # noqa: F401,F403  (helpers, time, torch, ...)


def run_twohop(ei, n, nset, k):
    """PyG TwoHop (A·A) ∪ A → khop2 candidates.
    TwoHop docs: https://pytorch-geometric.readthedocs.io/en/stable/generated/torch_geometric.transforms.TwoHop.html
    """
    from torch_geometric.data import Data
    import torch_geometric.transforms as T
    t0 = time.perf_counter()
    out = T.TwoHop()(Data(edge_index=ei.clone(), num_nodes=n))
    ei_out = torch.cat([ei, out.edge_index], dim=1)
    key = torch.unique(ei_out[0] * n + ei_out[1])
    u, v = key // n, key % n
    keep = v > u
    pairs, edges, top = score_pairs(u[keep], v[keep], nset, k)
    t1 = time.perf_counter()
    # ranking already happened inside score_pairs; charge it to rank
    t2 = t1
    return pairs, edges, top, mk_timings(t0, t1, t2)


def run_pyg(ei, n, nset, k, directed=False):
    """Per-source k_hop_subgraph(u, 2) then CN by set intersection.
    k_hop_subgraph docs: https://pytorch-geometric.readthedocs.io/en/stable/modules/utils.html#torch_geometric.utils.k_hop_subgraph
    SEAL_OGB uses the same primitive for LP: https://github.com/facebookresearch/SEAL_OGB
    directed=True, flow="target_to_source" → out-neighbours (matches get_out_nodes_id).
    """
    from torch_geometric.utils import k_hop_subgraph
    t0 = time.perf_counter()
    us, vs, cn, lab = [], [], [], []
    for u in range(n):
        if directed:
            subset, _, _, _ = k_hop_subgraph(u, 2, ei, num_nodes=n,
                                             directed=True, flow="target_to_source")
        else:
            subset, _, _, _ = k_hop_subgraph(u, 2, ei, num_nodes=n)
        for v in subset.tolist():
            if v > u:
                us.append(u); vs.append(v)
                cn.append(len(nset[u] & nset[v]))
                lab.append(1 if v in nset[u] else 0)
    u_t = torch.tensor(us, dtype=torch.int64)
    v_t = torch.tensor(vs, dtype=torch.int64)
    c = torch.tensor(cn, dtype=torch.float32)
    l = torch.tensor(lab, dtype=torch.int64)
    t1 = time.perf_counter()
    top = topk_pairs(u_t, v_t, c, l, k)
    t2 = time.perf_counter()
    return int(u_t.numel()), int(l.sum()), top, mk_timings(t0, t1, t2)


def run_fanout_pyg(ei, n, nset, s1, s2, k, batch, seed):
    """PyG NeighborLoader, disjoint per seed (requires pyg-lib).
    NeighborLoader docs: https://pytorch-geometric.readthedocs.io/en/stable/modules/loader.html#torch_geometric.loader.NeighborLoader
    """
    try:
        from torch_geometric.data import Data
        from torch_geometric.loader import NeighborLoader
    except ImportError as e:
        raise SystemExit(f"fanout requires pyg-lib: {e}")
    t0 = time.perf_counter()
    torch.manual_seed(seed)
    loader = NeighborLoader(Data(edge_index=ei, num_nodes=n), num_neighbors=[s1, s2],
                            batch_size=batch, shuffle=False, disjoint=True)
    keys = []
    for b in loader:
        seeds = b.n_id[:b.batch_size]
        u = seeds[b.batch]; v = b.n_id
        keys.append(u * n + v)
    key = torch.unique(torch.cat(keys))
    u, v = key // n, key % n
    keep = v > u
    pairs, edges, top = score_pairs(u[keep], v[keep], nset, k)
    t1 = time.perf_counter()
    return pairs, edges, top, mk_timings(t0, t1, t1)
