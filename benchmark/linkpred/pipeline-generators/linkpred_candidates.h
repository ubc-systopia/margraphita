// linkpred_candidates.h — candidate generation on the Flexograph DB, with cursors.
//
// Flexograph versions of the generic examples in candidate_generation.cpp
// (same strategies, same structure, same comments on node picking).
// Standalone: nothing in fg_linkpred_bench_db calls these yet.
//
// Strategies here:
//   exhaustive_all_pairs           every pair u < v
//   uniform_random_sampling        k random draws
//   distance_bounded_candidates    within k hops (k = 2 or 3)
//   friends_of_friends             2 hops, open triangles
// Placeholders (comments only, see bottom): ego-net expansion, meta-path.
//
// Two flavors per strategy, like candidate_generation.cpp:
//   X_each(g, ..., emit)  LAZY.  Calls emit(std::pair{u, v}) per candidate;
//                                return false from emit to stop early.
//                                Never stores the whole list.
//   X(g, ...)             EAGER. Collects everything into a CandidateList.
//                                Only for small graphs (s16 ≈ 2e9 pairs).
//
// skip_edges (default true): drop pairs (u, v) where v is already an out-
// neighbour of u, as in candidate_generation.cpp. Pass false to keep them —
// the current benchmark needs them, because existing edges are its labels.
//
// Cursor rule (important): walk nodes with get_node_iter(), which opens a FRESH
// cursor, and look up neighbours with get_out_nodes_id(). Do NOT walk with
// get_outnbd_iter() while also calling get_out_nodes_id() on the same handle:
// both use the handle's shared out_adjlist_cursor, so the lookup moves the walk.
//
// Edges are followed in the out direction only (get_out_nodes_id), matching
// the benchmark's labels and candidate_generation.cpp's g[u].

#pragma once

#include <algorithm>
#include <queue>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common_defs.h"
#include "graph.h"

namespace linkpred {

using CandidatePair = std::pair<node_id_t, node_id_t>;
using CandidateList = std::vector<CandidatePair>;

// ===============================================================
// PER-U generators — for threaded runners
// ===============================================================
// The whole-graph X_each functions below own the loop over u, so threads
// can't split them. A per-u generator only answers "which v's for this u?";
// the runner owns the u loop and hands each thread its own block of u's.
//
// Shape:  gen.for_each(u, emit)   emit(node_id_t v) -> bool (false = stop this u)
// Threading: one generator object per thread, built on THAT thread's handle
// (its cursor must not be shared). Each u belongs to one thread and only
// v > u is emitted, so no pair is produced twice.

// Exhaustive, per u: every node v > u. Same cursor steps as the inner loop of
// structural_worker_cursor in linkpred_exhaustive.h. Existing edges are NOT
// skipped (the runner labels them); filter in emit if you want them out.
struct ExhaustiveCandidates {
    NodeCursor *b;                                    // inner cursor: v > u

    explicit ExhaustiveCandidates(GraphBase *g) : b(g->get_node_iter()) {}
    ~ExhaustiveCandidates() { b->close(); delete b; }
    ExhaustiveCandidates(const ExhaustiveCandidates &) = delete;
    ExhaustiveCandidates &operator=(const ExhaustiveCandidates &) = delete;

    template <typename F>
    void for_each(node_id_t u, F emit)
    {
        b->reset();
        b->set_key_range({u + 1, OutOfBand_ID_MAX});  // v > u, each pair once
        node v;
        b->next(&v);
        while (v.id != OutOfBand_ID_MAX) {
            if (!emit(v.id)) return;
            b->next(&v);
        }
    }
};

// Friends-of-friends, per u: v reached by u → w → v, v != u, v > u.
// Same logic as friends_of_friends_each below, for one u. Ascending v order.
// skip_edges (default false here, so the runner sees true edges to label):
// when true, drop v that are already direct neighbours of u.
struct FofCandidates {
    GraphBase *g;                                     // this thread's handle
    bool skip_edges = false;

    explicit FofCandidates(GraphBase *g_, bool skip = false) : g(g_), skip_edges(skip) {}

    template <typename F>
    void for_each(node_id_t u, F emit)
    {
        std::vector<node_id_t> Nu = g->get_out_nodes_id(u);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        std::unordered_set<node_id_t> fof;
        for (node_id_t w : Nu)
            for (node_id_t v : g->get_out_nodes_id(w)) {
                if (v == u)                         continue;
                if (skip_edges && Nu_set.count(v))  continue;
                if (v > u) fof.insert(v);
            }

        std::vector<node_id_t> vs(fof.begin(), fof.end());
        std::sort(vs.begin(), vs.end());
        for (node_id_t v : vs)
            if (!emit(v)) return;
    }
};

// Within k hops, per u: BFS from u to depth k, emit v > u with
// min_dist <= dist(v) <= k. Same logic as distance_bounded_candidates_each
// below, for one u. Ascending v order.
// skip_edges (default false): when true, drop dist-1 nodes (direct neighbours).
struct KHopCandidates {
    GraphBase *g;                                     // this thread's handle
    int k = 2;
    bool skip_edges = false;

