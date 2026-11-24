// fastrp.cpp -- FastRP node embeddings computed directly from a Flexograph DB.
//
// Same maths as scripts-tests/node_embeddings/node_emb_fastrp.py (PyG version):
//   A    = symmetrised adjacency (out- and in-neighbours, duplicates removed)
//   S    = D^-1 A                       (row average over neighbours)
//   B_0  = R                            (very sparse random projection, +-sqrt(3))
//   B_k  = S . B_{k-1}                  (k = 1 .. p-1)
//   E    = sum_k w_k B_k, then each row L2-normalised
//   w    = [0.5, 1.0, 1.0, ...] by default (--weights to override)
// Note: like the Python version, k = 0 is the raw random R, so --iters=1 gives
// R only (no graph information). --step-norm additionally L2-normalises every
// B_k row (the FastRP paper / Neo4j variant). --variant=fastrp-paper-variant (Chen et al. 2019,
// Alg. 1; = node_emb_fastrp.py --fastrp_variant=paper): --step-norm, B_0 = L R
// with L = diag((deg/2m)^beta) (--beta=, default -0.5), default weights
// [0, 1, ..., 1] (E still adds w_0 R).
//
// Differences from the Python version: R is drawn per (node id, column) from a
// seeded hash instead of numpy's RNG (same distribution, different numbers),
// and ids are kept as they are (no "min id 1 -> shift by 1").
#include "ml/algorithms/fastrp.h"
#include <omp.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace ml {
namespace {
uint64_t splitmix(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// Row u of R: each entry non-zero with probability `density`, value +-sqrt(3);
// at least one non-zero per row (the Python version draws >= n non-zeros).
void random_row(uint64_t seed, uint64_t u, int d, double density, float *row) {
    const float v = std::sqrt(3.0f);
    bool any = false;
    for (int j = 0; j < d; ++j) {
        uint64_t h = splitmix(seed ^ splitmix(u * (uint64_t)d + (uint64_t)j));
        double r = (double)(h >> 11) * (1.0 / 9007199254740992.0);   // [0,1)
        row[j] = 0.0f;
        if (r < density) { row[j] = (h & 1) ? v : -v; any = true; }
    }
    if (!any) {
        uint64_t h = splitmix(seed ^ splitmix(u ^ 0xabcdefULL));
        row[h % (uint64_t)d] = (h & 0x100) ? v : -v;
    }
}
double secs_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}
 } // namespace

