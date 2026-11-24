// linkpred_eval.h — evaluation metrics over a drained, score-sorted top-k list.

#pragma once
#include <algorithm>
#include <cstddef>
#include <vector>

#include "linkpred_topk.h"   // ScoredPair

namespace linkpred {

// Precision@k over a drained sorted list.
inline double precision_at_k(const std::vector<ScoredPair> &top, size_t k)
{
    size_t kk = std::min(k, top.size());   // clamp k to available rows
    if (kk == 0) return 0.0;
    size_t hits = 0;
    for (size_t i = 0; i < kk; ++i)
        if (top[i].label == 1) hits++;
    return (double)hits / (double)kk;
}

// Recall@k over a drained sorted list.
inline double recall_at_k(const std::vector<ScoredPair> &top,
                          size_t k, size_t total_positives)
{
    if (total_positives == 0) return 0.0;
    size_t kk = std::min(k, top.size());
    size_t hits = 0;
    for (size_t i = 0; i < kk; ++i)
        if (top[i].label == 1) hits++;
    return (double)hits / (double)total_positives;
}

// Tie-aware precision@k (McSherry & Najork 2008): when the K-th score is shared by
// more pairs than the slots left, every ordering of the tied group is equally
// likely, so the expected hits in those slots = slots x (positives / size) of the
// tied group. Independent of the tie-break rule. Falls back to precision_at_k when
// the tie counts are not available.
inline double precision_at_k_tie_aware(const std::vector<ScoredPair> &top, size_t k,
                                       const TieStats &ts)
{
    size_t kk = std::min(k, top.size());
    if (kk == 0) return 0.0;
    if (!ts.valid || ts.at_cutoff == 0 || top.size() < k) return precision_at_k(top, k);
    size_t slots = kk - ts.above;                      // places filled from the tied group
    double tied_hits = (double)slots * (double)ts.pos_at_cutoff / (double)ts.at_cutoff;
    return ((double)ts.pos_above + tied_hits) / (double)kk;
}

// TODO: could add MRR, NDCG@k, AUC, AP later.

} // namespace linkpred
