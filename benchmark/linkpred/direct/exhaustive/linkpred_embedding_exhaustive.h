// linkpred_embedding_exhaustive.h -- embedding family (split from linkpred_exhaustive.h).
// Split by family from linkpred_exhaustive.h (2026-11-16); function bodies unchanged.
//
// linkpred_exhaustive.h — candidate strategy: EXHAUSTIVE (brute force).
// Scores every pair (u, v) with u < v, i.e. all pairs in the graph, each once.
//
// One file per candidate strategy. This one holds every method that uses
// exhaustive candidates:
//   structural (cn / jaccard)          structural_worker_cursor + run_direct_exhaustive_structural_cursor
//   structural, preloaded CSR          preload_csr + run_direct_exhaustive_structural_csr (--variant=csr)
//   structural or embedding, tiled     tiled_worker + run_direct_exhaustive_tiled (--variant=tiled, --tile=N)
//   embedding, per-pair DB reads       embedding_worker_cursor + run_direct_exhaustive_embedding_cursor
//   embedding, preloaded (pvector)     preload_embeddings + run_direct_exhaustive_embedding_pvector
// Other strategies: linkpred_fof.h, linkpred_khop.h (hand-written, --impl=direct).
// Single-threaded references for these live in linkpred_reference.h.

#pragma once

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <omp.h>
#include "common_defs.h"
#include "graph.h"
#include "pvector.h"

#include "linkpred_options.h"
#include "linkpred_scores.h"
#include "linkpred_topk.h"
#include "linkpred_types.h"

#include "linkpred_thread_handles.h"   // g_thread_handles (one RO handle per thread)
#include "linkpred_exhaustive_cursor.h"  // run_direct_exhaustive_cursor (shared cursor driver)
#include "linkpred_exhaustive_tiled.h"   // run_direct_exhaustive_tiled<Worker>, TiledU



// ---------------------------------------------------------------- embedding cursor
// Same shape as structural, but reads embeddings per pair from DB.
static void embedding_worker_cursor(GraphBase *g,
                             size_t block_lo, size_t block_hi,
                             size_t block_size, node_id_t max_id,
                             const Options &o,
                             linkpred::TopK &top,
                             size_t &pairs_scored,
                             size_t &true_edges)
{
    NodeCursor *a = g->get_node_iter();
    NodeCursor *b = g->get_node_iter();

    for (size_t blk = block_lo; blk < block_hi; ++blk) {
        node_id_t start = (node_id_t)(blk * block_size);
        node_id_t end   = start + block_size - 1;
        if (end > max_id) end = max_id;
        if (start > max_id) break;

        a->reset();
        a->set_key_range({start, end});
        node u;
        a->next(&u);

        while (u.id != OutOfBand_ID_MAX) {
            std::unordered_set<node_id_t> Nu_set;
            for (node_id_t n : g->get_out_nodes_id(u.id)) Nu_set.insert(n);
            prop_blob pu = g->get_node_properties(u.id);      // u's embedding, once per row

            b->reset();
            b->set_key_range({u.id + 1, OutOfBand_ID_MAX});
            node v;
            b->next(&v);

            while (v.id != OutOfBand_ID_MAX) {
                prop_blob pv = g->get_node_properties(v.id);  // v's embedding, PER PAIR (slow)

                int label = Nu_set.count(v.id) ? 1 : 0;
                double s = (o.method == "cosine")
                           ? linkpred::cosine_from_bytes(pu.data, pu.size,
                                                         pv.data, pv.size)
                           : linkpred::dot_from_bytes  (pu.data, pu.size,
                                                         pv.data, pv.size);
                top.offer({u.id, v.id, s, label});
                pairs_scored++;
                if (label == 1) true_edges++;

                delete[] const_cast<uint8_t*>(pv.data);       // free blob
                b->next(&v);
            }
            delete[] const_cast<uint8_t*>(pu.data);
            a->next(&u);
        }
    }

    a->close();
    delete a;
    b->close();
    delete b;
}

