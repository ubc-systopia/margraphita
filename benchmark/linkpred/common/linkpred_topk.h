// linkpred_topk.h — bounded top-k for scored candidate pairs.
// Keeps the k highest-scoring pairs in a min-heap (O(k) memory, O(n log k) time).

#pragma once
#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <queue>
#include <vector>

#include "common_defs.h"   // node_id_t

namespace linkpred {

struct ScoredPair {
    node_id_t u;
    node_id_t v;
    double    score;
    int       label;   // 1 = positive, 0 = negative
};

// Total order used everywhere a pair is ranked: higher score first, ties broken
// by smaller (u, v). Without the tie-break, which tied pair survives depends on
// the order pairs are offered, so multi-threaded runs were not repeatable.
// Scores are rounded to 1e-9 when offered (quantize_score) so values that are
// equal up to floating-point rounding (e.g. cosine 1.0000000000000002 vs
// 0.9999999999999999 for identical embeddings) count as ties and fall to the
// (u, v) tie-break, instead of being ordered by rounding error. Added
// 2026-11-22 after the s12 brute-force check. Scores beyond +-1e6 are left as is.
inline double quantize_score(double s)
{
    return (std::fabs(s) < 1e6) ? std::nearbyint(s * 1e9) / 1e9 : s;
}

inline bool ranks_higher(const ScoredPair &a, const ScoredPair &b)
{
    if (a.score != b.score) return a.score > b.score;
    if (a.u != b.u)         return a.u < b.u;
    return a.v < b.v;
}

struct ScoredPairByScoreDesc {
    bool operator()(const ScoredPair &a, const ScoredPair &b) const {
        return ranks_higher(a, b);   // heap top = the lowest-ranked kept pair
    }
};

// Tie counts at the final K-th score, filled by merge_topk (see TopK::ties_).
// valid = false when the run did not go through merge_topk (e.g. some reference paths).
struct TieStats {
    bool   valid = false;
    double cutoff = 0.0;      // the K-th (last kept) score
    size_t at_cutoff = 0;     // pairs scored exactly `cutoff`, kept or not
    size_t pos_at_cutoff = 0; // ... of which are positives (label 1)
    size_t above = 0;         // kept pairs scoring strictly above `cutoff`
    size_t pos_above = 0;     // ... of which are positives
};
inline TieStats g_last_tie_stats;   // set by merge_topk; read by the bench after a run

class TopK {
    
public:
    explicit TopK(size_t k_) : k(k_) { assert(k > 0); }   // k must be > 0

    void offer(ScoredPair p) {
        p.score = quantize_score(p.score);         // near-equal scores tie (see quantize_score)
        offered_++;                                // count every pair seen, not just the ones kept
        // Tie bookkeeping (added 2026-11-22): count (pairs, positives) per score for
        // scores at or above this heap's current K-th score. Equal scores share one
        // entry, so a million pairs tied at 1.0 are one map row; rows below the
        // cutoff are dropped as it rises, so the map stays at about K rows.
        if (heap.size() < k || p.score >= heap.top().score) {
            auto &c = ties_[p.score];
            c.first++;
            if (p.label == 1) c.second++;
        }
        if (heap.size() < k) {                     // heap not yet full
            heap.push(p);
        } else if (ranks_higher(p, heap.top())) {  // beats current minimum
            heap.pop();
            heap.push(p);
        }
        if (ties_.size() > 2 * k + 8) prune_ties();   // keep the tie map O(k)
    }

    // Drop tie rows below the current K-th score (only once the heap is full).
    void prune_ties() {
        if (heap.size() < k || ties_.empty()) return;
        ties_.erase(ties_.begin(), ties_.lower_bound(heap.top().score));
    }
    // (pairs, positives) offered with exactly this score, or {0, 0} if not tracked.
    std::pair<size_t, size_t> tie_count(double score) const {
        auto it = ties_.find(score);
        return it == ties_.end() ? std::pair<size_t, size_t>{0, 0} : it->second;
    }

    // Return kept pairs in descending score order, emptying the collector.
    std::vector<ScoredPair> drain_sorted() {
        std::vector<ScoredPair> out;
        out.reserve(heap.size());
        while (!heap.empty()) {
            out.push_back(heap.top());   // heap order is min-first; sort below
            heap.pop();
        }
        std::sort(out.begin(), out.end(), ranks_higher);
        return out;
    }

    size_t size()    const { return heap.size(); }
    size_t offered() const { return offered_; }
    size_t k_value() const { return k; }

private:
    size_t k;
    std::priority_queue<ScoredPair,
                        std::vector<ScoredPair>,
                        ScoredPairByScoreDesc> heap;
    size_t offered_ = 0;
    std::map<double, std::pair<size_t, size_t>> ties_;   // score -> (pairs, positives), >= K-th score
};

// Drain every per-thread heap and keep the global best k, in ranked order.
inline std::vector<ScoredPair> merge_topk(std::vector<TopK> &per_thread, size_t k)
{
    std::vector<ScoredPair> merged;
    for (auto &t : per_thread)
        for (const auto &sp : t.drain_sorted())
            merged.push_back(sp);
    std::sort(merged.begin(), merged.end(), ranks_higher);
    if (merged.size() > k) merged.resize(k);

    // Tie counts at the final cutoff. Every thread's own K-th score is <= the global
    // one, so no thread has pruned the global cutoff's row.
    TieStats ts;
    if (!merged.empty()) {
        ts.valid  = true;
        ts.cutoff = merged.back().score;
        for (const auto &sp : merged)
            if (sp.score > ts.cutoff) { ts.above++; if (sp.label == 1) ts.pos_above++; }
        for (const auto &t : per_thread) {
            auto c = t.tie_count(ts.cutoff);
            ts.at_cutoff += c.first; ts.pos_at_cutoff += c.second;
        }
    }
    g_last_tie_stats = ts;
    return merged;
}

// Metrics over the drained list (precision_at_k, recall_at_k) live in linkpred_eval.h.

} // namespace linkpred