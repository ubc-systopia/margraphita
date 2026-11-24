// test_linkpred_common.cpp — unit tests for linkpred_scores.h, linkpred_topk.h
// and linkpred_eval.h.
//
// No database. Only needs src/ on the include path (common_defs.h for node_id_t):
//   g++ -std=c++17 -DB64 -I../../src test_linkpred_common.cpp -o /tmp/test_linkpred_common
//   /tmp/test_linkpred_common
//
// Expects: "all linkpred tests passed"

// Release builds pass -DNDEBUG, which would turn every assert() below into a no-op.
#undef NDEBUG

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <utility>
#include <vector>

#include "linkpred_scores.h"
#include "linkpred_eval.h"
#include "linkpred_topk.h"

// ─── Toy graph for structural tests ─────────────────────────────────────────
//
// Undirected, stored as out-neighbour lists:
//   0 → 1, 2
//   1 → 0, 2, 3
//   2 → 0, 1, 4
//   3 → 1, 4
//   4 → 2, 3
static std::unordered_map<uint64_t, std::vector<uint64_t>> toy_graph = {
    {0, {1, 2}},
    {1, {0, 2, 3}},
    {2, {0, 1, 4}},
    {3, {1, 4}},
    {4, {2, 3}},
};

// Sorted copy of u's neighbour list; empty for missing nodes.
static std::vector<uint64_t> nbrs(uint64_t u)
{
    auto it = toy_graph.find(u);
    if (it == toy_graph.end()) return {};
    std::vector<uint64_t> v = it->second;
    std::sort(v.begin(), v.end());
    return v;
}

// ─── Structural score tests ────────────────────────────────────────────────

static void test_common_neighbors()
{
    // N(0)={1,2}, N(1)={0,2,3}; intersection {2} → 1
    assert(linkpred::common_neighbors(nbrs(0), nbrs(1)) == 1);
    // N(0)={1,2}, N(3)={1,4}; intersection {1} → 1
    assert(linkpred::common_neighbors(nbrs(0), nbrs(3)) == 1);
    // N(1)={0,2,3}, N(3)={1,4}; intersection {} → 0
    assert(linkpred::common_neighbors(nbrs(1), nbrs(3)) == 0);
    // N(0)={1,2}, N(4)={2,3}; intersection {2} → 1
    assert(linkpred::common_neighbors(nbrs(0), nbrs(4)) == 1);
    // N(2)={0,1,4}, N(4)={2,3}; intersection {} → 0
    assert(linkpred::common_neighbors(nbrs(2), nbrs(4)) == 0);
    // Self-intersection equals degree
    assert(linkpred::common_neighbors(nbrs(1), nbrs(1)) == 3);
    // Empty vs empty
    assert(linkpred::common_neighbors(nbrs(99), nbrs(98)) == 0);
    // Empty vs non-empty
    assert(linkpred::common_neighbors(nbrs(99), nbrs(1)) == 0);
}

static void test_jaccard()
{
    // Jaccard(0,1) = 1 / |{0,1,2,3}| = 0.25
    assert(std::abs(linkpred::jaccard(nbrs(0), nbrs(1)) - 0.25) < 1e-9);
    // Jaccard(0,3) = 1 / |{1,2,4}| = 1/3
    assert(std::abs(linkpred::jaccard(nbrs(0), nbrs(3)) - 1.0/3.0) < 1e-9);
    // Jaccard(1,3) = 0 / |{0,1,2,3,4}| = 0
    assert(std::abs(linkpred::jaccard(nbrs(1), nbrs(3)) - 0.0) < 1e-9);
    // Jaccard(v,v) = 1.0
    assert(std::abs(linkpred::jaccard(nbrs(1), nbrs(1)) - 1.0) < 1e-9);
    // Empty vs non-empty = 0
    assert(std::abs(linkpred::jaccard(nbrs(99), nbrs(1)) - 0.0) < 1e-9);
    // Empty vs empty = 0 (guard on uni > 0)
    assert(std::abs(linkpred::jaccard(nbrs(99), nbrs(98)) - 0.0) < 1e-9);
}

// ─── Embedding score tests ─────────────────────────────────────────────────

