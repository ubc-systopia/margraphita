"""cuGraph (GPU) mode: cugraph_native.
"""
from .common import *  # noqa: F401,F403  (helpers, time, torch, ...)


def run_cugraph_native(edges_path, cand, k, directed=False):
    """Native cuGraph: whole graph on GPU, candidates in one batched pass,
    Jaccard scoring in another. No per-pair dispatch.

    cand='fof'   → G.get_two_hop_neighbors()  (2-hop only)
    cand='khop2' → fof ∪ direct edges          (distance 1 or 2, matches C++)

    Always Jaccard-scored (cugraph.jaccard_coefficient). Pairs and true_edges
    match CN-scored runs exactly — only the score values differ (normalized).

    Refs:
      cugraph.jaccard_coefficient: https://docs.rapids.ai/api/cugraph/stable/api_docs/api/cugraph/cugraph.dgl.LinkPred.jaccard_coefficient.html
      cugraph.Graph.get_two_hop_neighbors: https://docs.rapids.ai/api/cugraph/stable/api_docs/api/cugraph/cugraph.structure.graph_classes.Graph.get_two_hop_neighbors.html
    """
    import cudf, cugraph
    t0 = time.perf_counter()

    # --- build graph on GPU (whole graph → VRAM)
    e = cudf.read_csv(edges_path, sep=r"\s+", header=None,
                      names=["src", "dst"], dtype="int32")
    if not directed:
        # symmetrize: cuGraph undirected graphs already treat edges as bidirectional,
        # so no need to duplicate. If `directed=True`, use cugraph.Graph(directed=True).
        pass
    G = cugraph.Graph(directed=directed)
    G.from_cudf_edgelist(e, source="src", destination="dst")

    # --- candidates
    two_hop = G.get_two_hop_neighbors()                 # fof set: columns [first, second]
    if cand == "khop2":
        direct = e.rename(columns={"src": "first", "dst": "second"})[["first", "second"]]
        pairs = cudf.concat([two_hop, direct]).drop_duplicates()
    else:  # fof
        pairs = two_hop

    # filter v > u (count each unordered pair once)
    pairs = pairs[pairs["second"] > pairs["first"]].reset_index(drop=True)

    # --- scoring: one GPU pass over all pairs
    scored = cugraph.jaccard_coefficient(G, pairs)     # columns: first, second, jaccard_coeff
    t1 = time.perf_counter()

    # --- labels + top-k (bring back to CPU for the heap)
    first  = scored["first"].to_numpy()
    second = scored["second"].to_numpy()
    score  = scored["jaccard_coeff"].to_numpy()

    # label = 1 iff (first, second) is an existing edge in the ORIGINAL edge list
    edge_pairs = set(zip(e["src"].to_pandas().tolist(),
                        e["dst"].to_pandas().tolist()))
    if not directed:
        edge_pairs |= set((b, a) for a, b in edge_pairs)
    labels = [1 if (int(u), int(v)) in edge_pairs else 0
              for u, v in zip(first, second)]

    u_t = torch.tensor(first, dtype=torch.int64)
    v_t = torch.tensor(second, dtype=torch.int64)
    c = torch.tensor(score, dtype=torch.float32)
    l = torch.tensor(labels, dtype=torch.int64)
    top = topk_pairs(u_t, v_t, c, l, k)
    t2 = time.perf_counter()
    return int(u_t.numel()), int(l.sum()), top, mk_timings(t0, t1, t2)
