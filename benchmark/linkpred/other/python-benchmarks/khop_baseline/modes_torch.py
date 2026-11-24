"""Plain PyTorch modes (no PyG): sparse_chunked (row-chunked A[rows] @ A), fanout_manual, uniform, oracle.

Every run_* returns (pairs, edges, top_k, timings_dict); gen_score is fused for every
mode except uniform, where generation and scoring are separate loops.
"""
from .common import *  # noqa: F401,F403  (helpers, time, torch, ...)


def run_sparse(A, n, cand, chunk, k):
    """Row-chunked A[rows] @ A. cand='fof' keeps A·A, 'khop2' adds A.
    Generation and scoring are FUSED — A@A produces candidate indices and
    their CN values simultaneously (this mirrors the C++ fof_wedge/structural
    paths; splitting would double the work).
    torch.sparse.mm: https://pytorch.org/docs/stable/generated/torch.sparse.mm.html
    scatter_reduce:  https://pytorch.org/docs/stable/generated/torch.Tensor.scatter_reduce_.html
    """
    t0 = time.perf_counter()
    us, vs, cn, lab = [], [], [], []
    for r0 in range(0, n, chunk):
        rows = torch.arange(r0, min(r0 + chunk, n))
        Ar = A.index_select(0, rows).coalesce()
        A2 = torch.sparse.mm(Ar, A).coalesce()
        i, j = A2.indices()
        c = A2.values()
        e_i, e_j = Ar.indices()
        if cand == "khop2":
            i = torch.cat([i, e_i]); j = torch.cat([j, e_j])
            c = torch.cat([c, torch.zeros(e_i.numel())])
            key, inv = torch.unique(i * n + j, return_inverse=True)
            c = torch.zeros(key.numel()).scatter_reduce(0, inv, c, "amax")
            i, j = key // n, key % n
        u = i + r0
        keep = j > u
        u, j, c = u[keep], j[keep], c[keep]
        edge_key = set(((e_i + r0) * n + e_j).tolist())
        label = torch.tensor([1 if x in edge_key else 0 for x in (u * n + j).tolist()],
                             dtype=torch.int64)
        us.append(u); vs.append(j); cn.append(c); lab.append(label)
    u_t = torch.cat(us); v_t = torch.cat(vs)
    c = torch.cat(cn); l = torch.cat(lab)
    t1 = time.perf_counter()
    top = topk_pairs(u_t, v_t, c, l, k)
    t2 = time.perf_counter()
    return int(u_t.numel()), int(l.sum()), top, mk_timings(t0, t1, t2)


def run_fanout(ei, n, s1, s2, k, seed):
    """Manual fan-out sampling: per u keep ≤ s1 neighbours, ≤ s2 of each."""
    g = torch.Generator().manual_seed(seed)
    t0 = time.perf_counter()
    nbr = [[] for _ in range(n)]
    for a, b in ei.t().tolist():
        nbr[a].append(b)
    nset_local = [set(x) for x in nbr]
    def sample(lst, s):
        if len(lst) <= s:
            return lst
        idx = torch.randperm(len(lst), generator=g)[:s].tolist()
        return [lst[i] for i in idx]
    us, vs, cn, lab = [], [], [], []
    for u in range(n):
        cand = set()
        for w in sample(nbr[u], s1):
            cand.add(w)
            cand.update(sample(nbr[w], s2))
        for v in cand:
            if v > u:
                us.append(u); vs.append(v)
                cn.append(len(nset_local[u] & nset_local[v]))
                lab.append(1 if v in nset_local[u] else 0)
    u_t = torch.tensor(us, dtype=torch.int64)
    v_t = torch.tensor(vs, dtype=torch.int64)
    c = torch.tensor(cn, dtype=torch.float32)
    l = torch.tensor(lab, dtype=torch.int64)
    t1 = time.perf_counter()
    top = topk_pairs(u_t, v_t, c, l, k)
    t2 = time.perf_counter()
    return int(u_t.numel()), int(l.sum()), top, mk_timings(t0, t1, t2)


def run_uniform(ei, n, K, seed, k_topk, nset):
    """Fixed-K per source, samples from (u, n-1], no edge rejection.
    Mirrors C++ uniformRandom_candidates exactly.

    UNIQUE among modes: gen (RNG) and score (CN per pair) are truly
    independent loops → we report gen_ms and score_ms separately in addition
    to gen_score_ms. For all other modes gen+score are fused.

    Field reference for uniform negative sampling (pair-shaped, not per-source):
      https://pytorch-geometric.readthedocs.io/en/stable/modules/utils.html#torch_geometric.utils.negative_sampling
    """
    gen = torch.Generator().manual_seed(seed)
    t0 = time.perf_counter()

    # --- generation phase (RNG only)
    all_pairs = []
    for u in range(n):
        span = n - 1 - u
        if span <= 0:
            continue
        want = min(K, span)
        max_tries = 4 * want + 64
        draws = torch.randint(u + 1, n, (max_tries,), generator=gen).tolist()
        picks = set()
        for v in draws:
            picks.add(v)
            if len(picks) >= want:
                break
        all_pairs.append((u, picks))
    t_gen_end = time.perf_counter()

    # --- scoring phase (CN per pair)
    us, vs, cn, lab = [], [], [], []
    for u, picks in all_pairs:
        for v in picks:
            us.append(u); vs.append(v)
            cn.append(len(nset[u] & nset[v]))
            lab.append(1 if v in nset[u] else 0)
    t_score_end = time.perf_counter()

    # --- ranking phase
    u_t = torch.tensor(us, dtype=torch.int64)
    v_t = torch.tensor(vs, dtype=torch.int64)
    c = torch.tensor(cn, dtype=torch.float32)
    l = torch.tensor(lab, dtype=torch.int64)
    top = topk_pairs(u_t, v_t, c, l, k_topk)
    t_rank_end = time.perf_counter()

    return (int(u_t.numel()), int(l.sum()), top,
            mk_timings(t0, t_score_end, t_rank_end,
                       gen_ms=(t_gen_end - t0) * 1e3,
                       score_ms=(t_score_end - t_gen_end) * 1e3))


def run_oracle(ei, n, nset, k):
    """Read in all positive edges as candidates — perfect recall by
    construction. Pair-centric; no generation phase, score+rank only."""
    t0 = time.perf_counter()
    u, v = ei[0], ei[1]
    keep = v > u
    pairs, edges, top = score_pairs(u[keep], v[keep], nset, k)
    t1 = time.perf_counter()
    return pairs, edges, top, mk_timings(t0, t1, t1)
