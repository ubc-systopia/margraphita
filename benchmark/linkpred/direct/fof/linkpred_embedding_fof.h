// linkpred_embedding_fof.h -- embedding family (split from linkpred_fof.h).
// Split by family from linkpred_fof.h (2026-11-16); function bodies unchanged.
//
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
#include "linkpred_fof_cursor.h"     // run_csr_cursor (in-memory CSR driver, --variant=csr)

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
        auto Nu = lp_nbrs(g, u.id, lp_both_directions(o.direction));       // DB read: u's neighbours (walk + labels)
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        // candidates: same walk as structural_worker_fof
        std::unordered_set<node_id_t> fof;
        for (node_id_t w : Nu)
            for (node_id_t v : lp_nbrs(g, w, lp_both_directions(o.direction)))   // DB read: w's neighbours
                if (v > u.id) fof.insert(v);
        std::vector<node_id_t> vs(fof.begin(), fof.end());
        std::sort(vs.begin(), vs.end());

        prop_blob pu = g->get_node_properties(u.id);  // u's embedding, once per u
        for (node_id_t v : vs) {
            prop_blob pv = g->get_node_properties(v);    // DB read: v's embedding (replaces N(v))
            int label = lp_label(u.id, v, Nu_set.count(v) != 0);   // --eval: recon / holdout
            if (label < 0) { delete[] const_cast<uint8_t*>(pv.data); continue; }                    // holdout: training edge, not scored
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

// ---- run_direct_fof_embedding_csr
//   --impl=direct --candidates=fof --method=cosine|dot --variant=csr
//   Fully in-memory: candidates (FoF walk) from the preloaded CSR (preload_csr) and
//   v's embedding from the preloaded pvector (preload_embeddings); no DB reads.
//   Same candidates and scores as --variant=cursor (run_csr_cursor, linkpred_fof_cursor.h).
template <typename Csr, typename Emb>
static RunResult run_direct_fof_embedding_csr(const Csr &csr, const Emb &all_emb, const Options &o)
{
    const bool cos = (o.method == "cosine");
    return run_csr_cursor(csr, o, /*walk2=*/true, 2,
        [&](node_id_t u, const node_id_t *, size_t, node_id_t v, const node_id_t *, size_t) {
            return cos ? linkpred::cosine(all_emb[u], all_emb[v])
                       : linkpred::dot(all_emb[u], all_emb[v]);
        });
}
