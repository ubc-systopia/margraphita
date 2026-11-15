// linkpred_khop.h — candidate strategy: WITHIN k HOPS (any --khop), hand-written.
//
// Same style as linkpred_exhaustive.h: one worker per 64-node block, candidate
// generation written directly into the loop, threads take blocks by stealing.
// This is the non-pipeline BASELINE for --candidates=khop:
//   --candidates=khop --khop=3 --impl=direct     this file
//   --candidates=khop --khop=3 --impl=pipeline   linkpred_pipeline.h
//                                                (run_pipeline_khop_structural_cursor_bfs)
// Both must give the same pairs_scored, true_edges and top-k.
//
// Candidates for u: BFS from u to depth k (--khop, default 2), every v with
// 1 <= dist(v) <= k and v > u (direct neighbours kept; they are the labels),
// scored in ascending v order. Same set as KHopCandidates(g, k, skip_edges=false).
// Works for any k (--khop, default 2; 3 is the usual k-hop choice).
//
// Notes:
//   - "within k hops", not "exactly k": the 2-hop set is included.
//   - CN is 0 for pairs at distance exactly 3 (no shared neighbour), so with
//     cn / jaccard the 3-hop-only pairs only add zero-score candidates.
//   - Hubs: the 3-hop neighbourhood of a high-degree node can be large
//     (memory per thread = one visited map per u).
//
// Methods: structural (cn / jaccard) and embedding (cosine / dot, same
// candidates, v's embedding read instead of N(v): embedding_worker_khop).
//
// Naming convention
// -----------------
//   run_<impl>_<cand>_<family>_<variant>[_<sub_algorithm>]
//
//   impl        direct  (hand-written, threaded; --threads=1 is the same code)
//   cand        khop
//   family      structural | embedding
//   variant     exactly the --variant= flag value
//   sub_alg     _bfs for the cursor variant (anticipated to grow a nested
//               sibling; wedge is its own variant, see below)
//
// All khop entry points defined here:
//   run_direct_khop_structural_cursor_bfs    --impl=direct --candidates=khop
//                                            --variant=cursor --khop=N (any N)
//   run_direct_khop_structural_wedge         --impl=direct --candidates=khop
//                                            --variant=wedge --khop=2 (k=2 only)
//   run_direct_khop_embedding_cursor_bfs     --impl=direct --candidates=khop
//                                            --method=cosine|dot --variant=cursor
//                                            --khop=N (any N)
//
// Shared implementation note
// --------------------------
// The wedge path shares its entire worker with linkpred_fof.h:
//   structural_worker_fof_wedge(..., with_hop1)  in linkpred_fof.h
//   run_direct_fof_structural_wedge(g, o, with_hop1) in linkpred_fof.h
//   run_direct_khop_structural_wedge(g, o)       here, calls the above with true
// The `with_hop1=true` flag adds u's direct neighbours as candidates (which fof
// excludes), producing the khop2 set instead of the fof set. This is why
// linkpred_fof.h is included below; do not duplicate the worker here without
// also moving the fof entry point.

#pragma once

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <omp.h>
#include "common_defs.h"
#include "graph.h"

#include "linkpred_thread_handles.h"   // g_thread_handles
#include "linkpred_fof.h"          // shared wedge worker + fof wrapper (with_hop1)
#include "linkpred_options.h"
#include "linkpred_scores.h"
#include "linkpred_topk.h"
#include "linkpred_types.h"

// ---------------------------------------------------------------- structural k-hop
// One thread's structural work over one block of u's. BFS to depth k from u,
// then score every v at distance 1..k with v > u using the intersection of
// N(u) and N(v). Any --method in {cn, jaccard}; any --khop >= 1.
//
// Feeds run_direct_khop_structural_cursor_bfs.
static void structural_worker_khop(GraphBase *g,
                                   node_id_t start, node_id_t end,
                                   const Options &o,
                                   linkpred::TopK &top,
                                   size_t &pairs_scored,
                                   size_t &true_edges)
{
    const int k = o.k_hops;
    NodeCursor *a = g->get_node_iter();            // outer: u
    a->set_key_range({start, end});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);       // DB read: u's neighbours
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());   // label lookup

        // candidates: BFS to depth k, v > u, ascending
        std::unordered_map<node_id_t, int> dist;
        std::queue<node_id_t> q;
        dist[u.id] = 0;
        q.push(u.id);
        while (!q.empty()) {
            node_id_t x = q.front(); q.pop();
            int dx = dist[x];
            if (dx == k) continue;                 // don't expand beyond k
            for (node_id_t y : (x == u.id ? Nu : g->get_out_nodes_id(x)))   // reuse N(u)
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
            auto Nv = g->get_out_nodes_id(v);       // DB read: v's neighbours
            std::sort(Nv.begin(), Nv.end());

            int label = Nu_set.count(v) ? 1 : 0;    // ground truth edge?
            double s = linkpred::score_pair(o.method, Nu, Nv);
            top.offer({u.id, v, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
        }
        a->next(&u);
    }

    a->close();
    delete a;
}

