// linkpred_scores.h — pure score functions for link prediction.
//
// No Flexograph dependency. Two families of scores:
//
//   Structural (operate on sorted adjacency lists of node IDs):
//     - common_neighbors    [implemented]
//     - jaccard             [implemented]
//     - score_pair          [implemented]  (picks cn / jaccard by --method)
//     - adamic_adar         [TODO: needs degree-lookup callback]
//     - preferential_attachment [TODO]
//     - resource_allocation [TODO: needs degree-lookup callback]
//
//   Embedding (operate on dense float arrays, e.g. node2vec):
//     - dot                 [implemented]  (vector<float>)
//     - cosine              [implemented]  (vector<float>)
//     - cosine_from_ptrs    [implemented]  (raw float*)
//     - dot_from_ptrs       [implemented]  (raw float*)
//     - cosine_from_bytes   [implemented]  (raw bytes, for prop_blob .data)
//     - dot_from_bytes      [implemented]  (raw bytes)
//     - neg_euclidean       [TODO]
//
// Both families return a scalar score; larger = more likely a link.
// Evaluation (AUC/AP/P@k) lives in linkpred_eval.h and is shared.

#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace linkpred {

// Pairs scored with an empty embedding on either side (no node_props row, or a
// zero-length vector): cosine / dot return 0 for them. Counted so a partial
// embedding DB is visible instead of silently scoring 0 (added 2026-11-22).
inline std::atomic<size_t> g_missing_embedding_pairs{0};

// - Structural scores -

// Intersection size of two sorted ascending lists.
template <typename T>
inline int64_t common_neighbors(const std::vector<T> &Nu,
                                const std::vector<T> &Nv)
{
    int64_t count = 0;
    size_t i = 0, j = 0;
    while (i < Nu.size() && j < Nv.size()) {
        if      (Nu[i] < Nv[j]) ++i;
        else if (Nu[i] > Nv[j]) ++j;
        else { ++count; ++i; ++j; }
    }
    return count;
}

// Jaccard similarity: |Nu ∩ Nv| / |Nu ∪ Nv|.
template <typename T>
inline double jaccard(const std::vector<T> &Nu,
                      const std::vector<T> &Nv)
{
    int64_t inter = common_neighbors(Nu, Nv);
    int64_t uni   = (int64_t)Nu.size() + (int64_t)Nv.size() - inter;
    return uni > 0 ? (double)inter / (double)uni : 0.0;
}

// Structural score picked by --method: "cn" → common_neighbors, else jaccard.
// (Moved from fg_linkpred_bench_db.cpp.)
template <typename T>
inline double score_pair(const std::string &method,
                         const std::vector<T> &Nu,
                         const std::vector<T> &Nv)
{
    if (method == "cn") return (double)common_neighbors(Nu, Nv);   // set intersection
    return jaccard(Nu, Nv);                                        // |∩|/|∪|
}

// Same three, on raw sorted arrays (used by --variant=csr, where each
// neighbour list is a slice of one flat array, not its own vector).
template <typename T>
inline int64_t common_neighbors(const T *Nu, size_t nu, const T *Nv, size_t nv)
{
    int64_t count = 0;
    size_t i = 0, j = 0;
    while (i < nu && j < nv) {
        if      (Nu[i] < Nv[j]) ++i;
        else if (Nu[i] > Nv[j]) ++j;
        else { ++count; ++i; ++j; }
    }
    return count;
}

template <typename T>
inline double score_pair(const std::string &method,
                         const T *Nu, size_t nu, const T *Nv, size_t nv)
{
    int64_t inter = common_neighbors(Nu, nu, Nv, nv);
    if (method == "cn") return (double)inter;
    int64_t uni = (int64_t)nu + (int64_t)nv - inter;
    return uni > 0 ? (double)inter / (double)uni : 0.0;
}

// - Embedding scores (vector<float>) -

// Dot product: Σ_i eu[i] * ev[i].
inline double dot(const std::vector<float> &eu, const std::vector<float> &ev)
{
    size_t n = std::min(eu.size(), ev.size());
    if (n == 0) g_missing_embedding_pairs.fetch_add(1, std::memory_order_relaxed);
    double s = 0.0;
    for (size_t i = 0; i < n; ++i)
        s += (double)eu[i] * (double)ev[i];
    return s;
}

// Cosine similarity: (eu · ev) / (||eu|| * ||ev||).
// Returns 0.0 if either vector is all zeros.
inline double cosine(const std::vector<float> &eu, const std::vector<float> &ev)
{
    size_t n = std::min(eu.size(), ev.size());
    if (n == 0) g_missing_embedding_pairs.fetch_add(1, std::memory_order_relaxed);
    double dot_uv = 0.0, norm_u = 0.0, norm_v = 0.0;
    for (size_t i = 0; i < n; ++i) {
        dot_uv += (double)eu[i] * (double)ev[i];
        norm_u += (double)eu[i] * (double)eu[i];
        norm_v += (double)ev[i] * (double)ev[i];
    }
    double denom = std::sqrt(norm_u) * std::sqrt(norm_v);
    return denom > 0.0 ? dot_uv / denom : 0.0;
}

