// linkpred_embedding_khop.h -- embedding family (split from linkpred_khop.h).
// Split by family from linkpred_khop.h (2026-11-16); function bodies unchanged.
//
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
#include "linkpred_options.h"
#include "linkpred_scores.h"
#include "linkpred_topk.h"
#include "linkpred_types.h"
#include "linkpred_fof_cursor.h"     // run_csr_cursor (in-memory CSR driver, --variant=csr)
#include "linkpred_khop_cursor.h"    // run_direct_khop_cursor_bfs (shared cursor driver)
#include "linkpred_khop_outcursor_nested.h"   // run_direct_khop_outcursor_nested<Worker> (k = 2)

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
        auto Nu = lp_nbrs(g, u.id, lp_both_directions(o.direction));       // DB read: u's neighbours (walk + labels)
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
            for (node_id_t y : (x == u.id ? Nu : lp_nbrs(g, x, lp_both_directions(o.direction))))   // reuse N(u)
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
            int label = lp_label(u.id, v, Nu_set.count(v) != 0);   // --eval: recon / holdout
            if (label < 0) { delete[] const_cast<uint8_t*>(pv.data); continue; }   // holdout: training edge
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
// ---- Previous hand-written driver, replaced by run_direct_khop_cursor_bfs<...>
// (linkpred_khop_cursor.h, 2026-11-16). Kept commented out for reference.
// static RunResult run_direct_khop_embedding_cursor_bfs(GraphBase *g, const Options &o)
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
//     std::atomic<size_t> next_block{0};                 // work-stealing counter
//
//     #pragma omp parallel num_threads(o.threads)
//     {
//         int tid = omp_get_thread_num();
//         GraphBase *myg = g_thread_handles[tid];        // private cursor set
//         bool shared = (o.accum == "shared");
//         linkpred::TopK my_top((size_t)o.topk);
//         size_t my_pairs = 0, my_edges = 0;
//         linkpred::TopK &top   = shared ? per_topk[tid]  : my_top;
//         size_t         &pairs = shared ? per_pairs[tid] : my_pairs;
//         size_t         &edges = shared ? per_edges[tid] : my_edges;
//         if (myg) {
//             size_t blk;
//             while ((blk = next_block.fetch_add(1)) < num_blocks) {   // grab next block
//                 node_id_t start = (node_id_t)(blk * BLOCK);
//                 node_id_t end   = start + BLOCK - 1;
//                 if (end > max_id) end = max_id;
//                 embedding_worker_khop(myg, start, end, o, top, pairs, edges);
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
//     r.top_k = linkpred::merge_topk(per_topk, (size_t)o.topk);
//     return r;
// }

static RunResult run_direct_khop_embedding_cursor_bfs(GraphBase *g, const Options &o)
{
    return run_direct_khop_cursor_bfs<embedding_worker_khop>(g, o);   // driver: linkpred_khop_cursor.h
}

// ---------------------------------------------------------------- embedding k-hop, outcursor_nested (k = 2)
// Direct version of run_reference_khop_embedding_outcursor_nested (2026-11-16).
// Nested loops instead of BFS (k = 2 only): hop 1 = N(u), hop 2 = N(w) for w in N(u).
// Every adjacency read (N(u), N(w)) goes through the handle's OutCursor
// (reset + set_key_range({x, x}) + next) instead of get_out_nodes_id, which skips
// get_out_nodes_id's has_node search. The OutCursor is the handle's shared cursor:
// reset after use, never closed. Same pairs and scores as the reference.
// Driver: run_direct_khop_outcursor_nested<...> (linkpred_khop_outcursor_nested.h).
static void embedding_worker_khop_outcursor_nested(GraphBase *g,
                                    node_id_t start, node_id_t end,
                                    const Options &o,
                                    linkpred::TopK &top,
                                    size_t &pairs_scored,
                                    size_t &true_edges)
{
    OutCursor *oc = thread_out_cursor(g);          // one per thread + handle (get_outnbd_iter leaks per call)
    const bool both = lp_both_directions(o.direction);   // --direction=both on a -d DB: out U in
    adjlist al{0, 0};
    auto out_list = [&](node_id_t x) -> std::vector<node_id_t> {
        oc->reset();
        oc->set_key_range({x, x});
        al.edgelist.clear(); al.degree = 0;      // split_ekey OutCursor appends; adj overwrites
        oc->next(&al);
        std::vector<node_id_t> nx;
        if (al.node_id == x) nx = al.edgelist;     // no adjlist row: no out-edges
        if (both) lp_add_in_neighbours(g, x, nx);  // sorts + dedups
        else std::sort(nx.begin(), nx.end());
        lp_drop_test_pairs(x, nx);                 // --eval=holdout
        return nx;
    };

    NodeCursor *a = g->get_node_iter();            // outer: u
    a->set_key_range({start, end});
    node u;
    a->next(&u);

    while (u.id != OutOfBand_ID_MAX) {
        std::vector<node_id_t> Nu = out_list(u.id);
        std::unordered_set<node_id_t> Nu_set(Nu.begin(), Nu.end());

        std::unordered_set<node_id_t> cand;
        for (node_id_t w : Nu) {
            cand.insert(w);                        // hop 1: direct neighbour
            for (node_id_t v : out_list(w))        // hop 2: neighbour's neighbour
                cand.insert(v);
        }
        cand.erase(u.id);

        std::vector<node_id_t> vs;
        for (node_id_t v : cand)
            if (v > u.id) vs.push_back(v);
        std::sort(vs.begin(), vs.end());

        prop_blob pu = g->get_node_properties(u.id);  // u's embedding, once per u
        for (node_id_t v : vs) {
            prop_blob pv = g->get_node_properties(v);    // DB read: v's embedding
            int label = lp_label(u.id, v, Nu_set.count(v) != 0);   // --eval: recon / holdout
            if (label < 0) { delete[] const_cast<uint8_t*>(pv.data); continue; }   // holdout: training edge
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
    oc->reset();                                   // reused across blocks: reset, never delete / close
}

static RunResult run_direct_khop_embedding_outcursor_nested(GraphBase *g, const Options &o)
{
    return run_direct_khop_outcursor_nested<embedding_worker_khop_outcursor_nested>(g, o);   // driver: linkpred_khop_outcursor_nested.h
}

// ---- run_direct_khop_embedding_csr
//   --impl=direct --candidates=khop --method=cosine|dot --variant=csr
//   Fully in-memory: candidates (BFS to --khop=N) from the preloaded CSR (preload_csr) and
//   v's embedding from the preloaded pvector (preload_embeddings); no DB reads.
//   Same candidates and scores as --variant=cursor (run_csr_cursor, linkpred_fof_cursor.h).
template <typename Csr, typename Emb>
static RunResult run_direct_khop_embedding_csr(const Csr &csr, const Emb &all_emb, const Options &o)
{
    const bool cos = (o.method == "cosine");
    return run_csr_cursor(csr, o, /*walk2=*/false, o.k_hops,
        [&](node_id_t u, const node_id_t *, size_t, node_id_t v, const node_id_t *, size_t) {
            return cos ? linkpred::cosine(all_emb[u], all_emb[v])
                       : linkpred::dot(all_emb[u], all_emb[v]);
        });
}
