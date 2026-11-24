// linkpred_pipeline.h — threaded link prediction with PLUGGABLE candidate generation.
//
// Same loop as linkpred_exhaustive.h, but "which v's for this u" comes from a
// per-u generator in linkpred_candidates.h (ExhaustiveCandidates,
// FofCandidates, KHopCandidates) instead of being written into the loop.
//
//   threads take blocks of u            (block stealing, like exhaustive)
//     per thread: own handle, own generator, own top-k + counters
//       per u: load N(u), labels, [embedding if applicable]
//         cands.for_each(u, v → score, label, offer)
//   sum counters, merge_topk            → RunResult
//
// The generator is a template parameter, so the call is inlined — no virtual
// call per pair. With ExhaustiveCandidates the result must equal
// run_direct_exhaustive_structural_cursor (linkpred_exhaustive.h): that is the check.
//
// Currently: structural (cn / jaccard) and embedding (cosine / dot, both
// cursor and pvector variations). Wired into fg_linkpred_bench_db.cpp
// (--candidates): structural only; the embedding runners are not called yet.
//
// Note (pvector): v's come from the generator, e.g. ExhaustiveCandidates walks a
// DB node cursor, so this path reads the DB in the hot loop — same results as
// run_direct_exhaustive_embedding_pvector, but timings are not comparable.

#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <unordered_set>
#include <vector>

#include <omp.h>
#include "common_defs.h"
#include "graph.h"
#include "pvector.h"               // pvector<std::vector<float>>

#include "linkpred_candidates.h"
#include "linkpred_thread_handles.h"   // g_thread_handles
#include "linkpred_options.h"
#include "linkpred_scores.h"
#include "linkpred_topk.h"
#include "linkpred_types.h"

// ===============================================================
// Per-u block functions
// ===============================================================

// ---------------------------------------------------------------- structural
template <class Cands>
void pipeline_structural_block(GraphBase *g, NodeCursor *a, Cands &cands,
                               node_id_t start, node_id_t end,
                               const Options &o,
                               linkpred::TopK &top,
                               size_t &pairs_scored,
                               size_t &true_edges)
{
    a->reset();
    a->set_key_range({start, end});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        auto Nu = g->get_out_nodes_id(u.id);           // DB read: u's neighbours
        std::sort(Nu.begin(), Nu.end());
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());   // label lookup

        cands.for_each(u.id, [&](node_id_t v) {        // candidates for u (pluggable)
            auto Nv = g->get_out_nodes_id(v);          // DB read: v's neighbours
            std::sort(Nv.begin(), Nv.end());

            int label = Nu_set.count(v) ? 1 : 0;       // ground truth edge?
            double s = linkpred::score_pair(o.method, Nu, Nv);
            top.offer({u.id, v, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
            return true;
        });
        a->next(&u);
    }
}

