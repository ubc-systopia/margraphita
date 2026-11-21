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
// B_k row (the FastRP paper / Neo4j variant). --variant=paper (Chen et al. 2019,
// Alg. 1; = node_emb_fastrp.py --fastrp_variant=paper): --step-norm, B_0 = L R
// with L = diag((deg/2m)^beta) (--beta=, default -0.5), default weights
// [0, 1, ..., 1] (E still adds w_0 R).
//
// Differences from the Python version: R is drawn per (node id, column) from a
// seeded hash instead of numpy's RNG (same distribution, different numbers),
// and ids are kept as they are (no "min id 1 -> shift by 1").
//
// The adjacency is read from the DB on every propagation step (one pass over
// all nodes per step, threaded with block stealing); nothing is loaded from a
// text edge list. The embedding is written as a float32 .npy with one row per
// node id (rows 0 .. max_id; ids without a node stay 0), which is the layout
// test/graphml/add_pytorch_emb_npy reads to store it in node_props.
//
// Usage:
//   fg_fastrp -p DB_DIR -m DB_NAME -g adj -r [-d] --out=FILE.npy
//             [--dim=128] [--iters=4] [--weights=0.5,1,1,1] [--density=0.01]
//             [--seed=42] [--threads=16] [--step-norm] [--adj-read=cursor|api|csr]
//             [--variant=orig|paper] [--beta=-0.5]
//             [--write-db[=fused|after]]
//
// --write-db (= fused): write each node's final embedding into the DB's node_props
//   table (float32 blob, the layout add_pytorch_emb_npy writes and the linkpred
//   embedding paths read) as it goes: in the last propagation step each thread
//   finishes row u (add w_last * B_last, L2-normalise) and writes it right away
//   from its own handle, so there is no separate write pass. node_props is
//   created if missing. --out becomes optional.
// --write-db=after: compute everything first, then one threaded write pass,
//   timed on its own (write_s); for comparison with fused.
//
// --adj-read=cursor (default): neighbour lists through the handle's OutCursor /
//   InCursor (reset + set_key_range({u,u}) + next), one lookup per list. Also
//   works around AdjList::has_node checking adjlistout: through the API,
//   get_in_nodes_id returns {} for nodes with no out-edges (sinks), so their
//   in-lists were lost on directed bulk-loaded DBs.
// --adj-read=api: get_out_nodes_id / get_in_nodes_id (has_node + lookup).
// --adj-read=csr: in-memory baseline. Read every neighbour list ONCE with the
//   cursors (same lists as cursor mode, symmetrised, sorted, deduped), keep
//   them as a CSR in RAM (csr_s, printed separately), then run the identical
//   steps from the CSR with no DB reads. cursor vs csr isolates the cost of
//   re-reading the adjacency from WiredTiger each step.

#include <omp.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sys/resource.h>   // getrusage: peak_rss_mb in the last log line

#include "command_line.h"
#include "graph_engine.h"

namespace {

struct FrpOpts {
    int dim = 128, iters = 4, threads = 16;
    double density = 0.01;
    uint64_t seed = 42;
    std::vector<float> weights;
    bool step_norm = false;
    bool paper = false;        // --variant=paper: step_norm + degree^beta scaling of R, weights [0,1,..]
    double beta = -0.5;        // --beta= (paper only)
    bool cursor_read = true;   // --adj-read=cursor (default) | api has node (not right)
    bool csr = false;          // --adj-read=csr: lists read once into an in-memory CSR
    std::string out;
    std::string write_db;      // "" = off | fused | after
};

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

bool parse_frp(int argc, char **argv, FrpOpts &f, std::vector<char *> &rest) {
    rest.push_back(argv[0]);
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto val = [&](const char *k) { return a.substr(std::strlen(k)); };
        if      (a.rfind("--dim=", 0) == 0)      f.dim = std::stoi(val("--dim="));
        else if (a.rfind("--iters=", 0) == 0)    f.iters = std::stoi(val("--iters="));
        else if (a.rfind("--threads=", 0) == 0)  f.threads = std::stoi(val("--threads="));
        else if (a.rfind("--density=", 0) == 0)  f.density = std::stod(val("--density="));
        else if (a.rfind("--seed=", 0) == 0)     f.seed = std::stoull(val("--seed="));
        else if (a.rfind("--out=", 0) == 0)      f.out = val("--out=");
        else if (a == "--step-norm")             f.step_norm = true;
        else if (a == "--variant=orig")          f.paper = false;
        else if (a == "--variant=paper")         f.paper = true;
        else if (a.rfind("--beta=", 0) == 0)     f.beta = std::stod(val("--beta="));
        else if (a == "--adj-read=api")          f.cursor_read = false;
        else if (a == "--adj-read=cursor")       f.cursor_read = true;
        else if (a == "--adj-read=csr")          { f.cursor_read = true; f.csr = true; }
        else if (a == "--write-db" || a == "--write-db=fused") f.write_db = "fused";
        else if (a == "--write-db=after")        f.write_db = "after";
        else if (a.rfind("--weights=", 0) == 0) {
            std::string s = val("--weights="); size_t p = 0;
            while (p <= s.size()) {
                size_t q = s.find(',', p); if (q == std::string::npos) q = s.size();
                if (q > p) f.weights.push_back(std::stof(s.substr(p, q - p)));
                p = q + 1;
            }
        }
        else rest.push_back(argv[i]);
    }
    if (f.paper) f.step_norm = true;
    if (f.weights.empty()) { f.weights.assign(f.iters, 1.0f); if (f.iters > 0) f.weights[0] = f.paper ? 0.0f : 0.5f; }
    if ((int)f.weights.size() != f.iters) { std::cerr << "ERROR: --weights needs " << f.iters << " values\n"; return false; }
    if (f.out.empty() && f.write_db.empty()) { std::cerr << "ERROR: --out=FILE.npy and/or --write-db is required\n"; return false; }
    if (f.dim < 1 || f.iters < 1 || f.threads < 1) { std::cerr << "ERROR: --dim, --iters, --threads must be >= 1\n"; return false; }
    return true;
}