static void test_dot()
{
    // dot({1,2,3}, {4,5,6}) = 4 + 10 + 18 = 32
    assert(std::abs(linkpred::dot({1.0f,2.0f,3.0f}, {4.0f,5.0f,6.0f}) - 32.0) < 1e-9);
    // dot({1,0}, {0,1}) = 0 (orthogonal)
    assert(std::abs(linkpred::dot({1.0f,0.0f}, {0.0f,1.0f}) - 0.0) < 1e-9);
    // dot(v, v) = ||v||^2
    assert(std::abs(linkpred::dot({3.0f,4.0f}, {3.0f,4.0f}) - 25.0) < 1e-9);
    // dot with a zero vector = 0
    assert(std::abs(linkpred::dot({0.0f,0.0f}, {1.0f,2.0f}) - 0.0) < 1e-9);
    // Empty vectors
    std::vector<float> empty;
    assert(std::abs(linkpred::dot(empty, empty) - 0.0) < 1e-9);
    // Mismatched lengths: uses min(len) and ignores the tail
    assert(std::abs(linkpred::dot({1.0f,2.0f,3.0f}, {4.0f,5.0f}) - 14.0) < 1e-9);
}

static void test_cosine()
{
    // cosine(v, v) = 1.0 for non-zero v
    assert(std::abs(linkpred::cosine({3.0f,4.0f}, {3.0f,4.0f}) - 1.0) < 1e-9);
    // cosine({1,0}, {0,1}) = 0.0 (orthogonal)
    assert(std::abs(linkpred::cosine({1.0f,0.0f}, {0.0f,1.0f}) - 0.0) < 1e-9);
    // cosine({1,0}, {-1,0}) = -1.0 (opposite)
    assert(std::abs(linkpred::cosine({1.0f,0.0f}, {-1.0f,0.0f}) + 1.0) < 1e-9);
    // cosine scales: cosine(2a, b) = cosine(a, b)
    assert(std::abs(linkpred::cosine({2.0f,0.0f}, {1.0f,1.0f})
                    - linkpred::cosine({1.0f,0.0f}, {1.0f,1.0f})) < 1e-9);
    // cosine with a zero vector = 0.0 (guard)
    assert(std::abs(linkpred::cosine({0.0f,0.0f}, {1.0f,2.0f}) - 0.0) < 1e-9);
    // cosine of two zero vectors = 0.0 (guard)
    assert(std::abs(linkpred::cosine({0.0f,0.0f}, {0.0f,0.0f}) - 0.0) < 1e-9);
    // Known non-trivial case: cosine({1,1}, {1,0}) = 1/sqrt(2)
    assert(std::abs(linkpred::cosine({1.0f,1.0f}, {1.0f,0.0f})
                    - 1.0/std::sqrt(2.0)) < 1e-9);
}

// ─── Evaluation tests (AUC, AP, P@k) ───────────────────────────────────────

// static void test_auc()
// {
//     using P = std::pair<double,int>;

//     // Perfect ranking: all positives above all negatives → AUC = 1
//     assert(std::abs(linkpred::compute_auc({P{0.9,1}, P{0.8,1}, P{0.2,0}, P{0.1,0}}) - 1.0) < 1e-9);
//     // Perfectly wrong ranking: all negatives above all positives → AUC = 0
//     assert(std::abs(linkpred::compute_auc({P{0.9,0}, P{0.8,0}, P{0.2,1}, P{0.1,1}}) - 0.0) < 1e-9);
//     // One positive at the top of three negatives → 1 positive beats 3 → AUC = 1
//     assert(std::abs(linkpred::compute_auc({P{0.9,1}, P{0.8,0}, P{0.7,0}, P{0.6,0}}) - 1.0) < 1e-9);
//     // One positive at the bottom of three negatives → AUC = 0
//     assert(std::abs(linkpred::compute_auc({P{0.9,0}, P{0.8,0}, P{0.7,0}, P{0.6,1}}) - 0.0) < 1e-9);
//     // Positive in the middle of three negatives → 1/3
//     assert(std::abs(linkpred::compute_auc({P{0.9,0}, P{0.8,0}, P{0.7,1}, P{0.6,0}}) - 1.0/3.0) < 1e-9);
//     // All ties → AUC = 0.5
//     assert(std::abs(linkpred::compute_auc({P{0.5,1}, P{0.5,0}, P{0.5,1}, P{0.5,0}}) - 0.5) < 1e-9);
//     // Empty / single-class input → 0 by definition
//     assert(std::abs(linkpred::compute_auc({}) - 0.0) < 1e-9);
//     assert(std::abs(linkpred::compute_auc({P{0.9,1}, P{0.8,1}}) - 0.0) < 1e-9);
//     assert(std::abs(linkpred::compute_auc({P{0.9,0}, P{0.8,0}}) - 0.0) < 1e-9);
// }

