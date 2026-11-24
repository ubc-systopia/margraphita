// linkpred_structural_fof_compact.h -- wedge counting with a compact node index.
//
// --variant=wedge_compact            (fof, khop2)  N(u), N(w) via get_out_nodes_id
// --variant=outcursor_wedge_compact  (fof, khop2)  N(u), N(w) via thread_out_cursor
//
// Same candidates, scores, labels and top-k as --variant=wedge / outcursor_wedge
// (run_direct_fof_structural_wedge in linkpred_structural_fof.h). What differs is
// how per-node state is indexed:
//
//   wedge          deg[], cnt[] indexed by node id    -> sized max_id + 1
//                  blocks of 64 ids over 0 .. max_id  -> max_id / 64 blocks
//   wedge_compact  deg[], cnt[] indexed by a dense    -> sized n (node count)
//                  node index 0 .. n-1; blocks of 64
//                  existing nodes                     -> n / 64 blocks
//
// On dense-id graphs (kron, LiveJournal) both are about the same size. On sparse
// ids (ego-Twitter: 81,306 nodes, max id 568,770,231) the id-indexed version needs
// 2.3 GB per thread for cnt (OOM at 20 threads); the compact one needs 0.3 MB.
//
// id -> index: a plain array when ids are dense (max_id + 1 <= 4n + 1024, as in
// fg_fastrp), otherwise an unordered_map. Neighbour ids not in the node list are
// skipped (they cannot be candidates). The old wedge is kept unchanged.

#pragma once

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include <omp.h>
#include "common_defs.h"
#include "graph.h"

#include "linkpred_thread_handles.h"   // g_thread_handles, thread_out_cursor
#include "linkpred_options.h"
#include "linkpred_scores.h"
#include "linkpred_topk.h"
#include "linkpred_types.h"

// Node list + id -> dense index, built once before the parallel region.
struct CompactNodeIndex {
    std::vector<node_id_t> ids;        // sorted node ids, index = position
    std::vector<degree_t>  deg;        // out-degree by index (Jaccard's |N(v)|)
    std::vector<uint32_t>  dense;      // id -> index when ids are dense
    std::unordered_map<node_id_t, uint32_t> sparse;   // otherwise
    bool use_dense = true;

    uint32_t index_of(node_id_t x) const {
        if (use_dense) return x < dense.size() ? dense[x] : UINT32_MAX;
        auto it = sparse.find(x);
        return it == sparse.end() ? UINT32_MAX : it->second;
    }
};

static CompactNodeIndex build_compact_node_index(GraphBase *g)
{
    CompactNodeIndex ix;
    NodeCursor *nc = g->get_node_iter();
    nc->set_key_range({0, OutOfBand_ID_MAX});
    node n;
    nc->next(&n);
    while (n.id != OutOfBand_ID_MAX) {
        ix.ids.push_back(n.id);
        ix.deg.push_back(n.out_degree);
        nc->next(&n);
    }
    nc->close();
    delete nc;
    // The node cursor returns ids in key order; keep it explicit.
    if (!std::is_sorted(ix.ids.begin(), ix.ids.end())) {
        std::vector<size_t> ord(ix.ids.size());
        for (size_t i = 0; i < ord.size(); ++i) ord[i] = i;
        std::sort(ord.begin(), ord.end(), [&](size_t a, size_t b) { return ix.ids[a] < ix.ids[b]; });
        std::vector<node_id_t> ids2; std::vector<degree_t> deg2;
        for (size_t i : ord) { ids2.push_back(ix.ids[i]); deg2.push_back(ix.deg[i]); }
        ix.ids.swap(ids2); ix.deg.swap(deg2);
    }
    const size_t n_nodes = ix.ids.size();
    const node_id_t max_id = n_nodes ? ix.ids.back() : 0;
    ix.use_dense = (uint64_t)max_id + 1 <= 4 * (uint64_t)n_nodes + 1024;
    if (ix.use_dense) {
        ix.dense.assign((size_t)max_id + 1, UINT32_MAX);
        for (size_t i = 0; i < n_nodes; ++i) ix.dense[ix.ids[i]] = (uint32_t)i;
    } else {
        ix.sparse.reserve(n_nodes * 2);
        for (size_t i = 0; i < n_nodes; ++i) ix.sparse[ix.ids[i]] = (uint32_t)i;
    }
    return ix;
}

