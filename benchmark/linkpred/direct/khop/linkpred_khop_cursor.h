// linkpred_khop_cursor.h -- shared driver for --candidates=khop --variant=cursor (BFS),
// used by both families:
//   run_direct_khop_structural_cursor_bfs -> run_direct_khop_cursor_bfs<structural_worker_khop>
//   run_direct_khop_embedding_cursor_bfs  -> run_direct_khop_cursor_bfs<embedding_worker_khop>
//
// The two drivers were identical apart from the worker call, so the worker is
// a template argument. It is fixed at compile time: each family gets its own
// instantiation with a direct call to its worker (no runtime check, no
// function pointer), i.e. the same code as the two hand-written copies.
//
// Threaded. Atomic block stealing over 64-node blocks of u; one TopK heap per
// thread; merge at the end. --threads=1 runs the same code with one thread.
// Any --khop >= 1. The wedge variant (run_direct_khop_structural_wedge) keeps
// its own path (it calls run_direct_fof_structural_wedge with with_hop1).

#pragma once

#include <atomic>
#include <cstdio>
#include <vector>

#include <omp.h>
#include "common_defs.h"
#include "graph.h"

#include "linkpred_thread_handles.h"   // g_thread_handles
#include "linkpred_options.h"
#include "linkpred_topk.h"
#include "linkpred_types.h"

// Worker: void(GraphBase *g, node_id_t start, node_id_t end, const Options &o,
//             linkpred::TopK &top, size_t &pairs_scored, size_t &true_edges)
template <auto Worker>
static RunResult run_direct_khop_cursor_bfs(GraphBase *g, const Options &o)
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
        bool shared = (o.accum == "shared");           // --accum, see linkpred_structural_exhaustive.h
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
                Worker(myg, start, end, o, top, pairs, edges);       // family's worker
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
