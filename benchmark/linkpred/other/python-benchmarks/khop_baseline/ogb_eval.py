"""OGB-style evaluation: fixed negative set (as ogbl-*_negatives.pt) and Hits@K / MRR.
"""
from .common import *  # noqa: F401,F403  (helpers, time, torch, ...)


def gen_uniform_negatives(ei, n, nset, ratio=1, seed=42, max_attempts=100):
    """Generate a fixed negative set (mirrors OGB's ogbl-*_negatives.pt files).
    OGB linkprop: https://ogb.stanford.edu/docs/linkprop/"""
    gen = torch.Generator().manual_seed(seed)
    neg_u, neg_v = [], []
    for u, _v in zip(ei[0].tolist(), ei[1].tolist()):
        for _ in range(ratio):
            for _a in range(max_attempts):
                vv = int(torch.randint(0, n, (1,), generator=gen).item())
                if vv != u and vv not in nset[u]:
                    neg_u.append(u); neg_v.append(vv); break
    return neg_u, neg_v


def run_ogb_eval(pos_u, pos_v, neg_u, neg_v, score_fn, k=10):
    """OGB Hits@K / MRR against a fixed negative set.

    OGB ranking (ogb/linkproppred/evaluate.py, _eval_hits_and_mrr):
      optimistic_rank  = #negatives with score >  pos_score
      pessimistic_rank = #negatives with score >= pos_score
      ranking = 0.5*(opt + pess) + 1
    Vectorized via torch.searchsorted: O((P+M) log M) instead of O(P*M).

    OGB evaluator: https://github.com/snap-stanford/ogb/blob/master/ogb/linkproppred/evaluate.py
    OGB docs:      https://ogb.stanford.edu/docs/linkprop/
    searchsorted:  https://pytorch.org/docs/stable/generated/torch.searchsorted.html

    Returns (hits, mrr, mean_rank, timings_dict). No generation phase.
    """
    if not pos_u or not neg_u:
        return 0.0, 0.0, float("inf"), mk_timings(0, 0, 0)
    t0 = time.perf_counter()
    s_pos = torch.tensor([score_fn(u, v) for u, v in zip(pos_u, pos_v)],
                         dtype=torch.float32)
    s_neg = torch.tensor([score_fn(u, v) for u, v in zip(neg_u, neg_v)],
                         dtype=torch.float32)
    t1 = time.perf_counter()
    M = s_neg.numel()
    s_neg_sorted, _ = torch.sort(s_neg)
    pess = M - torch.searchsorted(s_neg_sorted, s_pos, right=False)
    opt = M - torch.searchsorted(s_neg_sorted, s_pos, right=True)
    ranking = 0.5 * (opt.float() + pess.float()) + 1.0
    hits = (ranking <= k).float().mean().item()
    mrr = (1.0 / ranking).mean().item()
    mean_rank = ranking.mean().item()
    t2 = time.perf_counter()
    return hits, mrr, mean_rank, mk_timings(t0, t1, t2)