// static void test_average_precision()
// {
//     using P = std::pair<double,int>;

//     // Perfect ranking → AP = 1
//     assert(std::abs(linkpred::compute_average_precision(
//                 {P{0.9,1}, P{0.8,1}, P{0.2,0}, P{0.1,0}}) - 1.0) < 1e-9);
//     // Positives at ranks 1 and 3 → (1/1 + 2/3) / 2 = 5/6
//     assert(std::abs(linkpred::compute_average_precision(
//                 {P{0.9,1}, P{0.8,0}, P{0.7,1}, P{0.6,0}}) - (5.0/6.0)) < 1e-9);
//     // Positives at ranks 2 and 4 → (1/2 + 2/4) / 2 = 0.5
//     assert(std::abs(linkpred::compute_average_precision(
//                 {P{0.9,0}, P{0.8,1}, P{0.7,0}, P{0.6,1}}) - 0.5) < 1e-9);
//     // No positives → 0
//     assert(std::abs(linkpred::compute_average_precision(
//                 {P{0.9,0}, P{0.8,0}}) - 0.0) < 1e-9);
//     // Empty input → 0
//     assert(std::abs(linkpred::compute_average_precision({}) - 0.0) < 1e-9);
// }

// Builds a score-sorted list from (score, label) rows; u/v are just the row index.
static std::vector<linkpred::ScoredPair>
pairs(std::initializer_list<std::pair<double,int>> rows)
{
    std::vector<linkpred::ScoredPair> out;
    node_id_t i = 0;
    for (const auto &[score, label] : rows) {
        out.push_back({i, i + 1, score, label});
        ++i;
    }
    return out;
}

static void test_precision_at_k()
{
    // Top-2 are positives → P@2 = 1.0
    assert(std::abs(linkpred::precision_at_k(
                pairs({{0.9,1}, {0.8,1}, {0.2,0}, {0.1,0}}), 2) - 1.0) < 1e-9);
    // Top-4: 2 positives out of 4 → P@4 = 0.5
    assert(std::abs(linkpred::precision_at_k(
                pairs({{0.9,1}, {0.8,1}, {0.2,0}, {0.1,0}}), 4) - 0.5) < 1e-9);
    // Top-1 is a negative → P@1 = 0.0
    assert(std::abs(linkpred::precision_at_k(
                pairs({{0.9,0}, {0.8,1}}), 1) - 0.0) < 1e-9);
    // k > n: uses all n; 1 positive out of 2 → 0.5
    assert(std::abs(linkpred::precision_at_k(
                pairs({{0.9,1}, {0.8,0}}), 10) - 0.5) < 1e-9);
    // k = 0 → 0
    assert(std::abs(linkpred::precision_at_k(
                pairs({{0.9,1}, {0.8,0}}), 0) - 0.0) < 1e-9);
    // Empty input → 0
    assert(std::abs(linkpred::precision_at_k({}, 10) - 0.0) < 1e-9);
}

static void test_recall_at_k()
{
    // 2 positives in top-4, 8 positives overall → R@4 = 0.25
    assert(std::abs(linkpred::recall_at_k(
                pairs({{0.9,1}, {0.8,1}, {0.2,0}, {0.1,0}}), 4, 8) - 0.25) < 1e-9);
    // Only the top-1 counts → 1 / 8
    assert(std::abs(linkpred::recall_at_k(
                pairs({{0.9,1}, {0.8,1}}), 1, 8) - 0.125) < 1e-9);
    // No positives overall → 0 (guard)
    assert(std::abs(linkpred::recall_at_k(
                pairs({{0.9,0}}), 1, 0) - 0.0) < 1e-9);
}

