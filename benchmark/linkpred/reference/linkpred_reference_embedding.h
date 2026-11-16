// linkpred_reference_embedding.h -- plain single-threaded reference, embedding family (cosine / dot).
// Split from linkpred_reference.h (2026-11-16); function bodies unchanged.
//
// Purpose
// -------
// Plain single-threaded reference of every variation. Not the benchmarked
// path: fg_linkpred_bench_db runs the threaded functions by default,
// including at --threads=1 (that is the fast code with a single thread,
// not this file).
//
// Two uses:
//   1. Reading what a variation does without threading around it.
//   2. Cross-checking the threaded code: run the same command with
//      --impl=reference; pairs_scored, true_edges and top_k should match
//      the threaded path exactly (ties broken by (u, v) in both). A
//      mismatch means the threaded path has a bug.
//
// Timings from --impl=reference runs are not comparable to benchmark runs;
// this file is single-threaded and unoptimised by design.
//
// Naming convention
// -----------------
//   run_reference_<cand>_<family>_<variant>[_<sub_algorithm>]
//
//   impl        reference  (single-threaded; --threads is forced to 1, with
//                           a note on stderr)
//   cand        exhaustive | fof | khop | uniformRandom
//   family      structural | embedding
//   variant     exactly the --variant= flag value
//   sub_alg     _bfs on khop/…/cursor (anticipated to grow a nested sibling)
//
// Functions defined here, grouped by candidate
// --------------------------------------------
//   exhaustive structural  cursor, preloaded_ids, csr, tiled
//   exhaustive embedding   cursor, cursor_cached, preloaded_db, pvector, tiled
//   fof        structural  cursor, wedge
//   fof        embedding   cursor
//   khop       structural  cursor_bfs, outcursor_bfs, nested, outcursor_nested,
//                          wedge
//   khop       embedding   cursor_bfs, nested, outcursor_nested, nested_cached
//   uniformRandom          structural cursor, embedding cursor
//
// Experiments (defined, not wired)
// --------------------------------
// Two exhaustive structural functions are defined but not in the dispatch or
// the matrix:
//   run_reference_exhaustive_structural_no_reset
//   run_reference_exhaustive_structural_fresh_cursor
// Their docblocks record what they measured; both are commented out in
// fg_linkpred_bench_reference.cpp's dispatch. Kept compiled so the compiler
// type-checks them.
//
// How to run
// ----------
// Add --impl=reference to any normal command line, e.g.
//   fg_linkpred_bench_db -p <db_dir> -m <db_name> -g adj -r \
//       --method=cn --candidates=fof --variant=wedge --impl=reference
// --threads is ignored (forced to 1).
// Output is marked so it can't be mistaken for a benchmark run:
//   stdout line:  "LinkPredDB ... reference=1 ..."
//   CSV variation column:  <variant>_ref
//
// Variant semantics
// -----------------
// exhaustive — two axes (v enumeration × adjacency source):
//
//   variant                    | u source   | v source   | adjacency      | memory   | DB/pair
//   ---------------------------|------------|------------|----------------|----------|--------
//   _structural_cursor         | cursor a   | cursor b   | DB per pair    | O(deg)   | 1
//   _structural_preloaded_ids  | nodes[i]   | nodes[j]   | DB per pair    | O(n)     | 1
//   _structural_csr            | nodes[i]   | nodes[j]   | memory adj[]   | O(n+m)   | 0
//   _structural_tiled          | cursor a   | cursor b   | block per tile | O(tile)  | n/tile
//
//   _cursor and _preloaded_ids differ only in v enumeration (cursor walk vs
//   array index). Both still call get_out_nodes_id(v) per pair for scoring,
//   so their runtimes differ by <2%. The cursor advance is a tiny fraction
//   of the per-pair cost.
//
//   _csr is the ML-standard approach: preload the whole graph into memory,
//   then score with no DB calls. Expect 4-6x faster than _cursor on s12_e8,
//   at the cost of O(n+m) memory. Same tradeoff as pvector vs cursor in the
//   embedding family.
//
// embedding — two axes (enumeration × embedding access):
//
//   pair enumeration  |  embedding access           |  function suffix
//   ------------------|-----------------------------|-------------------
//   cursor walk       |  get_node_properties(v)     |  _embedding_cursor
//   cursor walk       |  lazy hash cache            |  _embedding_cursor_cached
//   preloaded walk    |  get_node_properties(v)     |  _embedding_preloaded_db
//   preloaded walk    |  all_emb[v] (preloaded)     |  _embedding_pvector
//   two cursors       |  blob read once per --tile  |  _embedding_tiled
//
// fof / khop — candidate set from walk or BFS, v > u:
//   structural (fof)             get_out_nodes_id per walk step and per candidate
//   structural_wedge (fof)       CN counted along u -> w -> v, no N(v) read
//   embedding (fof)              same candidates, blob per candidate
//   structural_bfs (khop)        BFS with get_out_nodes_id per frontier node and per candidate
//   structural_outcursor_bfs     same BFS, OutCursor for every adjacency read
//   structural_nested            k = 2 as nested loops over N(u), N(w); no BFS
//   structural_outcursor_nested  k = 2 nested loops, OutCursor for every adjacency read
//   structural_wedge (khop)      k = 2 nested loops, CN counted along u -> w -> v
//                                (no N(v) read; Jaccard uses get_out_degree(v))
//   (_outcursor names an access path, not a held structure;
//    khop grid = {bfs, nested} × {get_out_nodes_id, outcursor})
//   embedding_bfs (khop)         same BFS, blob per candidate
//   embedding_nested (khop)      k = 2 nested loops, blob per candidate
//   embedding_outcursor_nested   k = 2 nested loops, OutCursor for N(u), N(w);
//                                blob per candidate
//   embedding_nested_cached      k = 2 nested loops, each blob read at most once
//                                (lazy hash cache across all u)
//
// uniformRandom:
//   candidate per u = K draws uniformly from (u, max_node_id], filter duplicates
//   and non-node IDs, do NOT reject existing edges (they are the labels).
//   --samples=K --seed=S. Same shape as PyG negative_sampling / OGB random
//   negatives, but drawn per-source (fixed-K) instead of as a pair batch.
//
// Adding a new reference
// ----------------------
// Add a function following the naming pattern above. Update
// run_selected() in fg_linkpred_bench_db.cpp to route to it. Add a matrix
// row in linkpred_options.h. Do NOT reuse a threaded worker -- the whole
// point is that the reference is an independent implementation.

