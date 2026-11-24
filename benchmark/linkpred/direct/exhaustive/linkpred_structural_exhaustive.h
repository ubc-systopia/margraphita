// linkpred_structural_exhaustive.h -- structural family (split from linkpred_exhaustive.h).
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


// --variant=tiled: structural_worker_tiled here; shared driver in
// linkpred_exhaustive_tiled.h (embedding worker is in linkpred_embedding_exhaustive.h).

// ---------------------------------------------------------------- structural cursor
// One thread's structural work over a block of node IDs.
static void structural_worker_cursor(GraphBase *g,
                              size_t block_lo, size_t block_hi,
                              size_t block_size, node_id_t max_id,
                              const Options &o,
                              linkpred::TopK &top,
                              size_t &pairs_scored,
                              size_t &true_edges)
{
    NodeCursor *a = g->get_node_iter();            // outer: u
    NodeCursor *b = g->get_node_iter();            // inner: v > u

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
            auto Nu = g->get_out_nodes_id(u.id);       // DB read: u's neighbors
            std::sort(Nu.begin(), Nu.end());
            std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());   // O(1) label lookup

            b->reset();
            b->set_key_range({u.id + 1, OutOfBand_ID_MAX});   // v > u, each pair once
            node v;
            b->next(&v);

            while (v.id != OutOfBand_ID_MAX) {
                auto Nv = g->get_out_nodes_id(v.id);          // DB read: v's neighbors
                std::sort(Nv.begin(), Nv.end());

                int label = Nu_set.count(v.id) ? 1 : 0;       // ground truth edge?
                double s = linkpred::score_pair(o.method, Nu, Nv);
                top.offer({u.id, v.id, s, label});            // bounded heap
                pairs_scored++;
                if (label == 1) true_edges++;

                b->next(&v);
            }
            a->next(&u);
        }
    }

    a->close();
    delete a;
    b->close();
    delete b;
}
// ---- COMMENTED OUT (2026-11-16): per-thread-cursor experiment (E1p).
// Not wired: no bench dispatch or impl_matrix row, and --cursor-scope now only
// reaches the pipeline path. Cost difference vs per-block cursors
// (structural_worker_cursor) is ~2 cursor opens per 64 nodes. Kept for reference.
// // ---------------------------------------------------------------- structural, per-thread cursors (E1p)
// // Same body as structural_worker_cursor, but cursors a and b are passed in.
// // The runner creates them once per thread (before the block loop) and closes
// // them at end of thread.
// //
// // Only difference from E1:
// //   E1  — cursors created inside structural_worker_cursor (per block)
// //   E1p — cursors created in the runner (per thread)
// //
// // Reached via --cursor-scope=per-thread on the inlined path.
// static void structural_worker_cursor_thread(
//         GraphBase *g, NodeCursor *a, NodeCursor *b,
//         size_t block_lo, size_t block_hi, size_t block_size,
//         node_id_t max_id, const Options &o,
//         linkpred::TopK &top, size_t &pairs_scored, size_t &true_edges)
// {
//     // no a->close(), no b->close() — caller owns them
//     for (size_t blk = block_lo; blk < block_hi; ++blk) {
//         node_id_t start = (node_id_t)(blk * block_size);
//         node_id_t end   = start + block_size - 1;
//         if (end > max_id) end = max_id;
//         if (start > max_id) break;
//
//         a->reset();
//         a->set_key_range({start, end});
//         node u;
//         a->next(&u);
//
//         while (u.id != OutOfBand_ID_MAX) {
//             auto Nu = g->get_out_nodes_id(u.id);
//             std::sort(Nu.begin(), Nu.end());
//             std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());
//
//             b->reset();
//             b->set_key_range({u.id + 1, OutOfBand_ID_MAX});
//             node v;
//             b->next(&v);
//
//             while (v.id != OutOfBand_ID_MAX) {
//                 auto Nv = g->get_out_nodes_id(v.id);
//                 std::sort(Nv.begin(), Nv.end());
//
//                 int label = Nu_set.count(v.id) ? 1 : 0;
//                 double s = linkpred::score_pair(o.method, Nu, Nv);
//                 top.offer({u.id, v.id, s, label});
//                 pairs_scored++;
//                 if (label == 1) true_edges++;
//
//                 b->next(&v);
//             }
//             a->next(&u);
//         }
//     }
// }
//
// static RunResult run_direct_exhaustive_structural_threadcursor(GraphBase *g, const Options &o)
// {
//     RunResult r;
//     node_id_t max_id = g->get_max_node_id();
//     if (max_id == OutOfBand_ID_MAX || max_id == 0) {
//         fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
//                 (unsigned long long)max_id);
//         return r;
//     }
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
//         bool shared = (o.accum == "shared");
//         linkpred::TopK my_top((size_t)o.topk);
//         size_t my_pairs = 0, my_edges = 0;
//         linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
//         size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
//         size_t         &edges = shared ? per_edges[tid] : my_edges;
//         if (myg) {
//             NodeCursor *a = myg->get_node_iter();      // <-- per THREAD
//             NodeCursor *b = myg->get_node_iter();      // <-- per THREAD
//             size_t blk;
//             while ((blk = next_block.fetch_add(1)) < num_blocks) {
//                 structural_worker_cursor_thread(myg, a, b, blk, blk + 1,
//                                                 BLOCK, max_id, o,
//                                                 top, pairs, edges);
//             }
//             a->close();
//             b->close();
//             delete a;
//             delete b;
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
//     r.top_k = linkpred::merge_topk(per_topk, (size_t)o.topk);
//     return r;
// }
// ---- Previous hand-written driver, replaced by run_direct_exhaustive_cursor<...>
// (linkpred_exhaustive_cursor.h, 2026-11-16). Kept commented out for reference.
// static RunResult run_direct_exhaustive_structural_cursor(GraphBase *g, const Options &o)
// {
//     RunResult r;
//     node_id_t max_id = g->get_max_node_id();
//
//     // Guard: metadata missing → max_id = UINT64_MAX → num_blocks ≈ 2.88e17
//     if (max_id == OutOfBand_ID_MAX || max_id == 0) {
//         fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
//                 (unsigned long long)max_id);
//         return r;
//     }
//     const size_t BLOCK = 64;
//     size_t num_blocks = (size_t)(max_id / BLOCK) + 1;
//
//     // ---- Atomic block-stealing (one thread when --threads=1) ----
//     std::vector<linkpred::TopK> per_topk;
//     std::vector<size_t> per_pairs(o.threads, 0);
//     std::vector<size_t> per_edges(o.threads, 0);
//     per_topk.reserve(o.threads);
//     for (int t = 0; t < o.threads; ++t)
//         per_topk.emplace_back((size_t)o.topk);
//
//     std::atomic<size_t> next_block{0};                 // work-stealing counter
//
//     #pragma omp parallel num_threads(o.threads)
//     {
//         int tid = omp_get_thread_num();
//         GraphBase *myg = g_thread_handles[tid];        // private cursor set
//         // --accum=local:  thread-local heap + counters, published once at the end.
//         //                 Neighbouring per_* slots share cache lines, so updating
//         //                 them per pair makes threads slow each other down.
//         // --accum=shared: the original version — update per_*[tid] per pair.
//         bool shared = (o.accum == "shared");
//         linkpred::TopK my_top((size_t)o.topk);
//         size_t my_pairs = 0, my_edges = 0;
//         linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
//         size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
//         size_t         &edges = shared ? per_edges[tid] : my_edges;
//         if (myg) {
//             size_t blk;
//             while ((blk = next_block.fetch_add(1)) < num_blocks) {   // grab next block
//                 structural_worker_cursor(myg, blk, blk + 1, BLOCK, max_id, o,
//                                   top, pairs, edges);
//             }
//         }
//         if (!shared) {
//             per_topk[tid]  = std::move(my_top);
//             per_pairs[tid] = my_pairs;
//             per_edges[tid] = my_edges;
//         }
//     }
//
//     // Reduce per-thread counters (after parallel region)
//     for (size_t c : per_pairs) r.pairs_scored += c;
//     for (size_t c : per_edges) r.true_edges   += c;
//
//     // Merge per-thread heaps → global top-k
//     r.top_k = linkpred::merge_topk(per_topk, (size_t)o.topk);
//     return r;
// }