// - Embedding scores on raw float pointers -

// Cosine similarity over two float arrays of the same length.
// Inputs are float[]; the accumulator is double to avoid rounding over dim terms.
inline double cosine_from_ptrs(const float *a, const float *b, size_t dim)
{
    double dot_product = 0.0, a_sq_sum = 0.0, b_sq_sum = 0.0;
    for (size_t i = 0; i < dim; ++i) {
        dot_product += (double)a[i] * b[i];
        a_sq_sum    += (double)a[i] * a[i];
        b_sq_sum    += (double)b[i] * b[i];
    }
    double magnitude_product = std::sqrt(a_sq_sum) * std::sqrt(b_sq_sum);
    return magnitude_product > 0 ? dot_product / magnitude_product : 0.0;
}

// Dot product over two float arrays of the same length.
inline double dot_from_ptrs(const float *a, const float *b, size_t dim)
{
    double dot_product = 0.0;
    for (size_t i = 0; i < dim; ++i)
        dot_product += (double)a[i] * b[i];
    return dot_product;
}

// - Embedding scores on raw bytes (no Flexograph dependency) -
//
// These are for callers that have a prop_blob's data/size but I want to keep
// this header free of Flexograph types due to it being normal standard logical operators.
// Pass pb.data, pb.size directly.

inline double cosine_from_bytes(const uint8_t *a, size_t a_bytes,
                                const uint8_t *b, size_t b_bytes)
{
    size_t dim = std::min(a_bytes, b_bytes) / sizeof(float);
    if (dim == 0) { g_missing_embedding_pairs.fetch_add(1, std::memory_order_relaxed); return 0.0; }
    return cosine_from_ptrs(reinterpret_cast<const float*>(a),
                            reinterpret_cast<const float*>(b), dim);
}

inline double dot_from_bytes(const uint8_t *a, size_t a_bytes,
                             const uint8_t *b, size_t b_bytes)
{
    size_t dim = std::min(a_bytes, b_bytes) / sizeof(float);
    if (dim == 0) { g_missing_embedding_pairs.fetch_add(1, std::memory_order_relaxed); return 0.0; }
    return dot_from_ptrs(reinterpret_cast<const float*>(a),
                         reinterpret_cast<const float*>(b), dim);
}





// COULD DO: (below is not wired up if they do exists)
// Neo4J has them written out nicely for reference: https://neo4j.com/docs/graph-data-science/current/algorithms/topological-link-prediction/preferential-attachment/
// TODO: adamic_adar, preferential_attachment, resource_allocation
//       (need a degree-lookup callback std::function<int64_t(T)>).
// TODO: neg_euclidean (needs the same float-vector signature as dot/cosine).



// Adamic-Adar: Σ_{w ∈ Nu ∩ Nv} 1 / log(deg(w)).
//
// `degree` is a callable mapping a neighbour ID to that neighbour's degree,
// e.g.  auto deg = [&](T w) { return (int64_t)adj[w].size(); };
//
// Terms where deg(w) < 2 are skipped: log(1) == 0 (division by zero) and
// log(0) is undefined, so a node with a single neighbour would otherwise
// make the whole score infinite. Skipping matches the usual convention in
// the link-prediction literature.
template <typename T, typename DegreeFn>
inline double adamic_adar(const std::vector<T> &Nu,
                          const std::vector<T> &Nv,
                          DegreeFn degree)
{
    double score = 0.0;
    size_t i = 0, j = 0;
    while (i < Nu.size() && j < Nv.size()) {
        if      (Nu[i] < Nv[j]) ++i;
        else if (Nu[i] > Nv[j]) ++j;
        else {
            int64_t deg = (int64_t)degree(Nu[i]);
            if (deg > 1) score += 1.0 / std::log((double)deg);
            ++i; ++j;
        }
    }
    return score;
}

// Same, on raw sorted arrays (for --variant=csr, where neighbour lists are
// slices of one flat array).
template <typename T, typename DegreeFn>
inline double adamic_adar(const T *Nu, size_t nu,
                          const T *Nv, size_t nv,
                          DegreeFn degree)
{
    double score = 0.0;
    size_t i = 0, j = 0;
    while (i < nu && j < nv) {
        if      (Nu[i] < Nv[j]) ++i;
        else if (Nu[i] > Nv[j]) ++j;
        else {
            int64_t deg = (int64_t)degree(Nu[i]);
            if (deg > 1) score += 1.0 / std::log((double)deg);
            ++i; ++j;
        }
    }
    return score;
}
} // namespace linkpred