#pragma once

#include <algorithm>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common_defs.h"
#include "graph.h"
#include "pvector.h"

#include "linkpred_scores.h"
#include "linkpred_options.h"
#include "linkpred_topk.h"
#include "linkpred_types.h"

#include "linkpred_reference_structural.h"   // uniformRandom_candidates (shared candidate draw)

// ================================================================
// Helper: EmbeddingBlob + fetch_embedding
// ================================================================
// Used by the embedding variants. Takes a prop_blob from the DB and
// copies it into a std::vector<float>, so the cached copy lives in a
// normal C++ container (not a raw pointer that needs manual delete).
// The data is then usable with linkpred::cosine(vector, vector) directly.
//
// `valid` distinguishes "haven't fetched yet" from "fetched and got 0 floats".
struct EmbeddingBlob {
    std::vector<float> data;
    bool valid = false;
};

inline EmbeddingBlob fetch_embedding(GraphBase *g, node_id_t id)
{
    EmbeddingBlob out;
    prop_blob pb = g->get_node_properties(id);
    if (pb.size > 0 && pb.data != nullptr) {
        size_t nfloats = pb.size / sizeof(float);
        const float *f = reinterpret_cast<const float *>(pb.data);
        out.data.assign(f, f + nfloats);
        out.valid = true;
    }
    if (pb.data) delete[] const_cast<uint8_t*>(pb.data);
    return out;
}


// ================================================================
// Embedding exhaustive: variants differing in enumeration and embedding access
// ================================================================