static RunResult run_direct_exhaustive_structural_cursor(GraphBase *g, const Options &o)
{
    return run_direct_exhaustive_cursor<structural_worker_cursor>(g, o);   // driver: linkpred_exhaustive_cursor.h
}


// ---------------------------------------------------------------- structural csr (--variant=csr)
// In-memory ceiling for the structural path: the linkpred equivalent of
// benchmark/microbenchmarks/csr_seek_scan.cpp, and the structural partner of
// pvector. Adjacency is read from the DB once (preload_csr, counted in load_ms),
// then the same block-stealing loop scores every pair u < v with zero DB calls.
//
// Layout: nodes[i] = i-th node ID in DB order; node i's sorted neighbours are
// edges[offsets[i] .. offsets[i+1]). Blocks are 64 node *indices* (the cursor
// path uses 64-ID ranges); the pair set is the same, so pairs_scored,
// true_edges and the top-k match --variant=cursor exactly.
struct LinkpredCsr {
    std::vector<node_id_t> nodes;
    std::vector<size_t>    offsets;   // nodes.size() + 1 entries
    std::vector<node_id_t> edges;
};

static void preload_csr(GraphBase *g, LinkpredCsr &csr, bool both = false)   // both: out U in (--direction=both, -d)
{
    NodeCursor *nc = g->get_node_iter();
    nc->set_key_range({0, OutOfBand_ID_MAX});
    node n;
    nc->next(&n);
    csr.offsets.push_back(0);
    while (n.id != OutOfBand_ID_MAX) {
        auto Nn = lp_nbrs(g, n.id, both);
        std::sort(Nn.begin(), Nn.end());
        csr.nodes.push_back(n.id);
        csr.edges.insert(csr.edges.end(), Nn.begin(), Nn.end());
        csr.offsets.push_back(csr.edges.size());
        nc->next(&n);
    }
    nc->close();
    delete nc;
    std::cerr << "[csr] preloaded nodes=" << csr.nodes.size()
              << " adj_entries=" << csr.edges.size() << "\n";
}

