// linkpred_fof.h — candidate strategy: FRIENDS OF FRIENDS (2-step walks), hand-written.
//
// Same style as linkpred_exhaustive.h: one worker per 64-node block, candidate
// generation written directly into the loop, threads take blocks by stealing.
// This is the non-pipeline BASELINE for --candidates=fof:
//   --candidates=fof --impl=direct     this file (run_direct_fof_structural_cursor)
//   --candidates=fof --impl=pipeline   linkpred_pipeline.h (run_pipeline_fof_structural_cursor)
// Both must give the same pairs_scored, true_edges and top-k.
//
// Candidates for u: every v reached by u → w → v with v != u and v > u
// (direct neighbours that are also 2 hops away are kept; they are the labels),
// scored in ascending v order. Same set as FofCandidates(g, skip_edges=false).
//
// Edges are followed in the out direction. CN on these pairs is meaningful when
// the graph stores both directions (DIRECTED: 0); in a directed graph a path
// u → w → v is not a common neighbour and FoF pairs can all score 0.
//
// Methods: structural (cn / jaccard) and embedding (cosine / dot, same
// candidates, v's embedding read instead of N(v): embedding_worker_fof).
//
// Two scorings of the same candidates:
//   --variant=cursor (default)  structural_worker_fof: reads N(v) per candidate
//   --variant=wedge             structural_worker_fof_wedge: counts CN along the
//                               u → w → v walks, no N(v) read (see below)
//
// Naming convention
// -----------------
//   run_<impl>_<cand>_<family>_<variant>[_<sub_algorithm>]
//
//   impl        direct  (hand-written, threaded; --threads=1 is the same code)
//   cand        fof
//   family      structural | embedding
//   variant     exactly the --variant= flag value
//   sub_alg     (none currently; fof has no ambiguous variant)
//
// Functions defined here, and the flags that reach them:
//   run_direct_fof_structural_cursor       --impl=direct --candidates=fof --variant=cursor
//   run_direct_fof_structural_wedge        --impl=direct --candidates=fof --variant=wedge
//   run_direct_fof_embedding_cursor        --impl=direct --candidates=fof
//                                          --method=cosine|dot
//
// NOT defined here (in linkpred_khop.h):
//   run_direct_khop_structural_wedge       --impl=direct --candidates=khop --variant=wedge
//                                          khop2, k=2 only. That function calls
//                                          run_direct_fof_structural_wedge with
//                                          with_hop1=true, which is why the
//                                          with_hop1 parameter exists here.
//                                          See the comment above
//                                          run_direct_fof_structural_wedge for why.
//
// Shared worker note
// ------------------
// structural_worker_fof_wedge takes an internal `with_hop1` parameter:
//   with_hop1=false   pure fof candidates (v reached by u -> w -> v)
//   with_hop1=true    khop2 candidates (also direct neighbours of u);
//                     called only from run_direct_khop_structural_wedge
//                     in linkpred_khop.h.
// The same worker body serves both; the flag only seeds each direct neighbour
// w of u with cnt[w] = 1 so hop-1 nodes appear as candidates with CN = 0.

#pragma once

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <unordered_set>
#include <vector>

#include <omp.h>
#include "common_defs.h"
#include "graph.h"

#include "linkpred_thread_handles.h"   // g_thread_handles
#include "linkpred_options.h"
#include "linkpred_scores.h"
#include "linkpred_topk.h"
#include "linkpred_types.h"

// ---------------------------------------------------------------- structural 2-hop
// One thread's structural work over one block of u's.
//
// Flags: any --method in {cn, jaccard}; feeds run_direct_fof_structural_cursor.
static void structural_worker_fof(GraphBase *g,
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
        auto Nu = g->get_out_nodes_id(u.id);       // DB read: u's neighbours
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());   // label lookup

        // candidates: two hops out, v > u, deduplicated, ascending
        std::unordered_set<node_id_t> fof;
        for (node_id_t w : Nu)
            for (node_id_t v : g->get_out_nodes_id(w))   // DB read: w's neighbours
                if (v > u.id) fof.insert(v);
        std::vector<node_id_t> vs(fof.begin(), fof.end());
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