// ---------------------------------------------------------------- exhaustive, embedding cursor (cosine / dot)
//
// run_reference_exhaustive_embedding_cursor
//   Selected by: --impl=reference --method=cosine|dot --variant=cursor
//   In memory:  Nu_set, pu (u's blob), pv (v's blob)
//   Per pair:   get_node_properties(v) --> pv          : DB read
//   u source:   a->next(&u)                            : cursor walk
//   v source:   b->next(&v)                            : cursor walk
//   scoring:    cosine_from_bytes(pu, pv)
inline RunResult run_reference_exhaustive_embedding_cursor(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();
    NodeCursor *b = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        std::unordered_set<node_id_t> Nu_set;
        for (node_id_t n : g->get_out_nodes_id(u.id)) Nu_set.insert(n);
        prop_blob pu = g->get_node_properties(u.id);       // u's embedding, once per row

        b->reset();
        b->set_key_range({u.id + 1, OutOfBand_ID_MAX});
        node v;
        b->next(&v);

        while (v.id != OutOfBand_ID_MAX) {
            prop_blob pv = g->get_node_properties(v.id);   // v's embedding, per pair

            int label = Nu_set.count(v.id) ? 1 : 0;
            double s = (o.method == "cosine")
                       ? linkpred::cosine_from_bytes(pu.data, pu.size, pv.data, pv.size)
                       : linkpred::dot_from_bytes  (pu.data, pu.size, pv.data, pv.size);
            top.offer({u.id, v.id, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;

            delete[] const_cast<uint8_t*>(pv.data);
            b->next(&v);
        }
        delete[] const_cast<uint8_t*>(pu.data);
        a->next(&u);
    }

    a->close();
    delete a;
    b->close();
    delete b;
    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- exhaustive, embedding cursor_cached (cosine / dot)
//
// run_reference_exhaustive_embedding_cursor_cached
//   Selected by: --impl=reference --method=cosine|dot --variant=cursor_cached
//   In memory:  cache (unordered_map<id, EmbeddingBlob>), Nu_set
//   Per pair:   cache[v] (on miss: get_node_properties(v)) : memory (DB once)
//   u source:   a->next(&u)                            : cursor walk
//   v source:   b->next(&v)                            : cursor walk
//   scoring:    cosine(bu, bv)
//
// Preloaded: nothing upfront. Cache fills lazily, one blob per node
// touched. First access to v pays the DB read; later accesses are memory.
// No threaded counterpart exists; test-only.
inline RunResult run_reference_exhaustive_embedding_cursor_cached(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    std::unordered_map<node_id_t, EmbeddingBlob> cache;    // persists across u

    NodeCursor *a = g->get_node_iter();
    NodeCursor *b = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        std::unordered_set<node_id_t> Nu_set;
        for (node_id_t n : g->get_out_nodes_id(u.id)) Nu_set.insert(n);

        // u's embedding via cache.
        auto &bu = cache[u.id];
        if (!bu.valid) bu = fetch_embedding(g, u.id);

        b->reset();
        b->set_key_range({u.id + 1, OutOfBand_ID_MAX});
        node v;
        b->next(&v);

        while (v.id != OutOfBand_ID_MAX) {
            // v's embedding via cache. First pair touching v pays one DB
            // read; all later pairs are a hash lookup.
            auto &bv = cache[v.id];
            if (!bv.valid) bv = fetch_embedding(g, v.id);

            int label = Nu_set.count(v.id) ? 1 : 0;
            double s = (o.method == "cosine")
                       ? linkpred::cosine(bu.data, bv.data)
                       : linkpred::dot(bu.data, bv.data);
            top.offer({u.id, v.id, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;

            b->next(&v);
        }
        a->next(&u);
    }

    a->close();
    delete a;
    b->close();
    delete b;
    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- exhaustive, embedding preloaded_db (cosine / dot)
//
// run_reference_exhaustive_embedding_preloaded_db
//   Selected by: --impl=reference --method=cosine|dot --variant=preloaded_db
//   In memory:  nodes[], Nu_set, pu, pv
//   Per pair:   get_node_properties(v) --> pv          : DB read
//   u source:   nodes[i]                               : array index
//   v source:   nodes[j]                               : array index
//   scoring:    cosine_from_bytes(pu, pv)
//
// Preloaded: node ID list ONLY. Embeddings still come from DB per pair.
// Isolates enumeration cost from embedding access cost.
// No threaded counterpart exists; test-only.
inline RunResult run_reference_exhaustive_embedding_preloaded_db(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    // Build the node list once.
    std::vector<node_id_t> nodes;
    {
        NodeCursor *nc = g->get_node_iter();
        nc->set_key_range({0, OutOfBand_ID_MAX});
        node n;
        nc->next(&n);
        while (n.id != OutOfBand_ID_MAX) {
            nodes.push_back(n.id);
            nc->next(&n);
        }
        nc->close();
        delete nc;
    }

    for (size_t i = 0; i < nodes.size(); ++i) {
        node_id_t u = nodes[i];
        std::unordered_set<node_id_t> Nu_set;
        for (node_id_t n : g->get_out_nodes_id(u)) Nu_set.insert(n);
        prop_blob pu = g->get_node_properties(u);

        for (size_t j = i + 1; j < nodes.size(); ++j) {
            node_id_t v = nodes[j];
            prop_blob pv = g->get_node_properties(v);

            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine")
                       ? linkpred::cosine_from_bytes(pu.data, pu.size, pv.data, pv.size)
                       : linkpred::dot_from_bytes  (pu.data, pu.size, pv.data, pv.size);
            top.offer({u, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;

            delete[] const_cast<uint8_t*>(pv.data);
        }
        delete[] const_cast<uint8_t*>(pu.data);
    }

    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- exhaustive, embedding pvector (cosine / dot)
//
// run_reference_exhaustive_embedding_pvector
//   Selected by: --impl=reference --method=cosine|dot --variant=pvector
//   In memory:  nodes[], all_emb[] (all embeddings)
//   Per pair:   all_emb[u], all_emb[v]                 : memory (no DB)
//   u source:   nodes[i]                               : array index
//   v source:   nodes[j]                               : array index
//   scoring:    cosine(eu, ev)
//
// Preloaded: node IDs AND full embeddings. Preload happens in main()
// before the benchmark starts, so time_ms excludes the preload cost
// (which is in load_ms).
//
// nodes / all_emb come from the pvector preload phase in
// fg_linkpred_bench_db.cpp. If they are empty, this returns an empty result.
inline RunResult run_reference_exhaustive_embedding_pvector(
    GraphBase *g,
    const std::vector<node_id_t> &nodes,
    const pvector<std::vector<float>> &all_emb,
    const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    for (size_t i = 0; i < nodes.size(); ++i) {
        node_id_t u = nodes[i];

        std::unordered_set<node_id_t> Nu_set;
        for (node_id_t n : g->get_out_nodes_id(u)) Nu_set.insert(n);

        const std::vector<float> &eu = all_emb[u];                     // ref, no copy
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            node_id_t v = nodes[j];
            const std::vector<float> &ev = all_emb[v];
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine")
                       ? linkpred::cosine(eu, ev)
                       : linkpred::dot(eu, ev);
            top.offer({u, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
    }
    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- exhaustive, embedding tiled (cosine / dot)
//
// run_reference_exhaustive_embedding_tiled
//   Selected by: --impl=reference --method=cosine|dot --variant=tiled
//                (reads --tile=N)
//   In memory:  one block of up to --tile u embeddings + N(u) for labels
//   Per pair:   nothing from the DB; v's blob is read once per (block, v)
//   u source:   cursor a, taken --tile at a time         : cursor walk
//   v source:   cursor b, reset to just after the block's first u : cursor walk
//
// Single-threaded counterpart of --impl=direct --variant=tiled for embeddings,
// written independently. No node-ID preload: only the current block is held.
// Each blob is read ~n / tile times instead of once per pair
// (_embedding_cursor, _embedding_preloaded_db) or once (_embedding_pvector).
inline RunResult run_reference_exhaustive_embedding_tiled(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);
    const size_t T = (size_t)o.tile;

    NodeCursor *a = g->get_node_iter();             // outer: blocks of u
    NodeCursor *b = g->get_node_iter();             // inner: v after the block's first u
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    std::vector<node_id_t> us;
    std::vector<EmbeddingBlob> eu;
    std::vector<std::unordered_set<node_id_t>> Nu;
    while (u.id != OutOfBand_ID_MAX) {
        // The next block of u's: embeddings and neighbour sets (labels), read once.
        us.clear();
        eu.clear();
        Nu.clear();
        while (u.id != OutOfBand_ID_MAX && us.size() < T) {
            us.push_back(u.id);
            eu.push_back(fetch_embedding(g, u.id));
            Nu.emplace_back();
            for (node_id_t x : g->get_out_nodes_id(u.id)) Nu.back().insert(x);
            a->next(&u);
        }

        // Every v after the block's first u, read once for the whole block.
        b->reset();
        b->set_key_range({us.front() + 1, OutOfBand_ID_MAX});
        node v;
        b->next(&v);
        while (v.id != OutOfBand_ID_MAX) {
            EmbeddingBlob ev = fetch_embedding(g, v.id);
            for (size_t i = 0; i < us.size() && us[i] < v.id; ++i) {
                int label = Nu[i].count(v.id) ? 1 : 0;
                double s = (o.method == "cosine") ? linkpred::cosine(eu[i].data, ev.data)
                                                  : linkpred::dot(eu[i].data, ev.data);
                top.offer({us[i], v.id, s, label});
                r.pairs_scored++;
                if (label == 1) r.true_edges++;
            }
            b->next(&v);
        }
    }

    a->close();
    b->close();
    delete a;
    delete b;
    r.top_k = top.drain_sorted();
    return r;
}


// ================================================================
// fof: friends-of-friends (2-step walks, u -> w -> v)
// ================================================================



// ---------------------------------------------------------------- fof, embedding cursor (cosine / dot)
//
// run_reference_fof_embedding_cursor
//   Selected by: --impl=reference --candidates=fof --method=cosine|dot --variant=cursor
//   Candidates: same as run_reference_fof_structural_cursor (u -> w -> v, v > u)
//   Per pair:   fetch_embedding(v)                     : DB read (replaces N(v))
inline RunResult run_reference_fof_embedding_cursor(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        std::unordered_set<node_id_t> fof;
        for (node_id_t w : Nu)
            for (node_id_t v : g->get_out_nodes_id(w))
                if (v > u.id) fof.insert(v);
        std::vector<node_id_t> vs(fof.begin(), fof.end());
        std::sort(vs.begin(), vs.end());

        EmbeddingBlob eu = fetch_embedding(g, u.id);       // u's embedding, once per u
        for (node_id_t v : vs) {
            EmbeddingBlob ev = fetch_embedding(g, v);       // DB read: v's embedding
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine") ? linkpred::cosine(eu.data, ev.data)
                                              : linkpred::dot(eu.data, ev.data);
            top.offer({u.id, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
        a->next(&u);
    }

    a->close();
    delete a;
    r.top_k = top.drain_sorted();
    return r;
}



// ================================================================
// khop: within --khop hops (BFS or nested traversal, various access paths)
// ================================================================



// ---------------------------------------------------------------- khop, embedding cursor_bfs (cosine / dot)
//
// run_reference_khop_embedding_cursor_bfs
//   Selected by: --impl=reference --candidates=khop --method=cosine|dot
//                --variant=cursor (reads --khop=N, any N >= 1)
//   Candidates: same as run_reference_khop_structural_cursor_bfs (BFS to depth
//               --khop, v > u)
//   Per pair:   fetch_embedding(v)                     : DB read (replaces N(v))
inline RunResult run_reference_khop_embedding_cursor_bfs(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        std::unordered_map<node_id_t, int> dist;
        std::queue<node_id_t> q;
        dist[u.id] = 0;
        q.push(u.id);
        while (!q.empty()) {
            node_id_t x = q.front(); q.pop();
            int dx = dist[x];
            if (dx == o.k_hops) continue;
            for (node_id_t y : g->get_out_nodes_id(x))
                if (dist.find(y) == dist.end()) {
                    dist[y] = dx + 1;
                    q.push(y);
                }
        }
        std::vector<node_id_t> vs;
        for (const auto &[v, d] : dist)
            if (v > u.id && d >= 1) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        EmbeddingBlob eu = fetch_embedding(g, u.id);       // u's embedding, once per u
        for (node_id_t v : vs) {
            EmbeddingBlob ev = fetch_embedding(g, v);       // DB read: v's embedding
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine") ? linkpred::cosine(eu.data, ev.data)
                                              : linkpred::dot(eu.data, ev.data);
            top.offer({u.id, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
        a->next(&u);
    }

    a->close();
    delete a;
    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- khop, embedding nested (cosine / dot)
//
// run_reference_khop_embedding_nested
//   Selected by: --impl=reference --candidates=khop --method=cosine|dot
//                --variant=nested --khop=2  (k = 2 only)
//   Traversal:  same nested-loop traversal as _khop_structural_nested
//   Per pair:   fetch_embedding(v)                        : DB read (replaces N(v))
//
// Same candidate set as run_reference_khop_embedding_cursor_bfs.
inline RunResult run_reference_khop_embedding_nested(GraphBase *g, const Options &o)
{
    if (o.k_hops != 2) {
        fprintf(stderr, "ERROR: khop_embedding_nested is k=2 only (got k=%d)\n", o.k_hops);
        return RunResult{};
    }

    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        std::unordered_set<node_id_t> cand;
        for (node_id_t w : Nu) {
            cand.insert(w);
            for (node_id_t v : g->get_out_nodes_id(w))
                cand.insert(v);
        }
        cand.erase(u.id);

        std::vector<node_id_t> vs;
        for (node_id_t v : cand)
            if (v > u.id) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        EmbeddingBlob eu = fetch_embedding(g, u.id);
        for (node_id_t v : vs) {
            EmbeddingBlob ev = fetch_embedding(g, v);
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine") ? linkpred::cosine(eu.data, ev.data)
                                              : linkpred::dot(eu.data, ev.data);
            top.offer({u.id, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
        a->next(&u);
    }

    a->close();
    delete a;
    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- khop, embedding outcursor_nested (cosine / dot)
//
// run_reference_khop_embedding_outcursor_nested
//   Selected by: --impl=reference --candidates=khop --method=cosine|dot
//                --variant=outcursor_nested --khop=2  (k = 2 only)
//   Traversal:  same nested loops as _khop_embedding_nested : k = 2 only
//   Adjacency:  OutCursor (reset + set_key_range({x, x}) + next) for N(u), each N(w)
//   Per pair:   fetch_embedding(v)                        : DB read (unchanged)
//
// Embedding counterpart of _khop_structural_outcursor_nested. Only the traversal
// reads move to the OutCursor; the per-candidate blob read is the same as
// _khop_embedding_nested, so expect a smaller gain than in the structural case.
// Same OutCursor caveat: the graph's shared cursor, reset at the end, never closed.
inline RunResult run_reference_khop_embedding_outcursor_nested(GraphBase *g, const Options &o)
{
    if (o.k_hops != 2) {
        fprintf(stderr, "ERROR: khop_embedding_outcursor_nested is k=2 only (got k=%d)\n", o.k_hops);
        return RunResult{};
    }

    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    OutCursor *oc = g->get_outnbd_iter();
    adjlist al{0, 0};
    auto out_list = [&](node_id_t x) -> std::vector<node_id_t> {
        oc->reset();
        oc->set_key_range({x, x});
        al.edgelist.clear(); al.degree = 0;      // split_ekey OutCursor appends; adj overwrites
        oc->next(&al);
        if (al.node_id != x) return {};          // no adjlist row for x
        return al.edgelist;
    };

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        std::vector<node_id_t> Nu = out_list(u.id);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        std::unordered_set<node_id_t> cand;
        for (node_id_t w : Nu) {
            cand.insert(w);
            for (node_id_t v : out_list(w))
                cand.insert(v);
        }
        cand.erase(u.id);

        std::vector<node_id_t> vs;
        for (node_id_t v : cand)
            if (v > u.id) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        EmbeddingBlob eu = fetch_embedding(g, u.id);
        for (node_id_t v : vs) {
            EmbeddingBlob ev = fetch_embedding(g, v);
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine") ? linkpred::cosine(eu.data, ev.data)
                                              : linkpred::dot(eu.data, ev.data);
            top.offer({u.id, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
        a->next(&u);
    }

    a->close();
    delete a;
    oc->reset();                                         // shared cursor: reset, don't close
    delete oc;
    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- khop, embedding nested_cached (cosine / dot)
//
// run_reference_khop_embedding_nested_cached
//   Selected by: --impl=reference --candidates=khop --method=cosine|dot
//                --variant=nested_cached --khop=2  (k = 2 only)
//   Traversal:  same nested loops as _khop_embedding_nested : k = 2 only
//   Per pair:   cache[v]; on a miss fetch_embedding(v)    : DB read at most once per node
//
// Embedding counterpart of "don't read v again": cosine needs v's vector, so it
// can't be counted like CN, but the same v is a candidate for many u's. The
// cache keeps every blob after its first read (memory grows toward all n
// embeddings, n x dim x 4 bytes). Same pairs, scores and top-k as
// _khop_embedding_nested.
inline RunResult run_reference_khop_embedding_nested_cached(GraphBase *g, const Options &o)
{
    if (o.k_hops != 2) {
        fprintf(stderr, "ERROR: khop_embedding_nested_cached is k=2 only (got k=%d)\n", o.k_hops);
        return RunResult{};
    }

    RunResult r;
    linkpred::TopK top((size_t)o.topk);
    std::unordered_map<node_id_t, EmbeddingBlob> cache;
    auto emb = [&](node_id_t x) -> const EmbeddingBlob & {
        auto it = cache.find(x);
        if (it == cache.end()) it = cache.emplace(x, fetch_embedding(g, x)).first;
        return it->second;
    };

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        std::unordered_set<node_id_t> cand;
        for (node_id_t w : Nu) {
            cand.insert(w);
            for (node_id_t v : g->get_out_nodes_id(w))
                cand.insert(v);
        }
        cand.erase(u.id);

        std::vector<node_id_t> vs;
        for (node_id_t v : cand)
            if (v > u.id) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        const EmbeddingBlob &eu = emb(u.id);
        for (node_id_t v : vs) {
            const EmbeddingBlob &ev = emb(v);
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine") ? linkpred::cosine(eu.data, ev.data)
                                              : linkpred::dot(eu.data, ev.data);
            top.offer({u.id, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
        a->next(&u);
    }

    a->close();
    delete a;
    r.top_k = top.drain_sorted();
    return r;
}

// ================================================================
// uniformRandom: embedding scoring (candidates drawn by uniformRandom_candidates,
// defined in linkpred_reference_structural.h; see the notes there)
// ================================================================


// ---------------------------------------------------------------- uniformRandom, embedding cursor (cosine / dot)
//
// run_reference_uniformRandom_embedding_cursor
//   Selected by: --impl=reference --candidates=uniformRandom --method=cosine|dot
//                --variant=cursor (reads --samples=K --seed=S)
inline RunResult run_reference_uniformRandom_embedding_cursor(GraphBase *g, const Options &o,
                                                       int64_t k_per_node = 100,
                                                       uint64_t seed = 42, bool filter = true)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);
    std::mt19937_64 rng(seed);
    node_id_t max_id = g->get_max_node_id();

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);
    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);                // labels only
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());
        EmbeddingBlob eu = fetch_embedding(g, u.id);

        for (node_id_t v : uniformRandom_candidates(g, u.id, max_id, k_per_node, filter, rng)) {
            EmbeddingBlob ev = fetch_embedding(g, v);
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine") ? linkpred::cosine(eu.data, ev.data)
                                              : linkpred::dot(eu.data, ev.data);
            top.offer({u.id, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
        a->next(&u);
    }
    a->close();
    delete a;
    r.top_k = top.drain_sorted();
    return r;
}