    KHopCandidates(GraphBase *g_, int k_, bool skip = false) : g(g_), k(k_), skip_edges(skip) {}

    template <typename F>
    void for_each(node_id_t u, F emit)
    {
        int min_dist = skip_edges ? 2 : 1;
        std::unordered_map<node_id_t, int> dist;
        std::queue<node_id_t> q;
        dist[u] = 0;
        q.push(u);
        while (!q.empty()) {
            node_id_t x = q.front(); q.pop();
            int dx = dist[x];
            if (dx == k) continue;                    // don't expand beyond k
            for (node_id_t y : g->get_out_nodes_id(x))
                if (dist.find(y) == dist.end()) {
                    dist[y] = dx + 1;
                    q.push(y);
                }
        }

        std::vector<node_id_t> vs;
        for (const auto &[v, d] : dist)
            if (v > u && d >= min_dist) vs.push_back(v);
        std::sort(vs.begin(), vs.end());
        for (node_id_t v : vs)
            if (!emit(v)) return;
    }
};

// Uniform reservoir sample over actual node IDs, O(count) memory.
inline std::vector<node_id_t> sample_source_vertices(GraphBase *g, size_t count, uint64_t seed)
{
    std::vector<node_id_t> ids;
    std::mt19937_64 rng(seed);
    auto *cursor = g->get_node_iter();
    cursor->set_key_range({0, OutOfBand_ID_MAX});
    node n; cursor->next(&n);
    uint64_t seen = 0;
    while (n.id != OutOfBand_ID_MAX) {
        ++seen;
        if (ids.size() < count) ids.push_back(n.id);
        else {
            auto j = std::uniform_int_distribution<uint64_t>(0, seen - 1)(rng);
            if (j < count) ids[j] = n.id;
        }
        cursor->next(&n);
    }
    cursor->close(); delete cursor;
    std::sort(ids.begin(), ids.end());
    return ids;
}

// Reverse-duplicate filter for sampled sources (undirected graphs only).
// With sampled sources a source u emits all its candidates v, including v < u,
// so it explores its whole neighbourhood. If v is also a selected source, the
// same unordered pair is produced again from v as (v, u). Keep it once: skip
// (u, v) when v is a selected source and v < u (it was or will be emitted as (v, u)).
// Valid only for symmetric generators on an undirected graph (out adjacency
// is then symmetric): exhaustive (every pair), friends-of-friends (u→w→v iff
// v→w→u) and k-hop (dist(u,v) = dist(v,u)). For directed graphs, (u, v) and
// (v, u) are different targets: pass nullptr (no filter), which is also what
// the all-source path uses (it emits v > u only, so has no reverse duplicates).
struct ReverseFilter {
    std::unordered_set<node_id_t> selected;   // selected source IDs
    size_t skipped = 0;                       // reverse duplicates dropped
    bool skip(node_id_t u, node_id_t v) {
        if (v < u && selected.count(v)) { ++skipped; return true; }
        return false;
    }
};
inline ReverseFilter make_reverse_filter(const std::vector<node_id_t>& sources)
{
    ReverseFilter f; f.selected.insert(sources.begin(), sources.end()); return f;
}

// ===============================================================
// WHOLE-GRAPH generators — readable, single-threaded
// ===============================================================

// ---------------------------------------------------------------
// Exhaustive - all pairs
// ---------------------------------------------------------------
// Node picking:
//   Take every node u in order (node cursor). Pair it only with the nodes
//   that come after it (second node cursor over [u + 1, max]) so each
//   unordered pair is looked at once. If skip_edges and the pair is
//   already an edge, skip it; otherwise it becomes a candidate.
// Cost is O(n²); same walk as the exhaustive benchmark (linkpred_exhaustive.h).
template <typename F>
void exhaustive_all_pairs_each(GraphBase *g, F emit, bool skip_edges = true,
                                      const std::vector<node_id_t>* sources = nullptr,
                                      ReverseFilter* reverse = nullptr)
{
    NodeCursor *a = sources ? nullptr : g->get_node_iter();             // outer: u
    NodeCursor *b = g->get_node_iter();             // inner: v > u
    if (a) a->set_key_range({0, OutOfBand_ID_MAX});
    bool stop = false;
    node u;
    size_t source_index = 0;
    auto next_source = [&]() {
        if (a) a->next(&u);
        else u.id = source_index < sources->size() ? (*sources)[source_index++] : OutOfBand_ID_MAX;
    };
    next_source();

    while (!stop && u.id != OutOfBand_ID_MAX) {
        std::unordered_set<node_id_t> Nu;
        if (skip_edges)
            for (node_id_t n : g->get_out_nodes_id(u.id)) Nu.insert(n);

        b->reset();
        b->set_key_range({sources ? node_id_t(0) : u.id + 1, OutOfBand_ID_MAX});
        node v;
        b->next(&v);
        while (v.id != OutOfBand_ID_MAX) {
            if (v.id != u.id && !(skip_edges && Nu.count(v.id))
                && !(reverse && reverse->skip(u.id, v.id)) && !emit(CandidatePair{u.id, v.id})) {
                stop = true;
                break;
            }
            b->next(&v);
        }
        next_source();
    }

    if (a) a->close();
    b->close();
    delete a;
    delete b;
}

inline CandidateList exhaustive_all_pairs(GraphBase *g, bool skip_edges = true)
{
    CandidateList out;
    exhaustive_all_pairs_each(g, [&](CandidatePair c) {
        out.push_back(c); return true;
    }, skip_edges);
    return out;
}

// ---------------------------------------------------------------
// Uniform random sampling
// ---------------------------------------------------------------
// Node picking:
//   Repeatedly draw two node IDs u and v independently, uniformly at
//   random over [min_node_id, max_node_id]. Throw the draw away if u == v,
//   if either ID is not a node (IDs need not be contiguous), or if
//   skip_edges and the pair is already an edge. Otherwise emit it.
//   Stop after k draws (thrown-away draws count, as in the generic version).
// Point lookups only (has_node / has_edge), no cursor walk.
// Mostly a source of negatives / eval candidates, not a real generator.
template <typename F>
void uniform_random_sampling_each(GraphBase *g, int k, unsigned seed, F emit,
                                  bool skip_edges = true)
{
    node_id_t lo = g->get_min_node_id();
    node_id_t hi = g->get_max_node_id();
    if (hi == OutOfBand_ID_MAX || lo > hi) return;   // metadata missing / empty

    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<node_id_t> pick(lo, hi);
    for (int i = 0; i < k; ++i) {
        node_id_t u = pick(rng);
        node_id_t v = pick(rng);
        if (u == v)                                 continue;
        if (!g->has_node(u) || !g->has_node(v))     continue;
        if (skip_edges && g->has_edge(u, v))        continue;
        if (!emit(CandidatePair{u, v}))             return;
    }
}

inline CandidateList uniform_random_sampling(GraphBase *g, int k, unsigned seed,
                                             bool skip_edges = true)
{
    CandidateList out;
    uniform_random_sampling_each(g, k, seed, [&](CandidatePair c) {
        out.push_back(c); return true;
    }, skip_edges);
    return out;
}

// ---------------------------------------------------------------
// Distance-bounded candidates (k-hop)
// ---------------------------------------------------------------
// Node picking:
//   Pick each node u as a source (node cursor) and BFS outward from it up
//   to k hops (usually 2 or 3), expanding with get_out_nodes_id. Every node
//   v reached within k hops that is not u becomes a candidate, keeping only
//   u < v to avoid emitting each pair twice.
//   skip_edges: drop dist-1 nodes (direct neighbours), so k=2 gives exactly
//   the 2-hop pairs and k=3 adds the 3-hop pairs, as in the generic version.
//   Without skip_edges, direct neighbours (dist 1) are kept too.
//   Emitted per u in ascending v order.
// dist is a hash map of visited nodes (not a size-n array per u like the
// generic version), so memory follows the k-hop neighbourhood, not n.
template <typename F>
void distance_bounded_candidates_each(GraphBase *g, int k, F emit,
                                      bool skip_edges = true,
                                      const std::vector<node_id_t>* sources = nullptr,
                                      ReverseFilter* reverse = nullptr)
{
    int min_dist = skip_edges ? 2 : 1;
    NodeCursor *a = sources ? nullptr : g->get_node_iter();
    if (a) a->set_key_range({0, OutOfBand_ID_MAX});
    bool stop = false;
    node u;
    size_t source_index = 0;
    auto next_source = [&]() {
        if (a) a->next(&u);
        else u.id = source_index < sources->size() ? (*sources)[source_index++] : OutOfBand_ID_MAX;
    };
    next_source();

    while (!stop && u.id != OutOfBand_ID_MAX) {
        // BFS from u up to depth k
        std::unordered_map<node_id_t, int> dist;
        std::queue<node_id_t> q;
        dist[u.id] = 0;
        q.push(u.id);
        while (!q.empty()) {
            node_id_t x = q.front(); q.pop();
            int dx = dist[x];
            if (dx == k) continue;                  // don't expand beyond k
            for (node_id_t y : g->get_out_nodes_id(x)) {
                if (dist.find(y) == dist.end()) {
                    dist[y] = dx + 1;
                    q.push(y);
                }
            }
        }

        // emit pairs (u, v) with u < v and min_dist <= dist[v] <= k
        std::vector<node_id_t> vs;
        for (const auto &[v, d] : dist)
            if (v != u.id && (sources || v > u.id) && d >= min_dist) vs.push_back(v);
        std::sort(vs.begin(), vs.end());
        for (node_id_t v : vs) {
            if (reverse && reverse->skip(u.id, v)) continue;
            if (!emit(CandidatePair{u.id, v})) { stop = true; break; }
        }

        next_source();
    }

    if (a) a->close();
    delete a;
}

inline CandidateList distance_bounded_candidates(GraphBase *g, int k = 2,
                                                 bool skip_edges = true)
{
    CandidateList out;
    distance_bounded_candidates_each(g, k, [&](CandidatePair c) {
        out.push_back(c); return true;
    }, skip_edges);
    return out;
}

// ---------------------------------------------------------------
// Friends-of-friends / triadic closure
// ---------------------------------------------------------------
// Node picking:
//   For each node u (node cursor), look at its direct neighbours N(u) (one
//   hop), then at each neighbour's neighbours (two hops) to reach the
//   friends-of-friends set. For each v in that set: skip v if it's u, skip
//   it if skip_edges and it's already a direct neighbour of u (the triangle
//   is closed), and only emit when u < v to dedup. What's left are open
//   triangles, i.e. "People You May Know". Emitted per u in ascending v order.
// With skip_edges this is the same set as distance_bounded_candidates(k=2);
// it is written as a direct two-hop walk, which is the shape a fused CN
// version would use (each path u → w → v is one common neighbour).
template <typename F>
void friends_of_friends_each(GraphBase *g, F emit, bool skip_edges = true,
                                      const std::vector<node_id_t>* sources = nullptr,
                                      ReverseFilter* reverse = nullptr)
{
    NodeCursor *a = sources ? nullptr : g->get_node_iter();
    if (a) a->set_key_range({0, OutOfBand_ID_MAX});
    bool stop = false;
    node u;
    size_t source_index = 0;
    auto next_source = [&]() {
        if (a) a->next(&u);
        else u.id = source_index < sources->size() ? (*sources)[source_index++] : OutOfBand_ID_MAX;
    };
    next_source();

    while (!stop && u.id != OutOfBand_ID_MAX) {
        std::vector<node_id_t> Nu = g->get_out_nodes_id(u.id);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        std::unordered_set<node_id_t> fof;
        for (node_id_t w : Nu)
            for (node_id_t v : g->get_out_nodes_id(w)) {
                if (v == u.id)                      continue;
                if (skip_edges && Nu_set.count(v))  continue;
                if (sources || v > u.id) fof.insert(v);
            }

        std::vector<node_id_t> vs(fof.begin(), fof.end());
        std::sort(vs.begin(), vs.end());
        for (node_id_t v : vs) {
            if (reverse && reverse->skip(u.id, v)) continue;
            if (!emit(CandidatePair{u.id, v})) { stop = true; break; }
        }

        next_source();
    }

    if (a) a->close();
    delete a;
}

inline CandidateList friends_of_friends(GraphBase *g, bool skip_edges = true)
{
    CandidateList out;
    friends_of_friends_each(g, [&](CandidatePair c) {
        out.push_back(c); return true;
    }, skip_edges);
    return out;
}

// ---------------------------------------------------------------
// Ego-net / circle-of-trust expansion (PLACEHOLDER)
// ---------------------------------------------------------------
// TODO: for each u, pick its top-T neighbours, then take their neighbours.
//   Needs a ranking of u's neighbours. The kron graphs have no edge weights,
//   so options are: rank by neighbour degree (get_out_degree), or by a score
//   such as CN with u. See ego_net_expansion in candidate_generation.cpp.

// ---------------------------------------------------------------
// Meta-path constrained neighbourhoods (PLACEHOLDER)
// ---------------------------------------------------------------
// TODO: walk only along a typed path (e.g. A -> P -> A). Node types come from
//   the ID itself: VTYPE_OF(id) (common_defs.h, B64 builds). The kron graphs
//   are untyped, so this needs a typed dataset (e.g. LDBC).
//   See meta_path_candidates in candidate_generation.cpp.

} // namespace linkpred
