// linkpred_reference_structural.h -- plain single-threaded reference, structural family (cn / jaccard).
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




// ================================================================
// Structural exhaustive: variants differing in v enumeration and adjacency source
// ================================================================

// ---------------------------------------------------------------- exhaustive, structural cursor (cn / jaccard)
//
// run_reference_exhaustive_structural_cursor
//   Selected by: --impl=reference --candidates=exhaustive --variant=cursor
//                (empty --candidates also normalizes here for reference)
//   In memory:  Nu (u's neighbours), Nv (v's neighbours), Nu_set (hash set)
//   Per pair:   get_out_nodes_id(v) --> Nv            : DB read
//   u source:   a->next(&u)                            : cursor walk
//   v source:   b->next(&v)                            : cursor walk
inline RunResult run_reference_exhaustive_structural_cursor(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();             // outer: u
    NodeCursor *b = g->get_node_iter();             // inner: v > u
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        b->reset();
        b->set_key_range({u.id + 1, OutOfBand_ID_MAX});
        node v;
        b->next(&v);

        while (v.id != OutOfBand_ID_MAX) {
            auto Nv = g->get_out_nodes_id(v.id);
            std::sort(Nv.begin(), Nv.end());

            int label = Nu_set.count(v.id) ? 1 : 0;
            double s = (o.method == "cn")
                       ? (double)linkpred::common_neighbors(Nu, Nv)
                       : linkpred::jaccard(Nu, Nv);
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


// ---------------------------------------------------------------- exhaustive, structural preloaded_ids (cn / jaccard)
//
// run_reference_exhaustive_structural_preloaded_ids
//   Selected by: --impl=reference --candidates=exhaustive --variant=preloaded_ids
//   In memory:  nodes[] (all node IDs), Nu, Nv, Nu_set
//   Per pair:   get_out_nodes_id(v) --> Nv            : DB read
//   u source:   nodes[i]                               : array index
//   v source:   nodes[j]                               : array index
//
// Preloaded: node ID list ONLY. Neighbour lists still come from DB per
// pair. Saves the cursor advance per v (a few ns) but NOT the DB lookup
// (hundreds of ns). Runtime ends up within 1-2% of _structural_cursor.
inline RunResult run_reference_exhaustive_structural_preloaded_ids(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    // Build the node list once (O(n) cursor walk).
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
        auto Nu = g->get_out_nodes_id(u);
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        for (size_t j = i + 1; j < nodes.size(); ++j) {
            node_id_t v = nodes[j];
            auto Nv = g->get_out_nodes_id(v);
            std::sort(Nv.begin(), Nv.end());

            int label = Nu_set.count(v) ? 1 : 0;
            double s = linkpred::score_pair(o.method, Nu, Nv);
            top.offer({u, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
    }

    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- exhaustive, structural csr (cn / jaccard)
//
// run_reference_exhaustive_structural_csr
//   Selected by: --impl=reference --candidates=exhaustive --variant=csr
//   In memory:  nodes[], adj[] (all neighbour lists, pre-sorted)
//   Per pair:   adj[u], adj[v]                         : memory (no DB)
//   u source:   nodes[i]                               : array index
//   v source:   nodes[j]                               : array index
//
// Preloaded: node IDs AND full adjacency. Zero DB calls in the pair loop.
// This is the ML-standard approach (PyTorch Geometric / DGL style): build
// adjacency once, score from memory.
//
// Cost: one DB pass at start (n calls), O(n+m) memory during the loop.
// Expected 4-6x faster than _structural_cursor on s12_e8. Same tradeoff as
// pvector vs cursor in the embedding family.
//
// Previously named run_reference_exhaustive_structural_preloaded_adj;
// renamed to match the --variant=csr flag.
inline RunResult run_reference_exhaustive_structural_csr(
    GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    node_id_t max_id = g->get_max_node_id();
    if (max_id == OutOfBand_ID_MAX || max_id == 0) {
        fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
                (unsigned long long)max_id);
        return r;
    }

    // Preload: one pass over nodes, fills nodes[] and adj[].
    // Each adj[] entry is pre-sorted so binary_search works for labels.
    std::vector<node_id_t> nodes;
    std::vector<std::vector<node_id_t>> adj((size_t)max_id + 1);
    {
        NodeCursor *nc = g->get_node_iter();
        nc->set_key_range({0, OutOfBand_ID_MAX});
        node n;
        nc->next(&n);
        while (n.id != OutOfBand_ID_MAX) {
            nodes.push_back(n.id);
            adj[n.id] = g->get_out_nodes_id(n.id);
            std::sort(adj[n.id].begin(), adj[n.id].end());
            nc->next(&n);
        }
        nc->close();
        delete nc;
    }

    // Pair loop: zero DB calls.
    for (size_t i = 0; i < nodes.size(); ++i) {
        node_id_t u = nodes[i];
        const std::vector<node_id_t> &Nu = adj[u];

        for (size_t j = i + 1; j < nodes.size(); ++j) {
            node_id_t v = nodes[j];
            const std::vector<node_id_t> &Nv = adj[v];

            int label = std::binary_search(Nu.begin(), Nu.end(), v) ? 1 : 0;
            double s = linkpred::score_pair(o.method, Nu, Nv);
            top.offer({u, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
    }

    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- exhaustive, structural tiled (cn / jaccard)
//
// run_reference_exhaustive_structural_tiled
//   Selected by: --impl=reference --candidates=exhaustive --variant=tiled
//                (reads --tile=N)
//   In memory:  one block of up to --tile u IDs and their N(u) lists
//   Per pair:   nothing from the DB; N(v) is read once per (block, v)
//   u source:   cursor a, taken --tile at a time         : cursor walk
//   v source:   cursor b, reset to just after the block's first u : cursor walk
//
// Single-threaded counterpart of --impl=direct --variant=tiled, written
// independently: blocks are the next --tile nodes cursor a returns, not fixed
// ID ranges. No node-ID preload (that would be _preloaded_ids + tiling): only
// the current block is held in memory. Each N(v) is read ~n / tile times instead
// of once per pair (_structural_cursor) or once in total (_structural_csr).
inline RunResult run_reference_exhaustive_structural_tiled(GraphBase *g, const Options &o)
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
    std::vector<std::vector<node_id_t>> Nu;
    while (u.id != OutOfBand_ID_MAX) {
        // The next block of u's, read once.
        us.clear();
        Nu.clear();
        while (u.id != OutOfBand_ID_MAX && us.size() < T) {
            us.push_back(u.id);
            Nu.push_back(g->get_out_nodes_id(u.id));
            std::sort(Nu.back().begin(), Nu.back().end());
            a->next(&u);
        }

        // Every v after the block's first u, read once for the whole block.
        b->reset();
        b->set_key_range({us.front() + 1, OutOfBand_ID_MAX});
        node v;
        b->next(&v);
        while (v.id != OutOfBand_ID_MAX) {
            auto Nv = g->get_out_nodes_id(v.id);
            std::sort(Nv.begin(), Nv.end());
            for (size_t i = 0; i < us.size() && us[i] < v.id; ++i) {
                int label = std::binary_search(Nu[i].begin(), Nu[i].end(), v.id) ? 1 : 0;
                double s = linkpred::score_pair(o.method, Nu[i], Nv);
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

// ---------------------------------------------------------------- exhaustive, structural no_reset (EXPERIMENT, not wired)
//
// run_reference_exhaustive_structural_no_reset
//   Selected by: (not wired; commented out in fg_linkpred_bench_reference.cpp)
//   In memory:  same as _structural_cursor
//   Per pair:   same as _structural_cursor
//   u source:   a->next(&u)                            : cursor walk
//   v source:   b->next(&v), NO reset between u's      : cursor walk
//
// Tests whether b->set_key_range alone can reposition a WT cursor past
// its sentinel. If yes, reset() is unnecessary. If no, reset() is
// required and this variant produces garbage.
//
// Result on s12_e8: 4090 pairs produced instead of 8.37M. Confirms that
// reset() IS required. Do not remove reset() from any variant.
inline RunResult run_reference_exhaustive_structural_no_reset(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();
    NodeCursor *b = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        // NO b->reset() -- experiment: does set_key_range reposition on its own?
        b->set_key_range({u.id + 1, OutOfBand_ID_MAX});
        node v;
        b->next(&v);

        while (v.id != OutOfBand_ID_MAX) {
            auto Nv = g->get_out_nodes_id(v.id);
            std::sort(Nv.begin(), Nv.end());

            int label = Nu_set.count(v.id) ? 1 : 0;
            double s = linkpred::score_pair(o.method, Nu, Nv);
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


// ---------------------------------------------------------------- exhaustive, structural fresh_cursor (EXPERIMENT, not wired)
//
// run_reference_exhaustive_structural_fresh_cursor
//   Selected by: (not wired; commented out in fg_linkpred_bench_reference.cpp)
//   In memory:  same as _structural_cursor
//   Per pair:   same as _structural_cursor
//   u source:   a->next(&u)                            : cursor walk (reused)
//   v source:   fresh cursor b, opened per u, closed   : new cursor per u
//
// Tests the cost of cursor reuse. Predicted slower than _structural_cursor
// (4096 cursor create/close cycles vs 4096 resets). Confirms that
// reuse-via-reset is worth keeping.
//
// Result on s12_e8: ~42% slower than _structural_cursor. Justifies the
// per-thread cursor reuse pattern in the pipeline runner.
inline RunResult run_reference_exhaustive_structural_fresh_cursor(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        NodeCursor *b = g->get_node_iter();           // <-- fresh per u
        b->set_key_range({u.id + 1, OutOfBand_ID_MAX});
        node v;
        b->next(&v);

        while (v.id != OutOfBand_ID_MAX) {
            auto Nv = g->get_out_nodes_id(v.id);
            std::sort(Nv.begin(), Nv.end());

            int label = Nu_set.count(v.id) ? 1 : 0;
            double s = linkpred::score_pair(o.method, Nu, Nv);
            top.offer({u.id, v.id, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;

            b->next(&v);
        }
        b->close();                                   // <-- close per u
        delete b;
        a->next(&u);
    }

    a->close();
    delete a;
    r.top_k = top.drain_sorted();
    return r;
}


// ================================================================
// fof: friends-of-friends (2-step walks, u -> w -> v)
// ================================================================

// ---------------------------------------------------------------- fof, structural cursor (cn / jaccard)
//
// run_reference_fof_structural_cursor
//   Selected by: --impl=reference --candidates=fof --variant=cursor
//   In memory:  Nu (u's neighbors), Nu_set (label lookup), fof (2-hop set)
//   Per pair:   get_out_nodes_id(v) --> Nv             : DB read
//   u source:   a->next(&u)                            : cursor walk
//   v source:   fof set built via u -> w -> v, sorted  : memory after build
//
// Candidate set: every v reachable by u -> w -> v with v != u, v > u.
// Matches FofCandidates(skip_edges=false): direct neighbours ARE included
// if they happen to also be 2-hop reachable (so the runner can label them 1).
inline RunResult run_reference_fof_structural_cursor(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        // Build the fof candidate set for this u (skip_edges=false).
        std::unordered_set<node_id_t> fof;
        for (node_id_t w : Nu)
            for (node_id_t v : g->get_out_nodes_id(w)) {
                if (v == u.id)  continue;
                if (v > u.id)   fof.insert(v);
            }

        std::vector<node_id_t> vs(fof.begin(), fof.end());
        std::sort(vs.begin(), vs.end());

        for (node_id_t v : vs) {
            auto Nv = g->get_out_nodes_id(v);
            std::sort(Nv.begin(), Nv.end());

            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cn")
                       ? (double)linkpred::common_neighbors(Nu, Nv)
                       : linkpred::jaccard(Nu, Nv);
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


// ---------------------------------------------------------------- fof, structural wedge (cn / jaccard)
//
// run_reference_fof_structural_wedge
//   Selected by: --impl=reference --candidates=fof --variant=wedge
//   Candidates: same as run_reference_fof_structural_cursor (v > u, two steps away)
//   Scoring:    CN = number of wedges u → w → v (counted in a hash map),
//               |N(v)| for Jaccard from get_out_degree(v); no N(v) list read
//
// Single-threaded counterpart of --candidates=fof --variant=wedge, written
// independently (hash map + degree lookups instead of the direct path's
// counter array + degree array). Same pairs, true edges and top-k as
// run_reference_fof_structural_cursor on a graph stored in both directions.
inline RunResult run_reference_fof_structural_wedge(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);
    while (u.id != OutOfBand_ID_MAX) {
        std::vector<node_id_t> Nu = g->get_out_nodes_id(u.id);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());
        std::unordered_map<node_id_t, int64_t> wedges;
        for (node_id_t w : Nu)
            for (node_id_t v : g->get_out_nodes_id(w))
                if (v > u.id) wedges[v]++;

        std::vector<node_id_t> vs;
        vs.reserve(wedges.size());
        for (const auto &kv : wedges) vs.push_back(kv.first);
        std::sort(vs.begin(), vs.end());
        for (node_id_t v : vs) {
            int64_t inter = wedges[v];
            double s;
            if (o.method == "cn") {
                s = (double)inter;
            } else {
                int64_t uni = (int64_t)Nu.size() + (int64_t)g->get_out_degree(v) - inter;
                s = uni > 0 ? (double)inter / (double)uni : 0.0;
            }
            int label = Nu_set.count(v) ? 1 : 0;
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

// ---------------------------------------------------------------- khop, structural cursor_bfs (cn / jaccard)
//
// run_reference_khop_structural_cursor_bfs
//   Selected by: --impl=reference --candidates=khop --variant=cursor
//                (reads --khop=N, any N >= 1)
//   In memory:  Nu (u's neighbors), Nu_set (label lookup), dist (BFS map)
//   Per pair:   get_out_nodes_id(v) --> Nv             : DB read
//   u source:   a->next(&u)                            : cursor walk
//   v source:   BFS from u to depth o.k_hops, sorted   : memory after build
//
// Candidate set: every v within o.k_hops hops of u, v != u, v > u.
// Matches KHopCandidates(skip_edges=false): direct neighbours (dist 1) ARE
// included (min_dist=1).
inline RunResult run_reference_khop_structural_cursor_bfs(GraphBase *g, const Options &o)
{
    RunResult r;
    linkpred::TopK top((size_t)o.topk);

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        // BFS from u to depth o.k_hops. skip_edges=false -> min_dist=1
        // (include direct neighbours; they get labelled 1 below).
        std::unordered_map<node_id_t, int> dist;
        std::queue<node_id_t> q;
        dist[u.id] = 0;
        q.push(u.id);
        while (!q.empty()) {
            node_id_t x = q.front(); q.pop();
            int dx = dist[x];
            if (dx == o.k_hops) continue;
            for (node_id_t y : g->get_out_nodes_id(x)) {
                if (dist.find(y) == dist.end()) {
                    dist[y] = dx + 1;
                    q.push(y);
                }
            }
        }

        // Collect v > u with dist 1..k, sort, iterate.
        std::vector<node_id_t> vs;
        for (const auto &[v, d] : dist)
            if (v > u.id && d >= 1) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        for (node_id_t v : vs) {
            auto Nv = g->get_out_nodes_id(v);
            std::sort(Nv.begin(), Nv.end());

            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cn")
                       ? (double)linkpred::common_neighbors(Nu, Nv)
                       : linkpred::jaccard(Nu, Nv);
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

// ---------------------------------------------------------------- khop, structural outcursor_bfs (cn / jaccard)
//
// run_reference_khop_structural_outcursor_bfs
//   Selected by: --impl=reference --candidates=khop --variant=outcursor_bfs
//                (reads --khop=N, any N >= 1)
//   In memory:  Nu (u's neighbours), Nu_set (label lookup), dist (BFS map)
//   Adjacency:  OutCursor oc, reset + set_key_range({x, x}) + next(&adjlist)
//               for every frontier node and every candidate v       : DB cursor
//   u source:   a->next(&u)                            : cursor walk
//   v source:   BFS from u to depth o.k_hops, sorted   : memory after build
//
// Same candidates, pairs and scores as run_reference_khop_structural_cursor_bfs;
// only the adjacency read differs: the graph's OutCursor positioned on x
// instead of get_out_nodes_id(x) (which adds a has_node() search and a vector
// copy). A node with no out-edges has no adjlist row, so the cursor lands on
// the next node (or the end); that case is checked and treated as an empty list.
inline RunResult run_reference_khop_structural_outcursor_bfs(GraphBase *g, const Options &o)
{
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
        std::vector<node_id_t> nx = al.edgelist;
        std::sort(nx.begin(), nx.end());
        return nx;
    };

    NodeCursor *a = g->get_node_iter();
    a->set_key_range({0, OutOfBand_ID_MAX});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        std::vector<node_id_t> Nu = out_list(u.id);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        std::unordered_map<node_id_t, int> dist;
        std::queue<node_id_t> q;
        dist[u.id] = 0;
        q.push(u.id);
        while (!q.empty()) {
            node_id_t x = q.front(); q.pop();
            int dx = dist[x];
            if (dx == o.k_hops) continue;
            for (node_id_t y : (x == u.id ? Nu : out_list(x)))
                if (dist.find(y) == dist.end()) {
                    dist[y] = dx + 1;
                    q.push(y);
                }
        }

        std::vector<node_id_t> vs;
        for (const auto &[v, d] : dist)
            if (v > u.id && d >= 1) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        for (node_id_t v : vs) {
            std::vector<node_id_t> Nv = out_list(v);
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cn")
                       ? (double)linkpred::common_neighbors(Nu, Nv)
                       : linkpred::jaccard(Nu, Nv);
            top.offer({u.id, v, s, label});
            r.pairs_scored++;
            if (label == 1) r.true_edges++;
        }
        a->next(&u);
    }

    a->close();
    delete a;
    // AdjList::get_outnbd_iter() returns the graph's own out_adjlist_cursor,
    // not a new one: close() would close it for every later get_out_nodes_id
    // on this handle (segfault in the next reference run). Reset, don't close.
    oc->reset();
    delete oc;
    r.top_k = top.drain_sorted();
    return r;
}


// ---------------------------------------------------------------- khop, structural nested (cn / jaccard)
//
// run_reference_khop_structural_nested
//   Selected by: --impl=reference --candidates=khop --variant=nested --khop=2
//                (k = 2 only)
//   Traversal:  nested loops over adjacency (NOT BFS)     : k = 2 only
//   Adjacency:  get_out_nodes_id(x)                       : DB read
//   u source:   a->next(&u)                               : cursor walk
//   v source:   union of N(w) for w in N(u), deduped      : memory after build
//
// Same candidate set as run_reference_khop_structural_cursor_bfs:
//   { v : 1 <= dist(u, v) <= 2, v != u, v > u }
// Independent implementation: BFS expands a frontier via a queue + dist map;
// this walks every (w, v) path explicitly. A BFS-specific bug (wrong cutoff,
// wrong dedup, wrong visit order) surfaces as a pairs_scored / top_k mismatch.
//
// The cand.insert(w) before the inner loop is what makes this khop(2) rather
// than FoF — it keeps direct neighbours that are NOT 2-hop reachable (leaves).
inline RunResult run_reference_khop_structural_nested(GraphBase *g, const Options &o)
{
    if (o.k_hops != 2) {
        fprintf(stderr, "ERROR: khop_structural_nested is k=2 only (got k=%d)\n", o.k_hops);
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
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        // Nested 2-hop traversal.
        std::unordered_set<node_id_t> cand;
        for (node_id_t w : Nu) {
            cand.insert(w);                              // hop 1: direct neighbour
            for (node_id_t v : g->get_out_nodes_id(w))   // hop 2: neighbour's neighbour
                cand.insert(v);
        }
        cand.erase(u.id);

        std::vector<node_id_t> vs;
        for (node_id_t v : cand)
            if (v > u.id) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        for (node_id_t v : vs) {
            auto Nv = g->get_out_nodes_id(v);
            std::sort(Nv.begin(), Nv.end());

            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cn")
                       ? (double)linkpred::common_neighbors(Nu, Nv)
                       : linkpred::jaccard(Nu, Nv);
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


// ---------------------------------------------------------------- khop, structural outcursor_nested (cn / jaccard)
//
// run_reference_khop_structural_outcursor_nested
//   Selected by: --impl=reference --candidates=khop --variant=outcursor_nested
//                --khop=2  (k = 2 only)
//   Traversal:  same nested loops as _khop_structural_nested : k = 2 only
//   Adjacency:  OutCursor oc, reset + set_key_range({x, x}) + next(&adjlist)
//               for u, every w in N(u) and every candidate v           : DB cursor
//   u source:   a->next(&u)                               : cursor walk
//   v source:   N(u) plus N(w) for w in N(u), deduped     : memory after build
//
// Fills the grid {bfs, nested} x {get_out_nodes_id, outcursor}: same pairs and
// scores as the other three; differs from _nested only in the adjacency read,
// and from _outcursor_bfs only in the traversal. Same OutCursor caveat: the
// cursor is the graph's own, so reset it at the end, never close it.
inline RunResult run_reference_khop_structural_outcursor_nested(GraphBase *g, const Options &o)
{
    if (o.k_hops != 2) {
        fprintf(stderr, "ERROR: khop_structural_outcursor_nested is k=2 only (got k=%d)\n", o.k_hops);
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
        std::vector<node_id_t> nx = al.edgelist;
        std::sort(nx.begin(), nx.end());
        return nx;
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
            cand.insert(w);                              // hop 1: direct neighbour
            for (node_id_t v : out_list(w))              // hop 2: neighbour's neighbour
                cand.insert(v);
        }
        cand.erase(u.id);

        std::vector<node_id_t> vs;
        for (node_id_t v : cand)
            if (v > u.id) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        for (node_id_t v : vs) {
            std::vector<node_id_t> Nv = out_list(v);
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cn")
                       ? (double)linkpred::common_neighbors(Nu, Nv)
                       : linkpred::jaccard(Nu, Nv);
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


// ---------------------------------------------------------------- khop, structural wedge (cn / jaccard)
//
// run_reference_khop_structural_wedge
//   Selected by: --impl=reference --candidates=khop --variant=wedge --khop=2
//                (k = 2 only)
//   Traversal:  same nested loops as _khop_structural_nested : k = 2 only
//   Scoring:    CN = cnt[v], incremented once per w in N(u) with v in N(w)
//               (each such w is a shared neighbour of u and v)
//               Jaccard = cnt / (|N(u)| + get_out_degree(v) - cnt)
//   Adjacency:  get_out_nodes_id for N(u) and each N(w) only : no N(v) read
//   u source:   a->next(&u)                               : cursor walk
//
// Same pairs, scores and top-k as _khop_structural_nested (the intersect
// version): direct neighbours that share nothing with u stay in the map with
// cnt 0. The k = 2 counterpart of run_reference_fof_structural_wedge.
inline RunResult run_reference_khop_structural_wedge(GraphBase *g, const Options &o)
{
    if (o.k_hops != 2) {
        fprintf(stderr, "ERROR: khop_structural_wedge is k=2 only (got k=%d)\n", o.k_hops);
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

        // Nested 2-hop traversal, counting instead of storing.
        std::unordered_map<node_id_t, int64_t> cnt;
        for (node_id_t w : Nu) {
            cnt.try_emplace(w, 0);                       // hop 1: candidate, count may stay 0
            for (node_id_t v : g->get_out_nodes_id(w))   // hop 2
                cnt[v]++;                                // w is a shared neighbour of u and v
        }
        cnt.erase(u.id);

        std::vector<node_id_t> vs;
        for (const auto &kv : cnt)
            if (kv.first > u.id) vs.push_back(kv.first);
        std::sort(vs.begin(), vs.end());

        for (node_id_t v : vs) {
            int64_t inter = cnt[v];
            double s;
            if (o.method == "cn") {
                s = (double)inter;
            } else {
                int64_t uni = (int64_t)Nu.size() + (int64_t)g->get_out_degree(v) - inter;
                s = uni > 0 ? (double)inter / (double)uni : 0.0;
            }
            int label = Nu_set.count(v) ? 1 : 0;
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
// uniformRandom: PyG / OGB-style uniform random candidate pairs
// ================================================================
//
// Candidates: for each u (node cursor), draw k_per_node ids v uniformly from
//             (u, max_node_id] -- no graph walk, the id itself is the sample.
//             This is what PyG negative_sampling / OGB random negatives do.
// Filtering (filter = true, default): reject ids that are not nodes
//             (has_node, one probe -- only matters if ids have gaps) and
//             duplicates for the same u. Existing edges are NOT rejected:
//             they are kept and labelled 1, like every other reference.
//             filter = false: raw ids, no probe, duplicates allowed.
// Scoring:    same as fof/khop: N(v) per candidate (structural) or the
//             v blob per candidate (embedding).
// Labels:     Nu_set from get_out_nodes_id(u), read once per u.
// RNG:        std::mt19937_64(seed), u visited in id order -> deterministic.
//
// Contrast: khop2 / fof pick candidates from the 2-hop set (most new links
// close there), uniformRandom picks them blind -- same per-pair cost,
// far fewer true edges among the candidates.

inline std::vector<node_id_t> uniformRandom_candidates(GraphBase *g, node_id_t u, node_id_t max_id,
                                                       int64_t k_per_node, bool filter,
                                                       std::mt19937_64 &rng)
{
    std::vector<node_id_t> vs;
    if (u >= max_id) return vs;
    std::uniform_int_distribution<node_id_t> pick(u + 1, max_id);
    if (!filter) {
        vs.reserve(k_per_node);
        for (int64_t i = 0; i < k_per_node; i++) vs.push_back(pick(rng));
    } else {
        int64_t span = (int64_t)(max_id - u);               // ids available above u
        int64_t want = std::min<int64_t>(k_per_node, span);
        std::unordered_set<node_id_t> seen;
        int64_t tries = 0, max_tries = 4 * want + 64;      // stop if ids are sparse
        while ((int64_t)seen.size() < want && tries++ < max_tries) {
            node_id_t v = pick(rng);
            if (seen.count(v)) continue;
            if (!g->has_node(v)) continue;                  // one probe; skip id gaps
            seen.insert(v);
        }
        vs.assign(seen.begin(), seen.end());
    }
    std::sort(vs.begin(), vs.end());
    return vs;
}

// ---------------------------------------------------------------- uniformRandom, structural cursor (cn / jaccard)
//
// run_reference_uniformRandom_structural_cursor
//   Selected by: --impl=reference --candidates=uniformRandom --variant=cursor
//                (reads --samples=K --seed=S)
inline RunResult run_reference_uniformRandom_structural_cursor(GraphBase *g, const Options &o,
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
        auto Nu = g->get_out_nodes_id(u.id);
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        for (node_id_t v : uniformRandom_candidates(g, u.id, max_id, k_per_node, filter, rng)) {
            auto Nv = g->get_out_nodes_id(v);
            std::sort(Nv.begin(), Nv.end());
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cn")
                       ? (double)linkpred::common_neighbors(Nu, Nv)
                       : linkpred::jaccard(Nu, Nv);
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
