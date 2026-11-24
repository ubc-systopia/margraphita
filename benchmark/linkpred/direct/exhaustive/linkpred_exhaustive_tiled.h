// linkpred_exhaustive_tiled.h -- shared driver for --candidates=exhaustive
// --variant=tiled (--tile=N), used by both families:
//   run_direct_exhaustive_structural_tiled -> run_direct_exhaustive_tiled<structural_worker_tiled>
//   run_direct_exhaustive_embedding_tiled  -> run_direct_exhaustive_tiled<embedding_worker_tiled>
//
// Previously one tiled_worker served both families and checked the method
// (structural vs embedding) at run time for every u and every v. Now each
// family has its own tile worker, fixed at compile time; the driver below is
// the old run_direct_exhaustive_tiled with only the worker call changed.
//
// ---------------------------------------------------------------- tiled (--variant=tiled)
// Same pairs as the cursor paths (every u < v, straight from the DB, no full
// preload), but each v is read ONCE PER BLOCK instead of once per pair.
//
// The cursor paths read v's adjacency / embedding for every (u, v): each stored
// list is read ~n/2 times (s12: 8.4M reads of 4,091 lists). Here a thread takes
// a block of up to --tile node IDs (default 64), reads those u's once, then
// streams every v > (first u) once and scores it against all u's in the block
// with u < v. DB reads drop to ~n / tile per list; --tile = n is "read once",
// i.e. csr / pvector without the separate preload.
//
// Structural (cn / jaccard): reads N(u), N(v) via get_out_nodes_id.
// Embedding (cosine / dot):  reads the u / v blobs via get_node_properties and
//                            N(u) for the label, as the cursor path does.
// Results (pairs, true edges, top-k) match --variant=cursor exactly.

#pragma once

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <iostream>
#include <vector>

#include <omp.h>
#include "common_defs.h"
#include "graph.h"

#include "linkpred_thread_handles.h"   // g_thread_handles
#include "linkpred_options.h"
#include "linkpred_topk.h"
#include "linkpred_types.h"

// One u of the current tile, read once per tile.
struct TiledU {
    node_id_t id;
    std::vector<node_id_t> nbrs;   // sorted; also used for the label
    std::vector<float> emb;        // embedding methods only
};

// Worker: void(GraphBase *g, NodeCursor *a, NodeCursor *b, node_id_t start,
//             node_id_t end, const Options &o, std::vector<TiledU> &us,
//             linkpred::TopK &top, size_t &pairs_scored, size_t &true_edges,
//             size_t &v_reads)
template <auto Worker>
static RunResult run_direct_exhaustive_tiled(GraphBase *g, const Options &o)
{
    RunResult r;
    node_id_t max_id = g->get_max_node_id();
    if (max_id == OutOfBand_ID_MAX || max_id == 0) {
        fprintf(stderr, "ERROR: max_node_id=%llu (metadata missing?)\n",
                (unsigned long long)max_id);
        return r;
    }
    const size_t BLOCK = (size_t)o.tile;
    size_t num_blocks = (size_t)(max_id / BLOCK) + 1;

    std::vector<linkpred::TopK> per_topk;
    std::vector<size_t> per_pairs(o.threads, 0);
    std::vector<size_t> per_edges(o.threads, 0);
    per_topk.reserve(o.threads);
    for (int t = 0; t < o.threads; ++t)
        per_topk.emplace_back((size_t)o.topk);

    std::atomic<size_t> next_block{0};
    std::atomic<size_t> total_v_reads{0};

    #pragma omp parallel num_threads(o.threads)
    {
        int tid = omp_get_thread_num();
        GraphBase *myg = g_thread_handles[tid];
        bool shared = (o.accum == "shared");
        linkpred::TopK my_top((size_t)o.topk);
        size_t my_pairs = 0, my_edges = 0, my_reads = 0;
        linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
        size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
        size_t         &edges = shared ? per_edges[tid] : my_edges;
        if (myg) {
            NodeCursor *a = myg->get_node_iter();    // per thread
            NodeCursor *b = myg->get_node_iter();
            std::vector<TiledU> us;
            us.reserve(BLOCK);
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < num_blocks) {
                node_id_t start = (node_id_t)(blk * BLOCK);
                node_id_t end   = start + (node_id_t)BLOCK - 1;
                if (end > max_id) end = max_id;
                Worker(myg, a, b, start, end, o, us,
                       top, pairs, edges, my_reads);          // family's tile worker
            }
            a->close();
            b->close();
            delete a;
            delete b;
        }
        total_v_reads += my_reads;
        if (!shared) {
            per_topk[tid]  = std::move(my_top);
            per_pairs[tid] = my_pairs;
            per_edges[tid] = my_edges;
        }
    }

    for (size_t c : per_pairs) r.pairs_scored += c;
    for (size_t c : per_edges) r.true_edges   += c;
    r.top_k = linkpred::merge_topk(per_topk, (size_t)o.topk);
    std::cerr << "[tiled] tile=" << BLOCK << " v_reads=" << total_v_reads.load()
              << " pairs=" << r.pairs_scored
              << " (cursor paths read v once per pair)\n";
    return r;
}