FastRPResult fastrp(GraphBase& graph, const std::vector<GraphBase*>& h,
                    FastRPOptions f, bool directed)
{
    if (f.dim < 1 || f.iters < 1 || f.threads < 1 ||
        h.size() < static_cast<size_t>(f.threads))
        throw std::invalid_argument("Invalid FastRP options or worker count");
    if (f.fastrp_paper_variant) f.step_norm = true;
    if (f.weights.empty()) {
        f.weights.assign(f.iters, 1.0f);
        f.weights[0] = f.fastrp_paper_variant ? 0.0f : 0.5f;
    }
    if (f.weights.size() != static_cast<size_t>(f.iters))
        throw std::invalid_argument("Invalid FastRP weight count");
    GraphBase* g = &graph;
    // ---- node ids (sparse ids allowed) -> dense index ----
    std::vector<node_id_t> ids;
    {
        NodeCursor *a = g->get_node_iter();
        a->set_key_range({0, OutOfBand_ID_MAX});
        node u; a->next(&u);
        while (u.id != OutOfBand_ID_MAX) { ids.push_back(u.id); a->next(&u); }
        a->close(); delete a;
    }
    const size_t n = ids.size();
    const node_id_t max_id = n ? ids.back() : 0;
    const bool dense_ids = (uint64_t)max_id + 1 <= 4 * (uint64_t)n + 1024;
    std::vector<uint32_t> idx_dense;
    std::unordered_map<node_id_t, uint32_t> idx_map;
    if (dense_ids) { idx_dense.assign((size_t)max_id + 1, UINT32_MAX); for (size_t i = 0; i < n; ++i) idx_dense[ids[i]] = (uint32_t)i; }
    else { idx_map.reserve(n * 2); for (size_t i = 0; i < n; ++i) idx_map[ids[i]] = (uint32_t)i; }
    auto index_of = [&](node_id_t x) -> uint32_t {
        if (dense_ids) return x <= max_id ? idx_dense[x] : UINT32_MAX;
        auto it = idx_map.find(x); return it == idx_map.end() ? UINT32_MAX : it->second;
    };

    const int d = f.dim;
    std::cerr << "[fastrp] nodes=" << n << " max_id=" << max_id << " dim=" << d
              << " iters=" << f.iters << " density=" << f.density << " seed=" << f.seed
              << " threads=" << f.threads << " directed(symmetrise)=" << directed
              << " step_norm=" << f.step_norm
              << " variant=" << (f.fastrp_paper_variant ? "fastrp-paper-variant" : "orig") << (f.fastrp_paper_variant ? " beta=" + std::to_string(f.beta) : std::string())
              << " adj_read=" << (f.csr ? "csr" : (f.cursor_read ? "cursor" : "api"))
              << " write_db=" << (f.write_db.empty() ? "off" : f.write_db) << "\n";

    // L2-normalise row e in place (the Python version normalises the final E)
    auto l2norm = [d = f.dim](float *e) {
        double s = 0; for (int j = 0; j < d; ++j) s += (double)e[j] * e[j];
        if (s > 0) { float inv = (float)(1.0 / std::sqrt(s)); for (int j = 0; j < d; ++j) e[j] *= inv; }
    };
    const bool fused = (f.write_db == "fused") && f.iters > 1;   // iters=1: no step to fuse into
    std::atomic<size_t> db_written{0};

    // ---- B_0 = R; E = w_0 * R ----
    std::vector<float> B((size_t)n * d), Bn((size_t)n * d), E((size_t)n * d, 0.0f);
    #pragma omp parallel for num_threads(f.threads) schedule(static)
    for (size_t i = 0; i < n; ++i) {
        float *b = &B[i * d];
        random_row(f.seed, ids[i], d, f.density, b);
        for (int j = 0; j < d; ++j) E[i * d + j] = f.weights[0] * b[j];
    }

    // ---- --adj-read=csr: read every neighbour list once into an in-memory CSR ----
    std::vector<size_t> coff;          // n + 1 offsets
    std::vector<uint32_t> cidx;        // neighbour rows (dense indices)
    if (f.csr) {
        auto t_c = std::chrono::steady_clock::now();
        std::vector<std::vector<uint32_t>> rows(n);
        const size_t BLOCK = 64, nblocks = (n + BLOCK - 1) / BLOCK;
        std::atomic<size_t> next_block{0};
        #pragma omp parallel num_threads(f.threads)
        {
            GraphBase *my = h[omp_get_thread_num()];
            OutCursor *oc = my->get_outnbd_iter();
            InCursor  *ic = directed ? my->get_innbd_iter() : nullptr;
            adjlist al{0, 0};
            auto add_list = [&](auto *cur, node_id_t x, std::vector<uint32_t> &nb) {   // as in the step loop
                cur->reset();
                cur->set_key_range({x, x});
                al.edgelist.clear(); al.degree = 0;
                cur->next(&al);
                if (al.node_id == x)
                    for (node_id_t w : al.edgelist) { uint32_t y = index_of(w); if (y != UINT32_MAX) nb.push_back(y); }
            };
            size_t blk;
            while ((blk = next_block.fetch_add(1)) < nblocks)
                for (size_t i = blk * BLOCK; i < std::min(n, (blk + 1) * BLOCK); ++i) {
                    std::vector<uint32_t> &nb = rows[i];
                    add_list(oc, ids[i], nb);
                    if (directed) add_list(ic, ids[i], nb);
                    std::sort(nb.begin(), nb.end());
                    nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
                }
            oc->close(); delete oc;
            if (ic) { ic->close(); delete ic; }
        }
        coff.assign(n + 1, 0);
        for (size_t i = 0; i < n; ++i) coff[i + 1] = coff[i] + rows[i].size();
        cidx.resize(coff[n]);
        #pragma omp parallel for num_threads(f.threads) schedule(static)
        for (size_t i = 0; i < n; ++i) std::copy(rows[i].begin(), rows[i].end(), cidx.begin() + coff[i]);
        std::vector<std::vector<uint32_t>>().swap(rows);
        std::cerr << "[fastrp] csr built adj_entries=" << coff[n] << " ("
                  << (double)(cidx.size() * 4 + coff.size() * 8) / 1e9 << " GB) csr_s=" << secs_since(t_c) << "\n";
    }

    // ---- --variant=fastrp-paper-variant: B_0 = L R, L = diag((deg_i / 2m)^beta) (E keeps w_0 * R) ----
    // deg = symmetrised, deduplicated degree, as used by S. From the CSR when
    // there is one; otherwise one extra threaded pass over the DB lists (deg_s).
    if (f.fastrp_paper_variant && f.beta != 0.0 && f.iters > 1) {
        auto t_d = std::chrono::steady_clock::now();
        std::vector<uint32_t> deg(n, 0);
        if (f.csr) {
            for (size_t i = 0; i < n; ++i) deg[i] = (uint32_t)(coff[i + 1] - coff[i]);
        } else {
            const size_t BLOCK = 64, nblocks = (n + BLOCK - 1) / BLOCK;
            std::atomic<size_t> next_block{0};
            #pragma omp parallel num_threads(f.threads)
            {
                GraphBase *my = h[omp_get_thread_num()];
                OutCursor *oc = my->get_outnbd_iter();
                InCursor  *ic = directed ? my->get_innbd_iter() : nullptr;
                std::vector<uint32_t> nb;
                adjlist al{0, 0};
                auto add_list = [&](auto *cur, node_id_t x) {
                    cur->reset();
                    cur->set_key_range({x, x});
                    al.edgelist.clear(); al.degree = 0;
                    cur->next(&al);
                    if (al.node_id == x)
                        for (node_id_t w : al.edgelist) { uint32_t y = index_of(w); if (y != UINT32_MAX) nb.push_back(y); }
                };
                size_t blk;
                while ((blk = next_block.fetch_add(1)) < nblocks)
                    for (size_t i = blk * BLOCK; i < std::min(n, (blk + 1) * BLOCK); ++i) {
                        nb.clear();
                        add_list(oc, ids[i]);
                        if (directed) add_list(ic, ids[i]);
                        std::sort(nb.begin(), nb.end());
                        deg[i] = (uint32_t)(std::unique(nb.begin(), nb.end()) - nb.begin());
                    }
                oc->close(); delete oc;
                if (ic) { ic->close(); delete ic; }
            }
        }
        double two_m = 0; for (size_t i = 0; i < n; ++i) two_m += deg[i];
        #pragma omp parallel for num_threads(f.threads) schedule(static)
        for (size_t i = 0; i < n; ++i) {
            if (!deg[i] || two_m <= 0) continue;            // isolated: no neighbour reads it
            const float sc = (float)std::pow((double)deg[i] / two_m, f.beta);
            for (int j = 0; j < d; ++j) B[i * d + j] *= sc;
        }
        std::cerr << "[fastrp] degree scaling beta=" << f.beta << " two_m=" << (uint64_t)two_m
                  << " deg_s=" << secs_since(t_d) << "\n";
    }

    // ---- B_k = S . B_{k-1}: one threaded pass over the DB adjacency per step ----
    size_t edges_seen = 0;
    for (int k = 1; k < f.iters; ++k) {
        auto t_k = std::chrono::steady_clock::now();
        const size_t BLOCK = 64, nblocks = (n + BLOCK - 1) / BLOCK;
        std::atomic<size_t> next_block{0}, edges{0};
        #pragma omp parallel num_threads(f.threads)
        {
            GraphBase *my = h[omp_get_thread_num()];
            OutCursor *oc = (f.cursor_read && !f.csr) ? my->get_outnbd_iter() : nullptr;   // own cursors since 30b1c40:
            InCursor  *ic = (f.cursor_read && !f.csr && directed) ? my->get_innbd_iter() : nullptr;   // closed after the step
            std::vector<uint32_t> nb;
            adjlist al{0, 0};
            auto add_list = [&](auto *cur, node_id_t x) {
                cur->reset();
                cur->set_key_range({x, x});
                al.edgelist.clear(); al.degree = 0;
                cur->next(&al);
                if (al.node_id == x)
                    for (node_id_t w : al.edgelist) { uint32_t y = index_of(w); if (y != UINT32_MAX) nb.push_back(y); }
            };
            size_t my_edges = 0, blk;
            while ((blk = next_block.fetch_add(1)) < nblocks) {
                for (size_t i = blk * BLOCK; i < std::min(n, (blk + 1) * BLOCK); ++i) {
                    const uint32_t *nbp; size_t nbn;
                    if (f.csr) {                     // in-memory: the row built once above
                        nbp = cidx.data() + coff[i];
                        nbn = coff[i + 1] - coff[i];
                    } else {
                    nb.clear();
                    if (f.cursor_read) {
                        add_list(oc, ids[i]);
                        if (directed) add_list(ic, ids[i]);
                    } else {
                        for (node_id_t w : my->get_out_nodes_id(ids[i])) { uint32_t x = index_of(w); if (x != UINT32_MAX) nb.push_back(x); }
                        if (directed)
                            for (node_id_t w : my->get_in_nodes_id(ids[i])) { uint32_t x = index_of(w); if (x != UINT32_MAX) nb.push_back(x); }
                    }
                    std::sort(nb.begin(), nb.end());
                    nb.erase(std::unique(nb.begin(), nb.end()), nb.end());   // A = max(A, A^T): binary
                    nbp = nb.data(); nbn = nb.size();
                    }
                    my_edges += nbn;
                    float *o = &Bn[i * d];
                    std::fill(o, o + d, 0.0f);
                    for (size_t q = 0; q < nbn; ++q) { const float *src = &B[(size_t)nbp[q] * d]; for (int j = 0; j < d; ++j) o[j] += src[j]; }
                    if (nbn) { float inv = 1.0f / (float)nbn; for (int j = 0; j < d; ++j) o[j] *= inv; }
                    if (f.step_norm) {
                        double s = 0; for (int j = 0; j < d; ++j) s += (double)o[j] * o[j];
                        if (s > 0) { float inv = (float)(1.0 / std::sqrt(s)); for (int j = 0; j < d; ++j) o[j] *= inv; }
                    }
                    float *e = &E[i * d];
                    for (int j = 0; j < d; ++j) e[j] += f.weights[k] * o[j];
                    if (k == f.iters - 1) {          // last step: row i is final
                        l2norm(e);
                        if (fused) {                 // --write-db: store it now
                            my->set_node_properties(ids[i], reinterpret_cast<const uint8_t *>(e), d * sizeof(float));
                            db_written.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                }
            }
            edges += my_edges;
            if (oc) { oc->close(); delete oc; }
            if (ic) { ic->close(); delete ic; }
        }
        std::swap(B, Bn);
        edges_seen = edges.load();
        std::cerr << "[fastrp] step " << k << "/" << (f.iters - 1) << " adj_entries=" << edges_seen
                  << " time_s=" << secs_since(t_k) << "\n";
    }

    // ---- final L2 normalisation: done inside the last step; iters=1 has no step ----
    if (f.iters == 1) {
        #pragma omp parallel for num_threads(f.threads) schedule(static)
        for (size_t i = 0; i < n; ++i) l2norm(&E[i * d]);
    }

    // ---- --write-db=after (or fused with iters=1): one threaded write pass ----
    if (!f.write_db.empty() && !fused) {
        auto t_w = std::chrono::steady_clock::now();
        #pragma omp parallel num_threads(f.threads)
        {
            GraphBase *my = h[omp_get_thread_num()];
            #pragma omp for schedule(static)
            for (size_t i = 0; i < n; ++i)
                my->set_node_properties(ids[i], reinterpret_cast<const uint8_t *>(&E[i * d]), d * sizeof(float));
        }
        db_written = n;
        std::cerr << "[fastrp] write_db=after rows=" << n << " write_s=" << secs_since(t_w) << "\n";
    }
    if (!f.write_db.empty())
        std::cerr << "[fastrp] node_props rows written=" << db_written.load() << " (" << d * sizeof(float) << " B each)\n";
    return {std::move(ids), {n, static_cast<size_t>(d), std::move(E)}};
}
} // namespace ml
