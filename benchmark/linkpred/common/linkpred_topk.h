// linkpred_topk.h — bounded top-k for scored candidate pairs.
// Keeps the k highest-scoring pairs in a min-heap (O(k) memory, O(n log k) time).

#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
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

class TopK {
public:
    explicit TopK(size_t k_) : k(k_) { assert(k > 0); }   // k must be > 0

    void offer(const ScoredPair &p) {
        offered_++;                                // count every pair seen, not just the ones kept
        if (heap.size() < k) {                     // heap not yet full
            heap.push(p);
        } else if (ranks_higher(p, heap.top())) {  // beats current minimum
            heap.pop();
            heap.push(p);
        }
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
    return merged;
}

// Metrics over the drained list (precision_at_k, recall_at_k) live in linkpred_eval.h.

} // namespace linkpred