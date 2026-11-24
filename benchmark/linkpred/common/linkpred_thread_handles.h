// linkpred_thread_handles.h -- per-thread read-only graph handles.
//
// One handle per worker thread: WiredTiger sessions and cursors are NOT
// thread-safe, so each thread reads through its own handle,
// g_thread_handles[tid]. Always holds o.threads handles, including when
// threads == 1. Filled and closed by the bench (fg_linkpred_bench_db.cpp:
// engine.create_ro_graph_handle per thread); used by the direct
// (exhaustive, fof, khop) and pipeline-generator workers.
//
// Moved out of linkpred_exhaustive.h (2026-11-16) so fof, khop and the
// pipeline no longer include the exhaustive file just for this.

#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "graph.h"

inline std::vector<GraphBase*> g_thread_handles;   // inline: defined in a header

// thread_out_cursor: one OutCursor per (thread, handle), opened on first use and
// reused (reset()) for every 64-id block. Use it in per-block workers instead of
// calling get_outnbd_iter() per block. Do not close() or delete the result; the
// handle's session closes it at the end of the bench.
//
// Why not get_outnbd_iter() per block:
//  1. Every call opens a NEW WiredTiger cursor (session->open_cursor).
//  2. OutCursor's destructor is empty, so `delete oc` frees only the wrapper and
//     leaves the WiredTiger cursor open. One call per block = one open cursor per
//     block. Blocks = max id / 64: web-Google ~14K (unnoticed), ego-Twitter
//     (max id 568,770,231) ~8.9M -> OOM-killed, exit 137, 2026-11-17.
//  3. Before 30b1c40 (= Puneet's 8d1f2e4, cherry-picked 2026-11-22) the getter also
//     opened into the handle's out_adjlist_cursor member, which has_node and
//     get_out_nodes_id use, so close() would have broken those calls. With 30b1c40
//     the cursor is the caller's alone and `oc->close(); delete oc;` per block is
//     correct too, but it still opens a cursor per block (millions on sparse ids).
// One cursor per thread avoids both, with or without 30b1c40.
inline OutCursor *thread_out_cursor(GraphBase *g)
{
    thread_local std::unordered_map<GraphBase *, OutCursor *> cache;
    OutCursor *&oc = cache[g];
    if (!oc) oc = g->get_outnbd_iter();
    return oc;
}

// ---------------------------------------------------------------- neighbour direction
// --direction=both on a directed DB (opened with -d): every neighbour list used for
// candidates, scores and labels is N(x) = out(x) U in(x), sorted and deduplicated,
// so the graph is scored as undirected (as fg_fastrp, Graphalytics WCC/LCC/CDLP,
// NetworKit and PyG do). --direction=out, or an undirected DB: out(x) only, as
// before 2026-11-22. Undirected DBs are unchanged either way (their in-list cursor
// reads the out-list table, so it is skipped).
inline bool g_graph_directed = false;   // set by the bench from opts.is_directed (-d)

inline bool lp_both_directions(const std::string &direction)
{
    return g_graph_directed && direction == "both";
}

// One InCursor per (thread, handle), reused like thread_out_cursor. In-lists are
// read through the cursor, not get_in_nodes_id, which returns {} for nodes with
// no out-edges on bulk-loaded directed DBs (has_node checks adjlistout).
inline InCursor *thread_in_cursor(GraphBase *g)
{
    thread_local std::unordered_map<GraphBase *, InCursor *> cache;
    InCursor *&ic = cache[g];
    if (!ic) ic = g->get_innbd_iter();
    return ic;
}

// Append x's in-neighbours to N, then sort and deduplicate N (N may arrive unsorted).
inline void lp_add_in_neighbours(GraphBase *g, node_id_t x, std::vector<node_id_t> &N)
{
    InCursor *ic = thread_in_cursor(g);
    adjlist al{0, 0};
    ic->reset();
    ic->set_key_range({x, x});
    al.edgelist.clear(); al.degree = 0;
    ic->next(&al);
    if (al.node_id == x) N.insert(N.end(), al.edgelist.begin(), al.edgelist.end());
    ic->reset();
    std::sort(N.begin(), N.end());
    N.erase(std::unique(N.begin(), N.end()), N.end());
}

// ---------------------------------------------------------------- held-out split
// --eval=holdout (2026-11-22, the basic version): the test pairs from --split-file
// (tools/make_edge_split.py, undirected {u, v}) are removed from every neighbour list
// in both directions, so candidates and scores only see the training graph; a
// candidate is labelled 1 if it is a test pair, and pairs that are training edges
// (already known) are not scored. --eval=recon: unchanged.
inline bool g_holdout = false;
inline std::unordered_set<uint64_t> g_test_pairs;

inline uint64_t lp_pair_key(node_id_t a, node_id_t b)
{
    if (a > b) std::swap(a, b);
    return ((uint64_t)a << 32) ^ (uint64_t)b;   // ids < 2^32 (largest here: 568,770,231)
}
inline bool lp_is_test_pair(node_id_t a, node_id_t b)
{
    return g_test_pairs.count(lp_pair_key(a, b)) != 0;
}
// Load the test pairs; returns how many were read (0 on error).
inline size_t lp_load_split(const std::string &path)
{
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        unsigned long long u = 0, v = 0;
        if (std::sscanf(line.c_str(), "%llu %llu", &u, &v) == 2)
            g_test_pairs.insert(lp_pair_key((node_id_t)u, (node_id_t)v));
    }
    g_holdout = !g_test_pairs.empty();
    return g_test_pairs.size();
}
// Remove x's test pairs from N (no-op unless --eval=holdout).
inline void lp_drop_test_pairs(node_id_t x, std::vector<node_id_t> &N)
{
    if (!g_holdout) return;
    N.erase(std::remove_if(N.begin(), N.end(),
                           [&](node_id_t w) { return lp_is_test_pair(x, w); }), N.end());
}
// Label for a scored candidate (u, v). in_train = v is in u's (training) list.
//   recon:   1 if an edge, else 0 (as before).
//   holdout: -1 = training edge, skip it; 1 = test pair; 0 = neither.
inline int lp_label(node_id_t u, node_id_t v, bool in_train)
{
    if (!g_holdout) return in_train ? 1 : 0;
    if (in_train) return -1;
    return lp_is_test_pair(u, v) ? 1 : 0;
}

// Neighbour list for the API-read workers: get_out_nodes_id, plus in-list when
// both, minus held-out test pairs.
inline std::vector<node_id_t> lp_nbrs(GraphBase *g, node_id_t x, bool both)
{
    std::vector<node_id_t> N = g->get_out_nodes_id(x);
    if (both) lp_add_in_neighbours(g, x, N);
    lp_drop_test_pairs(x, N);
    return N;
}