// ---- run_direct_fof_structural_cursor
//   --impl=direct --candidates=fof --variant=cursor
//   Threaded. Atomic block stealing over 64-node blocks of u; one TopK heap per
//   thread; merge at the end. --threads=1 runs the same code with one thread.
static RunResult run_direct_fof_structural_cursor(GraphBase *g, const Options &o)
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
                structural_worker_fof(myg, start, end, o, top, pairs, edges);
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

// ---------------------------------------------------------------- structural 2-hop, wedge counting (--variant=wedge)
// Same candidates, pairs and scores as structural_worker_fof, without reading
// N(v) per candidate. For each u the generator walks every wedge u → w → v; the
// number of distinct w that reach v IS |N(u) ∩ N(v)| when edges are stored in
// both directions (DIRECTED: 0), so CN is counted on the way:
//   cnt[v] += 1 for every w in N(u) with v in N(w)
// Jaccard also needs |N(v)|: one degree array, filled from a node-cursor pass
// at the start of the run (node table only, no adjacency reads; counted in
// time_ms).
//
// DB reads per u: N(u) and N(w) for each w in N(u) (the generator's reads).
// The per-candidate N(v) lookup of the plain FoF path is gone (s20: ~16.8M
// list reads instead of ~1e9).
//
// with_hop1 (internal, default false):
//   false  pure fof candidates (v reached by u -> w -> v), cnt[v] = CN.
//          Called by run_direct_fof_structural_wedge (public fof path).
//   true   khop2 candidates (also direct neighbours w of u, which may share
//          nothing with u). cnt[v] holds CN + 1 so a hop-1 node with CN = 0
//          is still marked as touched. Called by
//          run_direct_khop_structural_wedge in linkpred_khop.h.
static void structural_worker_fof_wedge(GraphBase *g,
                                        node_id_t start, node_id_t end,
                                        const Options &o,
                                        const std::vector<degree_t> &deg,
                                        std::vector<uint32_t> &cnt,
                                        std::vector<node_id_t> &touched,
                                        linkpred::TopK &top,
                                        size_t &pairs_scored,
                                        size_t &true_edges,
                                        bool with_hop1 = false)
{
    const bool cn = (o.method == "cn");
    const int64_t off = with_hop1 ? 1 : 0;
    NodeCursor *a = g->get_node_iter();            // outer: u
    a->set_key_range({start, end});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);       // DB read: u's neighbours
        std::sort(Nu.begin(), Nu.end());

        touched.clear();
        for (node_id_t w : Nu) {
            if (with_hop1 && w > u.id && cnt[w] == 0) { cnt[w] = 1; touched.push_back(w); }
            for (node_id_t v : g->get_out_nodes_id(w)) { // DB read: w's neighbours
                if (v <= u.id) continue;
                if (cnt[v] == 0) { cnt[v] = (uint32_t)off; touched.push_back(v); }
                cnt[v]++;
            }
        }
        std::sort(touched.begin(), touched.end());       // ascending v, as plain FoF

        for (node_id_t v : touched) {
            int64_t inter = (int64_t)cnt[v] - off;
            cnt[v] = 0;                                  // reset for the next u
            int label = std::binary_search(Nu.begin(), Nu.end(), v) ? 1 : 0;
            double s;
            if (cn) {
                s = (double)inter;
            } else {
                int64_t uni = (int64_t)Nu.size() + (int64_t)deg[v] - inter;
                s = uni > 0 ? (double)inter / (double)uni : 0.0;
            }
            top.offer({u.id, v, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
        }
        a->next(&u);
    }

    a->close();
    delete a;
}