// ---- Previous hand-written driver, replaced by run_direct_exhaustive_cursor<...>
// (linkpred_exhaustive_cursor.h, 2026-11-16). Kept commented out for reference.
// static RunResult run_direct_exhaustive_embedding_cursor(GraphBase *g, const Options &o)
// {
//     RunResult r;
//     node_id_t max_id = g->get_max_node_id();
//     const size_t BLOCK = 64;
//     size_t num_blocks = (size_t)(max_id / BLOCK) + 1;
//
//     std::vector<linkpred::TopK> per_topk;
//     std::vector<size_t> per_pairs(o.threads, 0);
//     std::vector<size_t> per_edges(o.threads, 0);
//     per_topk.reserve(o.threads);
//     for (int t = 0; t < o.threads; ++t)
//         per_topk.emplace_back((size_t)o.topk);
//
//     std::atomic<size_t> next_block{0};
//
//     #pragma omp parallel num_threads(o.threads)
//     {
//         int tid = omp_get_thread_num();
//         GraphBase *myg = g_thread_handles[tid];
//         bool shared = (o.accum == "shared");           // --accum, see structural
//         linkpred::TopK my_top((size_t)o.topk);
//         size_t my_pairs = 0, my_edges = 0;
//         linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
//         size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
//         size_t         &edges = shared ? per_edges[tid] : my_edges;
//         if (myg) {
//             size_t blk;
//             while ((blk = next_block.fetch_add(1)) < num_blocks) {
//                 embedding_worker_cursor(myg, blk, blk + 1, BLOCK, max_id, o,
//                                  top, pairs, edges);
//             }
//         }
//         if (!shared) {
//             per_topk[tid]  = std::move(my_top);
//             per_pairs[tid] = my_pairs;
//             per_edges[tid] = my_edges;
//         }
//     }
//
//     for (size_t c : per_pairs) r.pairs_scored += c;
//     for (size_t c : per_edges) r.true_edges   += c;
//
//     r.top_k = linkpred::merge_topk(per_topk, (size_t)o.topk);
//     return r;
// }

static RunResult run_direct_exhaustive_embedding_cursor(GraphBase *g, const Options &o)
{
    return run_direct_exhaustive_cursor<embedding_worker_cursor>(g, o);   // driver: linkpred_exhaustive_cursor.h
}


// ---------------------------------------------------------------- pvector preload
// Read all embeddings into memory once, so the pvector hot loop never touches WT.
// Moved from main (B7); body unchanged. Fills nodes / all_emb, returns the dim.
static size_t preload_embeddings(GraphBase *g,
                                 std::vector<node_id_t> &nodes,
                                 pvector<std::vector<float>> &all_emb)
{
    size_t preload_dim = 0;
    node_id_t max_id = g->get_max_node_id();
    if (max_id != OutOfBand_ID_MAX) {
        all_emb.resize((size_t)max_id + 1);                 // one slot per node
        NodeCursor *nc = g->get_node_iter();                // serial pass
        nc->set_key_range({0, OutOfBand_ID_MAX});
        node u;
        nc->next(&u);
        while (u.id != OutOfBand_ID_MAX) {
            nodes.push_back(u.id);
            prop_blob pb = g->get_node_properties(u.id);
            if (preload_dim == 0) preload_dim = pb.size / sizeof(float);  // learn dim from first
            if (preload_dim > 0) {
                const float *raw = reinterpret_cast<const float*>(pb.data);
                all_emb[u.id].assign(raw, raw + preload_dim);
            }
            if (pb.data) delete[] const_cast<uint8_t*>(pb.data);
            nc->next(&u);
        }
        nc->close();
        delete nc;
        std::cerr << "[linkpred] preloaded " << nodes.size()
                  << " nodes × dim " << preload_dim << "\n";
    }
    return preload_dim;
}