void write_npy(const std::string &path, const std::vector<float> &E, size_t rows, int d) {
    std::string hdr = "{'descr': '<f4', 'fortran_order': False, 'shape': (" +
                      std::to_string(rows) + ", " + std::to_string(d) + "), }";
    size_t total = 10 + hdr.size() + 1;
    hdr += std::string((64 - total % 64) % 64, ' ') + "\n";
    std::ofstream o(path, std::ios::binary);
    o.write("\x93NUMPY\x01\x00", 8);
    uint16_t hl = (uint16_t)hdr.size();
    o.write(reinterpret_cast<const char *>(&hl), 2);
    o.write(hdr.data(), hdr.size());
    o.write(reinterpret_cast<const char *>(E.data()), E.size() * sizeof(float));
}

double secs_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

}  // namespace

int main(int argc, char **argv)
{
    FrpOpts f;
    std::vector<char *> rest;
    if (!parse_frp(argc, argv, f, rest)) return 1;
    CmdLineApp cli((int)rest.size(), rest.data());
    if (!cli.parse_args()) return 1;
    cmdline_opts opts = cli.get_parsed_opts();
    opts.read_only = f.write_db.empty();          // writing handles: normal read/write cursors
    opts.create_new = false;
    if (!f.write_db.empty()) {                    // handles open the node_props cursor
        opts.has_node_props = true;
        opts.prop_mode      = EMBEDDED;
    }
    const bool directed = opts.is_directed;   // -d: also read in-neighbours (symmetrise)

    auto t_all = std::chrono::steady_clock::now();
    GraphEngine engine(f.threads + 1, opts);
    if (!f.write_db.empty()) {
        // Bulk-loaded DBs have no node_props table: create it before the handles
        // open their cursors (same config as AdjList / add_pytorch_emb_npy; a
        // no-op if it exists). Adj only, like add_pytorch_emb_npy.
        if (opts.type != GraphType::Adj) { std::cerr << "ERROR: --write-db supports -g adj only\n"; return 1; }
        WT_CONNECTION *conn = engine.get_connection();
        WT_SESSION *sess = nullptr;
        int ret = conn->open_session(conn, nullptr, nullptr, &sess);
        if (ret == 0)
            ret = sess->create(sess, (std::string("table:") + AdjList::NODE_PROPS_TABLE).c_str(),
                               "key_format=u,value_format=u,leaf_page_max=64KB");
        if (sess) sess->close(sess, nullptr);
        if (ret != 0) { std::cerr << "ERROR: node_props table: " << wiredtiger_strerror(ret) << "\n"; return 1; }
    }
    GraphBase *g = engine.create_graph_handle();
    std::vector<GraphBase *> h(f.threads);
    for (int t = 0; t < f.threads; ++t) h[t] = engine.create_graph_handle();

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
              << " variant=" << (f.paper ? "paper" : "orig") << (f.paper ? " beta=" + std::to_string(f.beta) : std::string())
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

    // ---- --variant=paper: B_0 = L R, L = diag((deg_i / 2m)^beta) (E keeps w_0 * R) ----
    // deg = symmetrised, deduplicated degree, as used by S. From the CSR when
    // there is one; otherwise one extra threaded pass over the DB lists (deg_s).
    if (f.paper && f.beta != 0.0 && f.iters > 1) {
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

    // ---- write .npy (optional with --write-db): row = node id (0 .. max_id) ----
    const size_t rows = (size_t)max_id + 1;
    const double gb = (double)rows * d * 4 / 1e9;
    if (!f.out.empty()) {
        if (!dense_ids)
            std::cerr << "[fastrp] warning: sparse ids, .npy has " << rows << " rows (" << gb
                      << " GB) for " << n << " nodes\n";
        std::vector<float> out(rows * (size_t)d, 0.0f);
        for (size_t i = 0; i < n; ++i) std::memcpy(&out[(size_t)ids[i] * d], &E[i * d], d * sizeof(float));
        write_npy(f.out, out, rows, d);
    }

    const bool wrote = !f.write_db.empty();
    for (auto *x : h) x->close(false);   // false: inserts are autocommitted; sync_metadata would
    g->close(false);                     // rewrite graph metadata from every handle
    engine.close_graph();
    std::cerr << "[fastrp] wrote " << (f.out.empty() ? "" : f.out + " (" + std::to_string(rows) + " x " + std::to_string(d) + ") ")
              << (wrote ? "node_props " : "")
              << "total_s=" << secs_since(t_all)
              << " peak_rss_mb=" << [] { rusage u{}; getrusage(RUSAGE_SELF, &u); return u.ru_maxrss / 1024; }() << "\n";
    return 0;
}