// ---- run_direct_fof_structural_wedge
//   --impl=direct --candidates=fof --variant=wedge
//   Threaded. Same shape as run_direct_fof_structural_cursor, but scoring is
//   done by the wedge-counting worker above. Preloads degree[] once (node
//   table only; Jaccard needs |N(v)|).
//
//   with_hop1 is an INTERNAL parameter (default false). It exists only so
//   run_direct_khop_structural_wedge in linkpred_khop.h can call this
//   function with with_hop1=true to get the khop2 candidate set without
//   duplicating the wedge traversal. It is not part of the CLI surface and
//   is never set from fg_linkpred_bench_db.cpp's dispatch.
static RunResult run_direct_fof_structural_wedge(GraphBase *g, const Options &o,
                                                     bool with_hop1 = false)
{
    RunResult r;
    node_id_t max_id = g->get_max_node_id();
    if (max_id == OutOfBand_ID_MAX || max_id == 0) {
        fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
                (unsigned long long)max_id);
        return r;
    }

    // Degrees for Jaccard's |N(v)| (node table only).
    std::vector<degree_t> deg((size_t)max_id + 1, 0);
    {
        NodeCursor *nc = g->get_node_iter();
        nc->set_key_range({0, OutOfBand_ID_MAX});
        node n;
        nc->next(&n);
        while (n.id != OutOfBand_ID_MAX) {
            deg[n.id] = n.out_degree;
            nc->next(&n);
        }
        nc->close();
        delete nc;
    }

    const size_t BLOCK = 64;
    size_t num_blocks = (size_t)(max_id / BLOCK) + 1;

    std::vector<linkpred::TopK> per_topk;
    std::vector<size_t> per_pairs(o.threads, 0);
    std::vector<size_t> per_edges(o.threads, 0);
    per_topk.reserve(o.threads);
    for (int t = 0; t < o.threads; ++t)
        per_topk.emplace_back((size_t)o.topk);

    std::atomic<size_t> next_block{0};

    #pragma omp parallel num_threads(o.threads)
    {
        int tid = omp_get_thread_num();
        GraphBase *myg = g_thread_handles[tid];
        bool shared = (o.accum == "shared");
        linkpred::TopK my_top((size_t)o.topk);
        size_t my_pairs = 0, my_edges = 0;
        linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
        size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
        size_t         &edges = shared ? per_edges[tid] : my_edges;
        std::vector<uint32_t>  cnt((size_t)max_id + 1, 0);   // per thread, all zero between u's
        std::vector<node_id_t> touched;
        if (myg) {
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < num_blocks) {
                node_id_t start = (node_id_t)(blk * BLOCK);
                node_id_t end   = start + BLOCK - 1;
                if (end > max_id) end = max_id;
                structural_worker_fof_wedge(myg, start, end, o, deg, cnt, touched,
                                            top, pairs, edges, with_hop1);
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

// ---------------------------------------------------------------- embedding 2-hop (cosine / dot)
// Same candidates as the structural fof path; only the scoring fetch changes:
// v's embedding (get_node_properties) instead of v's neighbour list. One random
// DB read per candidate, like the structural lookup; there is no counted
// shortcut because cosine needs v's vector. --candidates=fof --method=cosine|dot.
//
// Feeds run_direct_fof_embedding_cursor.
static void embedding_worker_fof(GraphBase *g,
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

        // candidates: same walk as structural_worker_fof
        std::unordered_set<node_id_t> fof;
        for (node_id_t w : Nu)
            for (node_id_t v : g->get_out_nodes_id(w))   // DB read: w's neighbours
                if (v > u.id) fof.insert(v);
        std::vector<node_id_t> vs(fof.begin(), fof.end());
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

// ---- run_direct_fof_embedding_cursor
//   --impl=direct --candidates=fof --method=cosine|dot --variant=cursor
//   Threaded. Same block-stealing shape as the structural path; the only
//   difference is the worker reads v's embedding per candidate instead of
//   reading N(v) and intersecting.
static RunResult run_direct_fof_embedding_cursor(GraphBase *g, const Options &o)
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
                embedding_worker_fof(myg, start, end, o, top, pairs, edges);
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