// One thread's work over nodes ix.ids[first .. last) (dense indices).
// cnt is indexed by dense index and is all zero between u's.
static void structural_worker_fof_wedge_compact(GraphBase *g,
                                                size_t first, size_t last,
                                                const Options &o,
                                                const CompactNodeIndex &ix,
                                                std::vector<uint32_t> &cnt,
                                                std::vector<uint32_t> &touched,
                                                linkpred::TopK &top,
                                                size_t &pairs_scored,
                                                size_t &true_edges,
                                                bool with_hop1)
{
    const bool cn = (o.method == "cn");
    const int64_t off = with_hop1 ? 1 : 0;
    const bool use_oc = (o.variant == "outcursor_wedge_compact");
    OutCursor *oc = use_oc ? thread_out_cursor(g) : nullptr;   // one per thread + handle
    const bool both = lp_both_directions(o.direction);   // --direction=both on a -d DB: out U in
    adjlist al{0, 0};
    auto out_list = [&](node_id_t x) -> std::vector<node_id_t> {
        if (!use_oc) return lp_nbrs(g, x, both);
        oc->reset();
        oc->set_key_range({x, x});
        al.edgelist.clear(); al.degree = 0;      // split_ekey OutCursor appends; adj overwrites
        oc->next(&al);
        std::vector<node_id_t> N;
        if (al.node_id == x) N = al.edgelist;         // no adjlist row: no out-edges
        if (both) lp_add_in_neighbours(g, x, N);
        lp_drop_test_pairs(x, N);                  // --eval=holdout
        return N;
    };

    for (size_t iu = first; iu < last; ++iu) {
        const node_id_t u = ix.ids[iu];
        auto Nu = out_list(u);                     // DB read: u's neighbours
        std::sort(Nu.begin(), Nu.end());

        touched.clear();
        for (node_id_t w : Nu) {
            if (with_hop1 && w > u) {
                uint32_t iw = ix.index_of(w);
                if (iw != UINT32_MAX && cnt[iw] == 0) { cnt[iw] = 1; touched.push_back(iw); }
            }
            for (node_id_t v : out_list(w)) {      // DB read: w's neighbours
                if (v <= u) continue;
                uint32_t iv = ix.index_of(v);
                if (iv == UINT32_MAX) continue;    // not a node: cannot be a candidate
                if (cnt[iv] == 0) { cnt[iv] = (uint32_t)off; touched.push_back(iv); }
                cnt[iv]++;
            }
        }
        // Ascending v as the id-indexed wedge: indices follow sorted ids.
        std::sort(touched.begin(), touched.end());

        for (uint32_t iv : touched) {
            const node_id_t v = ix.ids[iv];
            int64_t inter = (int64_t)cnt[iv] - off;
            cnt[iv] = 0;                           // reset for the next u
            int label = lp_label(u, v, std::binary_search(Nu.begin(), Nu.end(), v));   // --eval: recon / holdout
            if (label < 0) continue;                    // holdout: training edge, not scored
            double s;
            if (cn) {
                s = (double)inter;
            } else {
                int64_t uni = (int64_t)Nu.size() + (int64_t)ix.deg[iv] - inter;
                s = uni > 0 ? (double)inter / (double)uni : 0.0;
            }
            top.offer({u, v, s, label});
            pairs_scored++;
            if (label == 1) true_edges++;
        }
    }
    if (oc) oc->reset();                           // reused across blocks: never delete / close
}

// --impl=direct --candidates=fof --variant=wedge_compact | outcursor_wedge_compact
// with_hop1 = true gives the khop2 set (direct neighbours added), as in the
// id-indexed wedge; run_direct_khop_structural_wedge_compact below.
static RunResult run_direct_fof_structural_wedge_compact(GraphBase *g, const Options &o,
                                                         bool with_hop1 = false)
{
    RunResult r;
    CompactNodeIndex ix = build_compact_node_index(g);
    const size_t n = ix.ids.size();
    if (n == 0) { fprintf(stderr, "ERROR: no nodes\n"); return r; }
    // --direction=both: Jaccard's |N(v)| = |out(v) U in(v)|, as in the id-indexed wedge.
    // --eval=holdout: likewise, so hidden test pairs do not count in |N(v)|.
    if ((lp_both_directions(o.direction) || g_holdout) && o.method != "cn") {
        const bool both = lp_both_directions(o.direction);
        #pragma omp parallel for num_threads(o.threads) schedule(dynamic, 256)
        for (size_t i = 0; i < n; ++i)
            ix.deg[i] = (degree_t)lp_nbrs(g_thread_handles[omp_get_thread_num()], ix.ids[i], both).size();
    }
    fprintf(stderr, "[wedge_compact] nodes=%zu max_id=%llu index=%s cnt_per_thread_MB=%.2f blocks=%zu\n",
            n, (unsigned long long)ix.ids.back(), ix.use_dense ? "array" : "hash",
            n * 4.0 / 1e6, (n + 63) / 64);

    const size_t BLOCK = 64;
    const size_t num_blocks = (n + BLOCK - 1) / BLOCK;

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
        std::vector<uint32_t> cnt(n, 0);           // per thread, by dense index
        std::vector<uint32_t> touched;
        if (myg) {
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < num_blocks) {
                size_t first = blk * BLOCK;
                size_t last  = std::min(n, first + BLOCK);
                structural_worker_fof_wedge_compact(myg, first, last, o, ix, cnt, touched,
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

// --impl=direct --candidates=khop --khop=2 --variant=wedge_compact | outcursor_wedge_compact
static RunResult run_direct_khop_structural_wedge_compact(GraphBase *g, const Options &o)
{
    return run_direct_fof_structural_wedge_compact(g, o, /*with_hop1=*/true);
}