// ---------------------------------------------------------------- pvector
// Single-threaded original: run_reference_embedding_pvector in linkpred_reference.h.
// NO g param — threads use g_thread_handles[tid] instead.
// A shared g here would corrupt WT session/cursor state.
static RunResult run_direct_exhaustive_embedding_pvector(
    const std::vector<node_id_t> &nodes,
    const pvector<std::vector<float>> &all_emb,
    const Options &o)
{
    RunResult r;

    std::vector<linkpred::TopK> per_topk;
    per_topk.reserve(o.threads);
    for (int t = 0; t < o.threads; ++t)
        per_topk.emplace_back((size_t)o.topk);

    std::vector<size_t> per_pairs(o.threads, 0);       // per-thread counters (see --accum)
    std::vector<size_t> per_edges(o.threads, 0);

    // num_threads(o.threads) is REQUIRED — without it OMP spawns max threads
    //   and g_thread_handles[tid] segfaults on out-of-range tid.
    #pragma omp parallel num_threads(o.threads)
    {
        int tid = omp_get_thread_num();
        GraphBase *myg = g_thread_handles[tid];        // private cursor set
        bool shared = (o.accum == "shared");           // --accum, see structural
        linkpred::TopK my_top((size_t)o.topk);
        size_t my_pairs = 0, my_edges = 0;
        linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
        size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
        size_t         &edges = shared ? per_edges[tid] : my_edges;

        // schedule(dynamic, 64): inner loop is triangular (row i has n-i iters);
        //   static would give thread 0 the heavy rows and thread T-1 the light.
        #pragma omp for schedule(dynamic, 64)
        for (size_t i = 0; i < nodes.size(); ++i) {
            node_id_t u = nodes[i];

            std::unordered_set<node_id_t> Nu_set;
            for (node_id_t n : myg->get_out_nodes_id(u)) Nu_set.insert(n);   // thread-local handle

            const std::vector<float> &eu = all_emb[u];     // read-only, safe
            for (size_t j = i + 1; j < nodes.size(); ++j) {
                node_id_t v = nodes[j];
                const std::vector<float> &ev = all_emb[v];
                int label = Nu_set.count(v) ? 1 : 0;
                double s = (o.method == "cosine")
                           ? linkpred::cosine(eu, ev)
                           : linkpred::dot(eu, ev);
                top.offer({u, v, s, label});
                pairs++;
                if (label == 1) edges++;
            }
        }

        if (!shared) {
            per_topk[tid]  = std::move(my_top);
            per_pairs[tid] = my_pairs;
            per_edges[tid] = my_edges;
        }
    }

    // Reduce after parallel region
    for (size_t c : per_pairs) r.pairs_scored += c;
    for (size_t c : per_edges) r.true_edges   += c;

    // Merge thread heaps
    r.top_k = linkpred::merge_topk(per_topk, (size_t)o.topk);
    return r;
}

// ---------------------------------------------------------------- tiled (--variant=tiled), embedding worker
// embedding half of the former tiled_worker (linkpred_structural_exhaustive.h):
// reads N(u) (labels) and u's vector for the tile once, then v's vector once
// per tile and scores it against every u < v in the tile.
// Driver: run_direct_exhaustive_tiled<...> in linkpred_exhaustive_tiled.h.
static void embedding_worker_tiled(GraphBase *g, NodeCursor *a, NodeCursor *b,
                                   node_id_t start, node_id_t end,
                                   const Options &o, std::vector<TiledU> &us,
                                   linkpred::TopK &top, size_t &pairs_scored,
                                   size_t &true_edges, size_t &v_reads)
{
    // ---- read the block's u's once ----
    us.clear();
    a->reset();
    a->set_key_range({start, end});
    node u;
    a->next(&u);
    while (u.id != OutOfBand_ID_MAX) {
        TiledU t;
        t.id = u.id;
        t.nbrs = g->get_out_nodes_id(u.id);
        std::sort(t.nbrs.begin(), t.nbrs.end());
        prop_blob pb = g->get_node_properties(u.id);
        const float *f = reinterpret_cast<const float *>(pb.data);
        if (pb.data) t.emb.assign(f, f + pb.size / sizeof(float));
        if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
        us.push_back(std::move(t));
        a->next(&u);
    }
    if (us.empty()) return;

    // ---- stream every v > first u once; score against u's below it ----
    b->reset();
    b->set_key_range({us.front().id + 1, OutOfBand_ID_MAX});
    node v;
    b->next(&v);
    while (v.id != OutOfBand_ID_MAX) {
        ++v_reads;
        prop_blob pv = g->get_node_properties(v.id);     // DB read: once per block
        const float *ev = reinterpret_cast<const float *>(pv.data);
        size_t dv = pv.data ? pv.size / sizeof(float) : 0;
        for (const TiledU &t : us) {
            if (t.id >= v.id) break;
            int label = std::binary_search(t.nbrs.begin(), t.nbrs.end(), v.id) ? 1 : 0;
            size_t dim = std::min(t.emb.size(), dv);
            double s = (dim == 0) ? 0.0
                     : (o.method == "cosine")
                       ? linkpred::cosine_from_ptrs(t.emb.data(), ev, dim)
                       : linkpred::dot_from_ptrs(t.emb.data(), ev, dim);
            top.offer({t.id, v.id, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
        }
        if (pv.data) delete[] const_cast<uint8_t *>(pv.data);
        b->next(&v);
    }
}

static RunResult run_direct_exhaustive_embedding_tiled(GraphBase *g, const Options &o)
{
    return run_direct_exhaustive_tiled<embedding_worker_tiled>(g, o);   // driver: linkpred_exhaustive_tiled.h
}
