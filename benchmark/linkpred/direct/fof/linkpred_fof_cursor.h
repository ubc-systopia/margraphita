// linkpred_fof_cursor.h -- shared driver for --candidates=fof --variant=cursor,
// used by both families:
//   run_direct_fof_structural_cursor -> run_direct_fof_cursor<structural_worker_fof>
//   run_direct_fof_embedding_cursor  -> run_direct_fof_cursor<embedding_worker_fof>
//
// The two drivers were identical apart from the worker call, so the worker is
// a template argument. It is fixed at compile time: each family gets its own
// instantiation with a direct call to its worker (no runtime check, no
// function pointer), i.e. the same code as the two hand-written copies.
//
// Threaded. Atomic block stealing over 64-node blocks of u; one TopK heap per
// thread; merge at the end. --threads=1 runs the same code with one thread.
// The wedge variant (linkpred_structural_fof.h) keeps its own driver.

#pragma once

#include <algorithm>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <unordered_set>
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
static RunResult run_direct_fof_cursor(GraphBase *g, const Options &o)
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


// ---------------------------------------------------------------- CSR cursor driver (--variant=csr_cursor / embedding csr)
// In-memory twin of the cursor workers (structural_worker_fof / _khop and the
// embedding ones): same candidate walk with the same per-u hash containers,
// same scoring, but every neighbour list is a slice of a preloaded CSR instead
// of a WiredTiger read. The gap to --variant=cursor is the DB read cost alone
// (the wedge variants also change the algorithm).
//
// Candidates:  walk2 = true  -> FoF: v reached by u -> w -> v, v > u
//              walk2 = false -> khop: BFS from u to depth k, v > u, d >= 1
// Blocks are 64 CSR rows (node indices), block stealing as the cursor driver.
// Csr:   nodes (sorted ids), offsets (nodes.size()+1), edges (sorted per row).
// Score: double(node_id_t u, const node_id_t *Nu, size_t du,
//               node_id_t v, const node_id_t *Nv, size_t dv)
// id -> CSR row. A plain array when ids are dense (max_id + 1 <= 4n + 1024, as in
// fg_fastrp and wedge_compact), otherwise a hash map, so graphs with sparse ids
// (ego-Twitter: 81K nodes, max id 568,770,231) no longer allocate max_id + 1 slots
// (2.3 GB). Added 2026-11-22; on dense-id graphs it is the same array as before.
struct CsrRowIndex {
    std::vector<uint32_t> dense;
    std::unordered_map<node_id_t, uint32_t> sparse;
    bool use_dense = true;
    template <typename Ids>
    explicit CsrRowIndex(const Ids &nodes) {
        const size_t n = nodes.size();
        const node_id_t max_id = n ? nodes.back() : 0;
        use_dense = (uint64_t)max_id + 1 <= 4 * (uint64_t)n + 1024;
        if (use_dense) {
            dense.assign((size_t)max_id + 1, UINT32_MAX);
            for (size_t i = 0; i < n; ++i) dense[nodes[i]] = (uint32_t)i;
        } else {
            sparse.reserve(n * 2);
            for (size_t i = 0; i < n; ++i) sparse[nodes[i]] = (uint32_t)i;
        }
    }
    uint32_t operator()(node_id_t x) const {
        if (use_dense) return x < dense.size() ? dense[x] : UINT32_MAX;
        auto it = sparse.find(x);
        return it == sparse.end() ? UINT32_MAX : it->second;
    }
};

template <typename Csr, typename Score>
static RunResult run_csr_cursor(const Csr &csr, const Options &o,
                                bool walk2, int k, Score score)
{
    RunResult r;
    const size_t n = csr.nodes.size();
    if (n == 0) return r;
    const CsrRowIndex row(csr.nodes);                    // id -> CSR row (array or hash)

    // neighbour slice of id x (empty if x has no row, as get_out_nodes_id)
    auto nbrs = [&](node_id_t x, size_t &d) -> const node_id_t * {
        uint32_t rx = row(x);
        if (rx == UINT32_MAX) { d = 0; return nullptr; }
        d = csr.offsets[rx + 1] - csr.offsets[rx];
        return csr.edges.data() + csr.offsets[rx];
    };

    const size_t BLOCK = 64;
    size_t num_blocks = (n + BLOCK - 1) / BLOCK;
    std::vector<linkpred::TopK> per_topk;
    std::vector<size_t> per_pairs(o.threads, 0), per_edges(o.threads, 0);
    per_topk.reserve(o.threads);
    for (int t = 0; t < o.threads; ++t) per_topk.emplace_back((size_t)o.topk);
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
        while ((blk = next_block.fetch_add(1)) < num_blocks) {
            for (size_t i = blk * BLOCK; i < std::min(n, (blk + 1) * BLOCK); ++i) {
                const node_id_t u = csr.nodes[i];
                const node_id_t *Nu = csr.edges.data() + csr.offsets[i];
                const size_t du = csr.offsets[i + 1] - csr.offsets[i];

                std::vector<node_id_t> vs;
                if (walk2) {                                   // as structural_worker_fof
                    std::unordered_set<node_id_t> fof;
                    for (size_t a = 0; a < du; ++a) {
                        size_t dw; const node_id_t *Nw = nbrs(Nu[a], dw);
                        for (size_t b = 0; b < dw; ++b)
                            if (Nw[b] > u) fof.insert(Nw[b]);
                    }
                    vs.assign(fof.begin(), fof.end());
                } else {                                       // as structural_worker_khop
                    std::unordered_map<node_id_t, int> dist;
                    std::queue<node_id_t> q;
                    dist[u] = 0;
                    q.push(u);
                    while (!q.empty()) {
                        node_id_t x = q.front(); q.pop();
                        int dx = dist[x];
                        if (dx == k) continue;
                        size_t dxn; const node_id_t *Nx = nbrs(x, dxn);
                        for (size_t b = 0; b < dxn; ++b)
                            if (dist.find(Nx[b]) == dist.end()) {
                                dist[Nx[b]] = dx + 1;
                                q.push(Nx[b]);
                            }
                    }
                    for (const auto &[v, d] : dist)
                        if (v > u && d >= 1) vs.push_back(v);
                }
                std::sort(vs.begin(), vs.end());

                for (node_id_t v : vs) {
                    size_t dv; const node_id_t *Nv = nbrs(v, dv);
                    int label = lp_label(u, v, std::binary_search(Nu, Nu + du, v));   // --eval: recon / holdout
                    if (label < 0) continue;                    // holdout: training edge, not scored
                    top.offer({u, v, score(u, Nu, du, v, Nv, dv), label});
                    pairs++;
                    if (label == 1) edges++;
                }
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
