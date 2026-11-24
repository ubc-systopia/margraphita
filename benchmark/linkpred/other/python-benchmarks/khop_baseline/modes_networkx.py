"""NetworkX mode: BFS depth 2 + common_neighbors.
"""
from .common import *  # noqa: F401,F403  (helpers, time, torch, ...)


def run_networkx(ei, n, k, method="cn"):
    """NetworkX BFS depth 2 + common_neighbors / jaccard (naive CPU reference).

    method="cn"      → nx.common_neighbors, sum of shared neighbours
    method="jaccard" → nx.jaccard_coefficient, |∩| / |∪|
                       (matches cugraph.jaccard_coefficient exactly)

    Same candidate set either way — Jaccard's support equals CN's support.
    Refs:
      BFS:                https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.shortest_paths.unweighted.single_source_shortest_path_length.html
      common_neighbors:   https://networkx.org/documentation/stable/reference/generated/networkx.classes.function.common_neighbors.html
      jaccard_coefficient: https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.link_prediction.jaccard_coefficient.html
    """
    import networkx as nx
    t0 = time.perf_counter()
    G = nx.Graph()
    G.add_nodes_from(range(n))
    G.add_edges_from(ei.t().tolist())

    # --- candidates: BFS depth 2 per source (v > u, dist 1 or 2)
    us, vs = [], []
    for u in range(n):
        dist = nx.single_source_shortest_path_length(G, u, cutoff=2)
        for v, d in dist.items():
            if v > u and d >= 1:
                us.append(u); vs.append(v)
    pairs = list(zip(us, vs))

    # --- scoring: dispatch on --method
    if method == "jaccard":
        scores = [p for _, _, p in nx.jaccard_coefficient(G, pairs)]
    else:  # cn
        scores = [sum(1 for _ in nx.common_neighbors(G, u, v))
                  for u, v in pairs]

    c = torch.tensor(scores, dtype=torch.float32)
    l = torch.tensor([1 if G.has_edge(u, v) else 0 for u, v in pairs],
                     dtype=torch.int64)
    u_t = torch.tensor(us, dtype=torch.int64)
    v_t = torch.tensor(vs, dtype=torch.int64)
    t1 = time.perf_counter()
    top = topk_pairs(u_t, v_t, c, l, k)
    t2 = time.perf_counter()
    return int(u_t.numel()), int(l.sum()), top, mk_timings(t0, t1, t2)