// ─── TopK tests ────────────────────────────────────────────────────────────

static void test_topk()
{
    // Keeps the 3 highest of 6 offers, drained in descending score order.
    linkpred::TopK top(3);
    double scores[] = {0.5, 0.1, 0.9, 0.3, 0.7, 0.2};
    for (node_id_t i = 0; i < 6; ++i)
        top.offer({i, i + 1, scores[i], 0});
    assert(top.offered() == 6);
    assert(top.size() == 3);
    auto out = top.drain_sorted();
    assert(out.size() == 3);
    assert(out[0].score == 0.9 && out[1].score == 0.7 && out[2].score == 0.5);
    assert(top.size() == 0);   // drain empties the heap

    // Fewer offers than k → keeps them all.
    linkpred::TopK small(10);
    small.offer({0, 1, 0.4, 1});
    small.offer({0, 2, 0.6, 0});
    auto s = small.drain_sorted();
    assert(s.size() == 2 && s[0].score == 0.6 && s[1].score == 0.4);
}

static void test_topk_ties()
{
    // Five pairs all scoring 1.0, offered in two different orders. k=2 must keep
    // the two smallest (u, v) both times, regardless of arrival order.
    std::vector<linkpred::ScoredPair> tied = {
        {3, 4, 1.0, 0}, {0, 5, 1.0, 0}, {2, 9, 1.0, 0}, {0, 2, 1.0, 0}, {1, 1, 1.0, 0}};

    linkpred::TopK fwd(2), rev(2);
    for (size_t i = 0; i < tied.size(); ++i) fwd.offer(tied[i]);
    for (size_t i = tied.size(); i-- > 0;)   rev.offer(tied[i]);
    auto a = fwd.drain_sorted();
    auto b = rev.drain_sorted();
    assert(a.size() == 2 && b.size() == 2);
    assert(a[0].u == 0 && a[0].v == 2 && a[1].u == 0 && a[1].v == 5);
    assert(b[0].u == a[0].u && b[0].v == a[0].v && b[1].u == a[1].u && b[1].v == a[1].v);
}

static void test_merge_topk()
{
    // Same 6 pairs split two different ways across "threads" → same merged top-3.
    std::vector<linkpred::ScoredPair> all = {
        {0, 1, 0.9, 1}, {0, 2, 0.5, 0}, {1, 2, 0.5, 1},
        {1, 3, 0.5, 0}, {2, 3, 0.1, 0}, {0, 3, 0.7, 1}};

    std::vector<linkpred::TopK> split1, split2;
    for (int t = 0; t < 2; ++t) { split1.emplace_back(3); split2.emplace_back(3); }
    for (size_t i = 0; i < all.size(); ++i) {
        split1[i % 2].offer(all[i]);           // interleaved
        split2[i < 3 ? 0 : 1].offer(all[i]);   // first half / second half
    }
    auto m1 = linkpred::merge_topk(split1, 3);
    auto m2 = linkpred::merge_topk(split2, 3);
    assert(m1.size() == 3 && m2.size() == 3);
    // 0.9, 0.7, then the smallest (u, v) among the three 0.5s → (0, 2)
    assert(m1[0].score == 0.9 && m1[1].score == 0.7);
    assert(m1[2].u == 0 && m1[2].v == 2);
    for (size_t i = 0; i < 3; ++i)
        assert(m1[i].u == m2[i].u && m1[i].v == m2[i].v);

    // k larger than everything offered → keeps all
    std::vector<linkpred::TopK> one;
    one.emplace_back(10);
    one[0].offer(all[0]);
    assert(linkpred::merge_topk(one, 10).size() == 1);
}

// ─── main ─────────────────────────────────────────────────────────────────

int main()
{
    test_common_neighbors();
    test_jaccard();
    test_dot();
    test_cosine();
    // test_auc();
    // test_average_precision();
    test_precision_at_k();
    test_recall_at_k();
    test_topk();
    test_topk_ties();
    test_merge_topk();

    std::printf("all linkpred tests passed\n");
    return 0;
}