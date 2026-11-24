"""NetworKit mode: candidates with MissingLinksFinder, scores with its link-prediction indices.

Follows NetworKit's link-prediction notebook
(networkit.github.io/dev-docs/nightly/notebooks/LinkPrediction.html):
  MissingLinksFinder(G).findAtDistance(2)  -> non-adjacent pairs at distance 2
  CommonNeighborsIndex(G).runOn(pairs)     -> [((u, v), score), ...]
  JaccardIndex(G).runOn(pairs)             -> same, Jaccard
NetworKit is C++ with OpenMP; threads follow --threads (nk.setNumberOfThreads).

Candidate rules, to match the C++ bench (v > u):
  khop2  edges (distance 1) + pairs at distance 2
  fof    pairs with at least one common neighbour: distance-2 pairs + edges with CN > 0
Undirected only (MissingLinksFinder works on the undirected graph).
"""
from .common import *  # noqa: F401,F403  (helpers, time, torch, ...)


def run_networkit(ei, n, cand, k, method="cn", threads=0):
    """Returns (pairs, edges, top_k, timings) like every run_*."""
    import networkit as nk
    if threads:
        nk.setNumberOfThreads(threads)
    t0 = time.perf_counter()

    # --- graph: ei holds both directions (undirected load); add each edge once
    src, dst = ei[0], ei[1]
    once = src < dst
    G = nk.Graph(n, directed=False)
    G.addEdges((src[once].numpy(), dst[once].numpy()))

    # --- candidates
    dist2 = nk.linkprediction.MissingLinksFinder(G).findAtDistance(2)
    edges_uv = list(zip(src[once].tolist(), dst[once].tolist()))
    pairs = [(min(u, v), max(u, v)) for u, v in dist2] + edges_uv

    # --- scores
    index = (nk.linkprediction.JaccardIndex(G) if method == "jaccard"
             else nk.linkprediction.CommonNeighborsIndex(G))
    scored = index.runOn(pairs)                         # [((u, v), score), ...], order not kept
    if cand == "fof":                                   # edges count only with a common neighbour
        cn_edges = nk.linkprediction.CommonNeighborsIndex(G).runOn(edges_uv)
        no_cn = set((min(p), max(p)) for p, c in cn_edges if c == 0)
        scored = [(p, sc) for p, sc in scored if (min(p), max(p)) not in no_cn]

    u_t = torch.tensor([min(p) for p, _ in scored], dtype=torch.int64)
    v_t = torch.tensor([max(p) for p, _ in scored], dtype=torch.int64)
    c = torch.tensor([s for _, s in scored], dtype=torch.float32)
    l = torch.tensor([1 if G.hasEdge(int(a), int(b)) else 0 for a, b in zip(u_t.tolist(), v_t.tolist())],
                     dtype=torch.int64)
    t1 = time.perf_counter()
    top = topk_pairs(u_t, v_t, c, l, k)
    t2 = time.perf_counter()
    return int(u_t.numel()), int(l.sum()), top, mk_timings(t0, t1, t2)


def run_networkit_stream(ei, n, cand, k, method="cn", threads=0, batch_pairs=2_000_000):
    """--mode networkit_stream: same NetworKit calls as run_networkit, streamed.

    run_networkit materialises every candidate pair as Python tuples (dist-2
    list, pairs list, runOn result): ~100+ B per pair per copy, so s16
    (36.6M pairs) is near the 15 GB WSL limit and s17 (85M) does not fit.
    Here each node u contributes MissingLinksFinder.findFromNode(u, 2) (pairs
    at distance exactly 2) plus its edges (u, w), keeping v > u; nodes are
    batched until ~batch_pairs pairs, each batch is scored with one runOn call
    (OpenMP inside NetworKit), and an exact running top-k is kept (same order
    as the C++ heap: score desc, then u, v asc). Memory is one batch.

    timings: gen_ms = time inside findFromNode, score_ms = time inside runOn
    (both NetworKit C++, incl. the Python <-> C++ list conversion of the call);
    total_ms - gen_ms - score_ms is the Python loop / batching / top-k overhead.
    Same candidate rules as run_networkit (khop2: edges + distance-2; fof:
    distance-2 + edges with CN > 0). Undirected only.
    """
    import networkit as nk
    if threads:
        nk.setNumberOfThreads(threads)
    t0 = time.perf_counter()
    src, dst = ei[0], ei[1]
    once = src < dst
    G = nk.Graph(n, directed=False)
    G.addEdges((src[once].numpy(), dst[once].numpy()))
    mlf = nk.linkprediction.MissingLinksFinder(G)
    index = (nk.linkprediction.JaccardIndex(G) if method == "jaccard"
             else nk.linkprediction.CommonNeighborsIndex(G))
    cn_index = nk.linkprediction.CommonNeighborsIndex(G)

    gen_s = score_s = 0.0
    n_pairs = n_edges = 0
    top = []                                        # current top-k as (u, v, score, label)
    batch, labels = [], []

    def flush():
        nonlocal score_s, n_pairs, n_edges, top
        if not batch:
            return
        ts = time.perf_counter()
        scored = index.runOn(batch)                 # order not kept; match labels by key
        drop = set()
        if cand == "fof":                           # edges count only with a common neighbour
            e_pairs = [p for p, l in zip(batch, labels) if l]
            if e_pairs:
                drop = set(p for p, c in cn_index.runOn(e_pairs) if c == 0)
        score_s += time.perf_counter() - ts
        lab = dict(zip(batch, labels))
        uu, vv, ss, ll = [], [], [], []
        for (a, b), sc in scored:
            p = (min(a, b), max(a, b))
            if p in drop:
                continue
            uu.append(p[0]); vv.append(p[1]); ss.append(sc); ll.append(lab[p])
        n_pairs += len(uu)
        n_edges += sum(ll)
        for (a, b, sc, l) in top:                   # merge the running top-k back in
            uu.append(a); vv.append(b); ss.append(sc); ll.append(l)
        top = topk_pairs(torch.tensor(uu, dtype=torch.int64), torch.tensor(vv, dtype=torch.int64),
                         torch.tensor(ss, dtype=torch.float32), torch.tensor(ll, dtype=torch.int64), k)
        batch.clear(); labels.clear()

    for u in range(n):
        tg = time.perf_counter()
        d2 = mlf.findFromNode(u, 2)                 # [(u, v), ...] at distance exactly 2
        gen_s += time.perf_counter() - tg
        for a, b in d2:
            v = b if a == u else a
            if v > u:
                batch.append((u, v)); labels.append(0)
        for w in G.iterNeighbors(u):
            if w > u:
                batch.append((u, w)); labels.append(1)
        if len(batch) >= batch_pairs:
            flush()
    flush()
    t1 = time.perf_counter()
    # top-k already final; rank time is folded into the batches
    return n_pairs, n_edges, top, mk_timings(t0, t1, t1, gen_ms=gen_s * 1e3, score_ms=score_s * 1e3)