static void structural_worker_csr(const LinkpredCsr &csr,
                                  size_t i_lo, size_t i_hi,
                                  const Options &o,
                                  linkpred::TopK &top,
                                  size_t &pairs_scored,
                                  size_t &true_edges)
{
    const size_t n = csr.nodes.size();
    for (size_t i = i_lo; i < i_hi && i < n; ++i) {
        node_id_t u = csr.nodes[i];
        const node_id_t *Nu = csr.edges.data() + csr.offsets[i];
        size_t nu = csr.offsets[i + 1] - csr.offsets[i];

        for (size_t j = i + 1; j < n; ++j) {
            node_id_t v = csr.nodes[j];
            const node_id_t *Nv = csr.edges.data() + csr.offsets[j];
            size_t nv = csr.offsets[j + 1] - csr.offsets[j];

            int label = std::binary_search(Nu, Nu + nu, v) ? 1 : 0;
            double s = linkpred::score_pair(o.method, Nu, nu, Nv, nv);
            top.offer({u, v, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
        }
    }
}

static RunResult run_direct_exhaustive_structural_csr(const LinkpredCsr &csr, const Options &o)
{
    RunResult r;
    const size_t BLOCK = 64;
    size_t num_blocks = (csr.nodes.size() + BLOCK - 1) / BLOCK;

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
        bool shared = (o.accum == "shared");
        linkpred::TopK my_top((size_t)o.topk);
        size_t my_pairs = 0, my_edges = 0;
        linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
        size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
        size_t         &edges = shared ? per_edges[tid] : my_edges;
        size_t blk;
        while ((blk = next_block.fetch_add(1)) < num_blocks)
            structural_worker_csr(csr, blk * BLOCK, (blk + 1) * BLOCK, o,
                                  top, pairs, edges);
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


// ---- COMMENTED OUT (2026-11-16): combined tiled_worker + run_direct_exhaustive_tiled,
// replaced by structural_worker_tiled (below), embedding_worker_tiled
// (linkpred_embedding_exhaustive.h) and the run_direct_exhaustive_tiled<Worker>
// driver (linkpred_exhaustive_tiled.h). Kept for reference.
// // ---------------------------------------------------------------- tiled (--variant=tiled)
// // Same pairs as the cursor paths (every u < v, straight from the DB, no full
// // preload), but each v is read ONCE PER BLOCK instead of once per pair.
// //
// // The cursor paths read v's adjacency / embedding for every (u, v): each stored
// // list is read ~n/2 times (s12: 8.4M reads of 4,091 lists). Here a thread takes
// // a block of up to --tile node IDs (default 64), reads those u's once, then
// // streams every v > (first u) once and scores it against all u's in the block
// // with u < v. DB reads drop to ~n / tile per list; --tile = n is "read once",
// // i.e. csr / pvector without the separate preload.
// //
// // Structural (cn / jaccard): reads N(u), N(v) via get_out_nodes_id.
// // Embedding (cosine / dot):  reads the u / v blobs via get_node_properties and
// //                            N(u) for the label, as the cursor path does.
// // Results (pairs, true edges, top-k) match --variant=cursor exactly.
// struct TiledU {
//     node_id_t id;
//     std::vector<node_id_t> nbrs;   // sorted; also used for the label
//     std::vector<float> emb;        // embedding methods only
// };
//
// static void tiled_worker(GraphBase *g, NodeCursor *a, NodeCursor *b,
//                          node_id_t start, node_id_t end, bool structural,
//                          const Options &o, std::vector<TiledU> &us,
//                          linkpred::TopK &top, size_t &pairs_scored,
//                          size_t &true_edges, size_t &v_reads)
// {
//     // ---- read the block's u's once ----
//     us.clear();
//     a->reset();
//     a->set_key_range({start, end});
//     node u;
//     a->next(&u);
//     while (u.id != OutOfBand_ID_MAX) {
//         TiledU t;
//         t.id = u.id;
//         t.nbrs = g->get_out_nodes_id(u.id);
//         std::sort(t.nbrs.begin(), t.nbrs.end());
//         if (!structural) {
//             prop_blob pb = g->get_node_properties(u.id);
//             const float *f = reinterpret_cast<const float *>(pb.data);
//             if (pb.data) t.emb.assign(f, f + pb.size / sizeof(float));
//             if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
//         }
//         us.push_back(std::move(t));
//         a->next(&u);
//     }
//     if (us.empty()) return;
//
//     // ---- stream every v > first u once; score against u's below it ----
//     b->reset();
//     b->set_key_range({us.front().id + 1, OutOfBand_ID_MAX});
//     node v;
//     b->next(&v);
//     while (v.id != OutOfBand_ID_MAX) {
//         ++v_reads;
//         if (structural) {
//             auto Nv = g->get_out_nodes_id(v.id);         // DB read: once per block
//             std::sort(Nv.begin(), Nv.end());
//             for (const TiledU &t : us) {
//                 if (t.id >= v.id) break;                 // us is in ID order
//                 int label = std::binary_search(t.nbrs.begin(), t.nbrs.end(), v.id) ? 1 : 0;
//                 double s = linkpred::score_pair(o.method, t.nbrs, Nv);
//                 top.offer({t.id, v.id, s, label});
//                 pairs_scored++;
//                 if (label == 1) true_edges++;
//             }
//         } else {
//             prop_blob pv = g->get_node_properties(v.id); // DB read: once per block
//             const float *ev = reinterpret_cast<const float *>(pv.data);
//             size_t dv = pv.data ? pv.size / sizeof(float) : 0;
//             for (const TiledU &t : us) {
//                 if (t.id >= v.id) break;
//                 int label = std::binary_search(t.nbrs.begin(), t.nbrs.end(), v.id) ? 1 : 0;
//                 size_t dim = std::min(t.emb.size(), dv);
//                 double s = (dim == 0) ? 0.0
//                          : (o.method == "cosine")
//                            ? linkpred::cosine_from_ptrs(t.emb.data(), ev, dim)
//                            : linkpred::dot_from_ptrs(t.emb.data(), ev, dim);
//                 top.offer({t.id, v.id, s, label});
//                 pairs_scored++;
//                 if (label == 1) true_edges++;
//             }
//             if (pv.data) delete[] const_cast<uint8_t *>(pv.data);
//         }
//         b->next(&v);
//     }
// }
//
// static RunResult run_direct_exhaustive_tiled(GraphBase *g, const Options &o)
// {
//     RunResult r;
//     node_id_t max_id = g->get_max_node_id();
//     if (max_id == OutOfBand_ID_MAX || max_id == 0) {
//         fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
//                 (unsigned long long)max_id);
//         return r;
//     }
//     const size_t BLOCK = (size_t)o.tile;
//     size_t num_blocks = (size_t)(max_id / BLOCK) + 1;
//     const bool structural = method_is_structural(o);
//
//     std::vector<linkpred::TopK> per_topk;
//     std::vector<size_t> per_pairs(o.threads, 0);
//     std::vector<size_t> per_edges(o.threads, 0);
//     per_topk.reserve(o.threads);
//     for (int t = 0; t < o.threads; ++t)
//         per_topk.emplace_back((size_t)o.topk);
//
//     std::atomic<size_t> next_block{0};
//     std::atomic<size_t> total_v_reads{0};
//
//     #pragma omp parallel num_threads(o.threads)
//     {
//         int tid = omp_get_thread_num();
//         GraphBase *myg = g_thread_handles[tid];
//         bool shared = (o.accum == "shared");
//         linkpred::TopK my_top((size_t)o.topk);
//         size_t my_pairs = 0, my_edges = 0, my_reads = 0;
//         linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
//         size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
//         size_t         &edges = shared ? per_edges[tid] : my_edges;
//         if (myg) {
//             NodeCursor *a = myg->get_node_iter();    // per thread
//             NodeCursor *b = myg->get_node_iter();
//             std::vector<TiledU> us;
//             us.reserve(BLOCK);
//             size_t blk;
//             while ((blk = next_block.fetch_add(1)) < num_blocks) {
//                 node_id_t start = (node_id_t)(blk * BLOCK);
//                 node_id_t end   = start + (node_id_t)BLOCK - 1;
//                 if (end > max_id) end = max_id;
//                 tiled_worker(myg, a, b, start, end, structural, o, us,
//                              top, pairs, edges, my_reads);
//             }
//             a->close();
//             b->close();
//             delete a;
//             delete b;
//         }
//         total_v_reads += my_reads;
//         if (!shared) {
//             per_topk[tid]  = std::move(my_top);
//             per_pairs[tid] = my_pairs;
//             per_edges[tid] = my_edges;
//         }
//     }
//
//     for (size_t c : per_pairs) r.pairs_scored += c;
//     for (size_t c : per_edges) r.true_edges   += c;
//     r.top_k = linkpred::merge_topk(per_topk, (size_t)o.topk);
//     std::cerr << "[tiled] tile=" << BLOCK << " v_reads=" << total_v_reads.load()
//               << " pairs=" << r.pairs_scored
//               << " (cursor paths read v once per pair)\n";
//     return r;
// }

// ---------------------------------------------------------------- tiled (--variant=tiled), structural worker
// structural half of the former tiled_worker: reads N(u) for the tile once,
// then N(v) once per tile and scores it against every u < v in the tile.
// Driver: run_direct_exhaustive_tiled<...> in linkpred_exhaustive_tiled.h.
static void structural_worker_tiled(GraphBase *g, NodeCursor *a, NodeCursor *b,
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
        auto Nv = g->get_out_nodes_id(v.id);             // DB read: once per block
        std::sort(Nv.begin(), Nv.end());
        for (const TiledU &t : us) {
            if (t.id >= v.id) break;                     // us is in ID order
            int label = std::binary_search(t.nbrs.begin(), t.nbrs.end(), v.id) ? 1 : 0;
            double s = linkpred::score_pair(o.method, t.nbrs, Nv);
            top.offer({t.id, v.id, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
        }
        b->next(&v);
    }
}

static RunResult run_direct_exhaustive_structural_tiled(GraphBase *g, const Options &o)
{
    return run_direct_exhaustive_tiled<structural_worker_tiled>(g, o);   // driver: linkpred_exhaustive_tiled.h
}