// ---- run_direct_khop_structural_cursor_bfs
//   --impl=direct --candidates=khop --variant=cursor --khop=N   (any N >= 1)
//   Threaded. Atomic block stealing over 64-node blocks of u; one TopK heap per
//   thread; merge at the end. --threads=1 runs the same code with one thread.
//
//   The _bfs sub-algorithm token is on the cursor variant because the
//   traversal (BFS vs nested loops) is a distinct implementation choice; a
//   nested sibling for higher k is a plausible future addition and would be
//   run_direct_khop_structural_cursor_nested.
static RunResult run_direct_khop_structural_cursor_bfs(GraphBase *g, const Options &o)
{
    RunResult r;
    node_id_t max_id = g->get_max_node_id();

    // Guard: metadata missing → max_id = UINT64_MAX → num_blocks ≈ 2.88e17
    if (max_id == OutOfBand_ID_MAX || max_id == 0) {
        fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
                (unsigned long long)max_id);
        return r;
    }
    const size_t BLOCK = 64;
    size_t num_blocks = (size_t)(max_id / BLOCK) + 1;

    // ---- Atomic block-stealing (one thread when --threads=1) ----
    std::vector<linkpred::TopK> per_topk;
    std::vector<size_t> per_pairs(o.threads, 0);
    std::vector<size_t> per_edges(o.threads, 0);
    per_topk.reserve(o.threads);
    for (int t = 0; t < o.threads; ++t)
        per_topk.emplace_back((size_t)o.topk);

    std::atomic<size_t> next_block{0};                 // work-stealing counter

    #pragma omp parallel num_threads(o.threads)
    {
        int tid = omp_get_thread_num();
        GraphBase *myg = g_thread_handles[tid];        // private cursor set
        bool shared = (o.accum == "shared");           // --accum, see linkpred_exhaustive.h
        linkpred::TopK my_top((size_t)o.topk);
        size_t my_pairs = 0, my_edges = 0;
        linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
        size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
        size_t         &edges = shared ? per_edges[tid] : my_edges;
        if (myg) {
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < num_blocks) {   // grab next block
                node_id_t start = (node_id_t)(blk * BLOCK);
                node_id_t end   = start + BLOCK - 1;
                if (end > max_id) end = max_id;
                structural_worker_khop(myg, start, end, o, top, pairs, edges);
            }
        }
        if (!shared) {
            per_topk[tid]  = std::move(my_top);
            per_pairs[tid] = my_pairs;
            per_edges[tid] = my_edges;
        }
    }

    for (size_t c : per_pairs) r.pairs_scored += c;
    for (size_t c : per_edges) r.true_edges   += c;
    r.top_k = linkpred::merge_topk(per_topk, (size_t)o.topk);
    return r;
}

// ---- run_direct_khop_structural_wedge
//   --impl=direct --candidates=khop --variant=wedge --khop=2
//   k = 2 only: the wedge trick counts 2-hop paths (u -> w -> v), so it does
//   not generalise to k >= 3 without a different formulation.
//
//   Delegates to run_direct_fof_structural_wedge(g, o, /*with_hop1=*/true).
//   The worker counts CN along the wedges and additionally seeds each direct
//   neighbour w of u with cnt[w] = 1 so that hop-1 nodes appear as candidates
//   even when CN(u, w) = 0. That seeding is exactly what makes the result a
//   khop2 set (fof set ∪ direct neighbours) instead of a fof set.
//
//   The worker lives in linkpred_fof.h; it is not duplicated here because
//   for fof (with_hop1=false) and khop2 (with_hop1=true) the body is identical.
static RunResult run_direct_khop_structural_wedge(GraphBase *g, const Options &o)
{
    if (o.k_hops != 2) {
        fprintf(stderr, "ERROR: khop_structural_wedge is k=2 only (got k=%d)\n", o.k_hops);
        return RunResult{};
    }
    return run_direct_fof_structural_wedge(g, o, /*with_hop1=*/true);
}