// ---------------------------------------------------------------- embedding, cursor
// Mirrors embedding_worker_cursor in linkpred_exhaustive.h: embeddings read
// from DB per pair. Each pv blob is freed after scoring; pu per u.
template <class Cands>
void pipeline_embedding_cursor_block(GraphBase *g, NodeCursor *a, Cands &cands,
                                     node_id_t start, node_id_t end,
                                     const Options &o,
                                     linkpred::TopK &top,
                                     size_t &pairs_scored,
                                     size_t &true_edges)
{
    a->reset();
    a->set_key_range({start, end});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        std::unordered_set<node_id_t> Nu_set;
        for (node_id_t n : g->get_out_nodes_id(u.id)) Nu_set.insert(n);
        prop_blob pu = g->get_node_properties(u.id);      // u's embedding, once per row

        cands.for_each(u.id, [&](node_id_t v) {
            prop_blob pv = g->get_node_properties(v);     // v's embedding, PER PAIR
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine")
                       ? linkpred::cosine_from_bytes(pu.data, pu.size,
                                                     pv.data, pv.size)
                       : linkpred::dot_from_bytes  (pu.data, pu.size,
                                                     pv.data, pv.size);
            top.offer({u.id, v, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
            delete[] const_cast<uint8_t*>(pv.data);       // free blob
            return true;
        });
        delete[] const_cast<uint8_t*>(pu.data);
        a->next(&u);
    }
}

// ---------------------------------------------------------------- embedding, pvector
// Mirrors the inner loop of run_direct_exhaustive_embedding_pvector: iterates
// a block of indices into `nodes`, uses all_emb[] for scoring.
//
// `nodes` must be sorted ascending (preload_embeddings iterates via cursor, so
// it is). cands.for_each(u, ...) emits v > u by node id, which for sorted
// `nodes` corresponds to "later entries in nodes" — the same pair set as the
// hand-coded nodes[j], j > i loop.
template <class Cands>
void pipeline_embedding_pvector_block(GraphBase *g, Cands &cands,
                                      const std::vector<node_id_t> &nodes,
                                      size_t i_lo, size_t i_hi,
                                      const pvector<std::vector<float>> &all_emb,
                                      const Options &o,
                                      linkpred::TopK &top,
                                      size_t &pairs_scored,
                                      size_t &true_edges)
{
    for (size_t i = i_lo; i < i_hi; ++i) {
        node_id_t u = nodes[i];

        std::unordered_set<node_id_t> Nu_set;
        for (node_id_t n : g->get_out_nodes_id(u)) Nu_set.insert(n);   // DB: label lookup
        const std::vector<float> &eu = all_emb[u];                     // read-only, safe

        cands.for_each(u, [&](node_id_t v) {
            const std::vector<float> &ev = all_emb[v];                 // read-only, safe
            int label = Nu_set.count(v) ? 1 : 0;
            double s = (o.method == "cosine")
                       ? linkpred::cosine(eu, ev)
                       : linkpred::dot(eu, ev);
            top.offer({u, v, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
            return true;
        });
    }
}

// ===============================================================
// Threaded runners
// ===============================================================

// ---------------------------------------------------------------- structural
template <class MakeCands>
RunResult run_pipeline_structural(GraphBase *g, const Options &o, MakeCands make_cands)
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
        if (myg && o.cursor_scope == "block") {
            // --cursor-scope=block: fresh generator + outer cursor per 64-node
            // block, like structural_worker_cursor in linkpred_exhaustive.h.
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < num_blocks) {
                node_id_t start = (node_id_t)(blk * BLOCK);
                node_id_t end   = start + BLOCK - 1;
                if (end > max_id) end = max_id;
                auto cands = make_cands(myg);          // opened for this block only
                NodeCursor *a = myg->get_node_iter();
                pipeline_structural_block(myg, a, cands, start, end, o,
                                          top, pairs, edges);
                a->close();
                delete a;
            }                                          // cands closes its cursor here
        }
        else if (myg) {
            // --cursor-scope=thread (default): one generator + outer cursor per
            // thread, reused for every block.
            auto cands = make_cands(myg);              // C++17: elided, no move
            NodeCursor *a = myg->get_node_iter();
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < num_blocks) {
                node_id_t start = (node_id_t)(blk * BLOCK);
                node_id_t end   = start + BLOCK - 1;
                if (end > max_id) end = max_id;
                pipeline_structural_block(myg, a, cands, start, end, o,
                                          top, pairs, edges);
            }
            a->close();
            delete a;
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

// ---------------------------------------------------------------- structural, contiguous
// A/B counterpart to run_pipeline_structural. Both call the exact same
// pipeline_structural_block, so scoring, top-k, labels, --accum split, and merge
// are byte-identical. Only the outer block assignment differs:
//
//   run_pipeline_structural — blocks grabbed on demand via atomic fetch_add. Load-balances
//                 automatically. Correct choice when per-u work is skewed
//                 (triangular exhaustive, hub-heavy khop).
//   _contiguous — each thread gets one fixed run of blocks up front. No atomic.
//                 Faster when per-u work is uniform; much slower when skewed
//                 because one thread gets all the heavy u's.
//
// Neither branches on partition strategy. The A/B is resolved by calling one or
// the other — see test_partition_compare.cpp.
template <class MakeCands>
RunResult run_pipeline_structural_contiguous(GraphBase *g, const Options &o, MakeCands make_cands)
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

    // No atomic. Pre-assign contiguous ranges. Last thread takes the remainder.
    size_t blocks_per = num_blocks / (size_t)o.threads;

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
        if (myg) {
            auto cands = make_cands(myg);
            NodeCursor *a = myg->get_node_iter();

            size_t blk_start = (size_t)tid * blocks_per;
            size_t blk_end   = (tid == o.threads - 1)
                                 ? num_blocks
                                 : blk_start + blocks_per;

            for (size_t blk = blk_start; blk < blk_end; ++blk) {
                node_id_t start = (node_id_t)(blk * BLOCK);
                node_id_t end   = start + BLOCK - 1;
                if (end > max_id) end = max_id;
                pipeline_structural_block(myg, a, cands, start, end, o,
                                          top, pairs, edges);
            }
            a->close();
            delete a;
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

// ---------------------------------------------------------------- embedding, cursor
// NOTE: identical shape to run_pipeline_structural; only the block fn differs.
template <class MakeCands>
RunResult run_pipeline_embedding_cursor(GraphBase *g, const Options &o, MakeCands make_cands)
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
        if (myg) {
            auto cands = make_cands(myg);
            NodeCursor *a = myg->get_node_iter();
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < num_blocks) {
                node_id_t start = (node_id_t)(blk * BLOCK);
                node_id_t end   = start + BLOCK - 1;
                if (end > max_id) end = max_id;
                pipeline_embedding_cursor_block(myg, a, cands, start, end, o,
                                                top, pairs, edges);
            }
            a->close();
            delete a;
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

// ---------------------------------------------------------------- embedding, pvector
// Different block unit: blocks over indices into `nodes`, not node IDs.
template <class MakeCands>
RunResult run_pipeline_embedding_pvector(
    GraphBase *g,
    const std::vector<node_id_t> &nodes,
    const pvector<std::vector<float>> &all_emb,
    const Options &o,
    MakeCands make_cands)
{
    RunResult r;
    if (nodes.empty()) return r;

    const size_t BLOCK = 64;
    size_t num_blocks = (nodes.size() + BLOCK - 1) / BLOCK;

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
        if (myg) {
            auto cands = make_cands(myg);
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < num_blocks) {
                size_t i_lo = blk * BLOCK;
                size_t i_hi = std::min(i_lo + BLOCK, nodes.size());
                pipeline_embedding_pvector_block(myg, cands, nodes, i_lo, i_hi,
                                                 all_emb, o, top, pairs, edges);
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

// ===============================================================
// Single-threaded versions
// ===============================================================
// Same per-u scoring loop, no threads / OMP / per-thread accumulators.
// Uses the main handle `g` directly — never touches g_thread_handles, so it
// runs even when no worker handles were opened.
//
// Two uses:
//   1) --reference --candidates={exhaustive,fof,khop}   single-threaded path
//      for any generator (linkpred_reference.h only has exhaustive).
//   2) Correctness baseline: *_single(g, o, make_cands) must match
//      *(g, o, make_cands) with --threads=1 on pairs_scored / true_edges /
//      every top-k row. Same code, same generator, only threading differs.

// ---------------------------------------------------------------- structural
template <class MakeCands>
RunResult run_pipeline_structural_single(GraphBase *g, const Options &o,
                                          MakeCands make_cands)
{
    RunResult r;
    node_id_t max_id = g->get_max_node_id();
    if (max_id == OutOfBand_ID_MAX || max_id == 0) {
        fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
                (unsigned long long)max_id);
        return r;
    }

    linkpred::TopK top((size_t)o.topk);
    size_t pairs = 0, edges = 0;

    auto cands = make_cands(g);
    NodeCursor *a = g->get_node_iter();
    pipeline_structural_block(g, a, cands, 0, max_id, o, top, pairs, edges);
    a->close();
    delete a;

    r.pairs_scored = pairs;
    r.true_edges   = edges;
    std::vector<linkpred::TopK> vt;
    vt.push_back(std::move(top));
    r.top_k = linkpred::merge_topk(vt, (size_t)o.topk);
    return r;
}

// ---------------------------------------------------------------- embedding, cursor
template <class MakeCands>
RunResult run_pipeline_embedding_cursor_single(GraphBase *g, const Options &o,
                                                MakeCands make_cands)
{
    RunResult r;
    node_id_t max_id = g->get_max_node_id();
    if (max_id == OutOfBand_ID_MAX || max_id == 0) {
        fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
                (unsigned long long)max_id);
        return r;
    }

    linkpred::TopK top((size_t)o.topk);
    size_t pairs = 0, edges = 0;

    auto cands = make_cands(g);
    NodeCursor *a = g->get_node_iter();
    pipeline_embedding_cursor_block(g, a, cands, 0, max_id, o, top, pairs, edges);
    a->close();
    delete a;

    r.pairs_scored = pairs;
    r.true_edges   = edges;
    std::vector<linkpred::TopK> vt;
    vt.push_back(std::move(top));
    r.top_k = linkpred::merge_topk(vt, (size_t)o.topk);
    return r;
}

// ---------------------------------------------------------------- embedding, pvector
template <class MakeCands>
RunResult run_pipeline_embedding_pvector_single(
    GraphBase *g,
    const std::vector<node_id_t> &nodes,
    const pvector<std::vector<float>> &all_emb,
    const Options &o,
    MakeCands make_cands)
{
    RunResult r;
    if (nodes.empty()) return r;

    linkpred::TopK top((size_t)o.topk);
    size_t pairs = 0, edges = 0;

    auto cands = make_cands(g);
    pipeline_embedding_pvector_block(g, cands, nodes, 0, nodes.size(),
                                      all_emb, o, top, pairs, edges);

    r.pairs_scored = pairs;
    r.true_edges   = edges;
    std::vector<linkpred::TopK> vt;
    vt.push_back(std::move(top));
    r.top_k = linkpred::merge_topk(vt, (size_t)o.topk);
    return r;
}


// ─────────────────────────────────────────────────────────────────────
// Per-candidate entry points. Each is a thin wrapper over the generic
// engine, so the dispatch in fg_linkpred_bench_db.cpp can call by name
// and the matrix function column names something real.
// ─────────────────────────────────────────────────────────────────────

// structural / cursor
inline RunResult run_pipeline_exhaustive_structural_cursor(GraphBase *g, const Options &o) {
    return run_pipeline_structural(g, o,
        [](GraphBase *h){ return linkpred::ExhaustiveCandidates(h); });
}
inline RunResult run_pipeline_fof_structural_cursor(GraphBase *g, const Options &o) {
    return run_pipeline_structural(g, o,
        [](GraphBase *h){ return linkpred::FofCandidates(h); });
}
inline RunResult run_pipeline_khop_structural_cursor(GraphBase *g, const Options &o) {
    return run_pipeline_structural(g, o,
        [&](GraphBase *h){ return linkpred::KHopCandidates(h, o.k_hops); });
}