// ---------------------------------------------------------------- embedding k-hop (cosine / dot)
// Same candidates as the structural khop path; only the scoring fetch changes:
// v's embedding (get_node_properties) instead of v's neighbour list. One random
// DB read per candidate, like the structural lookup; there is no counted
// shortcut because cosine needs v's vector. --candidates=khop --method=cosine|dot.
//
// Any --khop >= 1; any of --method=cosine|dot.
// Feeds run_direct_khop_embedding_cursor_bfs.
static void embedding_worker_khop(GraphBase *g,
                                  node_id_t start, node_id_t end,
                                  const Options &o,
                                  linkpred::TopK &top,
                                  size_t &pairs_scored,
                                  size_t &true_edges)
{
    NodeCursor *a = g->get_node_iter();            // outer: u
    a->set_key_range({start, end});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);       // DB read: u's neighbours (walk + labels)
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        // candidates: same BFS as structural_worker_khop
        const int k = o.k_hops;
        std::unordered_map<node_id_t, int> dist;
        std::queue<node_id_t> q;
        dist[u.id] = 0;
        q.push(u.id);
        while (!q.empty()) {
            node_id_t x = q.front(); q.pop();
            int dx = dist[x];
            if (dx == k) continue;                 // don't expand beyond k
            for (node_id_t y : (x == u.id ? Nu : g->get_out_nodes_id(x)))   // reuse N(u)
                if (dist.find(y) == dist.end()) {
                    dist[y] = dx + 1;
                    q.push(y);
                }
        }
        std::vector<node_id_t> vs;
        for (const auto &[v, d] : dist)
            if (v > u.id && d >= 1) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        prop_blob pu = g->get_node_properties(u.id);  // u's embedding, once per u
        for (node_id_t v : vs) {
            prop_blob pv = g->get_node_properties(v);    // DB read: v's embedding (replaces N(v))
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine")
                       ? linkpred::cosine_from_bytes(pu.data, pu.size, pv.data, pv.size)
                       : linkpred::dot_from_bytes  (pu.data, pu.size, pv.data, pv.size);
            top.offer({u.id, v, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
            delete[] const_cast<uint8_t*>(pv.data);
        }
        delete[] const_cast<uint8_t*>(pu.data);
        a->next(&u);
    }

    a->close();
    delete a;
}

// ---- run_direct_khop_embedding_cursor_bfs
//   --impl=direct --candidates=khop --method=cosine|dot --variant=cursor --khop=N
//   Threaded. Same block-stealing shape as the structural cursor; the only
//   difference is the worker reads v's embedding per candidate instead of
//   reading N(v) and intersecting.
static RunResult run_direct_khop_embedding_cursor_bfs(GraphBase *g, const Options &o)
{
    RunResult r;
    node_id_t max_id = g->get_max_node_id();
    if (max_id == OutOfBand_ID_MAX || max_id == 0) {
        fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
                (unsigned long long)max_id);
        return r;
    }
    const size_t BLOCK = 64;
    size_t num_blocks = (size_t)(max_id / BLOCK) + 1;

    std::vector<linkpred::TopK> per_topk;
    std::vector<size_t> per_pairs(o.threads, 0);
    std::vector<size_t> per_edges(o.threads, 0);
    per_topk.reserve(o.threads);
    for (int t = 0; t < o.threads; ++t)
        per_topk.emplace_back((size_t)o.topk);

    std::atomic<size_t> next_block{0};                 // work-stealing counter

    #pragma omp parallel num_threads(o.threads)
    {
        int tid = omp_get_thread_num();
        GraphBase *myg = g_thread_handles[tid];        // private cursor set
        bool shared = (o.accum == "shared");
        linkpred::TopK my_top((size_t)o.topk);
        size_t my_pairs = 0, my_edges = 0;
        linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
        size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
        size_t         &edges = shared ? per_edges[tid] : my_edges;
        if (myg) {
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < num_blocks) {   // grab next block
                node_id_t start = (node_id_t)(blk * BLOCK);
                node_id_t end   = start + BLOCK - 1;
                if (end > max_id) end = max_id;
                embedding_worker_khop(myg, start, end, o, top, pairs, edges);
            }
        }
        if (!shared) {
            per_topk[tid]  = std::move(my_top);
            per_pairs[tid] = my_pairs;
            per_edges[tid] = my_edges;
        }
    }

    for (size_t c : per_pairs) r.pairs_scored += c;
    for (size_t c : per_edges) r.true_edges   += c;
    r.top_k = linkpred::merge_topk(per_topk, (size_t)o.topk);
    return r;
}

// ---------------------------------------------------------------- potential variants (not implemented)
// Three cursor-alternative implementations that were sketched but never landed.
// Each would live here under a run_direct_khop_structural_cursor_<sub_algorithm>
// name if implemented, and would need a matrix row and a dispatch branch.
//
//   run_direct_khop_structural_cursor_bitmap        bitset visited map per thread
//   run_direct_khop_structural_cursor_preloaded     preloaded CSR adjacency
//   run_direct_khop_structural_cursor_bidirectional two-sided BFS from u and v
//
// static RunResult run_direct_khop_structural_cursor_bitmap(GraphBase *g, const Options &o);
// static RunResult run_direct_khop_structural_cursor_preloaded(GraphBase *g, const Options &o);
// static RunResult run_direct_khop_structural_cursor_bidirectional(GraphBase *g, const Options &o);