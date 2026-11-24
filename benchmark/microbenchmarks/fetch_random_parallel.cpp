// fetch_random_parallel.cpp — do random embedding fetches scale with threads?
//
// Companion to next_vs_cosine.cpp (single-thread costs). That one says how
// expensive one random fetch is next to one cosine; this one says whether
// running T fetches at once (what a prefetcher does) brings the effective cost
// per fetch down.
//
// For each T in --threads (default 1,2,4,8), T threads each with their own
// read-only handle fetch get_node_properties(id) over the same shuffled id list
// (thread t takes every T-th id). Reported per T:
//   par<T>_ns_per_fetch   wall-clock ns / total fetches (effective cost with
//                         T fetches in flight)
//   par<T>_speedup        vs the first T in the list
//   par<T>/cosine_<D>d_compute_only   effective fetch / one cache-hot cosine at
//                         the DB's dim: < 1 means T fetchers keep up with one
//                         scoring thread (what a prefetch pipeline needs)
// If ns_per_fetch keeps falling as T grows, parallel fetching scales; if it
// flattens, something shared (cache, locks, memory bandwidth) is the limit.
//
// Method: one untimed warm-up pass per thread, a barrier, then >= --calls
// fetches in total; --rounds repeats and the median is reported.
//
// Usage:
//   fetch_random_parallel -p <db_dir> -m <db_name> -g adj -r
//                         [--threads=1,2,4,8] [--calls=N] [--rounds=R]
//                         [--wt-cache=16MB] [--out=FILE.csv]
//   --wt-cache  WiredTiger cache size (default: WiredTiger's own); drop the OS
//               page cache too for a cold run
//   --out       append one CSV row per reported value (long format)

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <omp.h>

#include "command_line.h"
#include "common_defs.h"
#include "graph_engine.h"
#include "linkpred_scores.h"   // linkpred::cosine_from_ptrs (same code as the benchmark)

using Clock = std::chrono::steady_clock;

static double ns_since(Clock::time_point t0)
{
    return std::chrono::duration<double, std::nano>(Clock::now() - t0).count();
}

static double median(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

// Wall-clock ns per fetch with T threads fetching the shuffled ids at once.
static double time_fetch_parallel(const std::vector<GraphBase *> &hs, int T,
                                  const std::vector<node_id_t> &ids, size_t min_calls)
{
    const size_t per_thread = (min_calls + T - 1) / T;
    size_t total = 0;
    double ns = 0.0;
    Clock::time_point t0;                                    // shared: set once, read once
    #pragma omp parallel num_threads(T) reduction(+ : total)
    {
        int t = omp_get_thread_num();
        GraphBase *h = hs[t];
        size_t sink = 0;
        for (size_t i = t; i < ids.size(); i += T) {        // warm-up, untimed
            prop_blob pb = h->get_node_properties(ids[i]);
            sink += pb.size;
            if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
        }
        #pragma omp barrier
        #pragma omp single
        t0 = Clock::now();                                   // implicit barrier
        size_t done = 0;
        while (done < per_thread) {
            for (size_t i = t; i < ids.size() && done < per_thread; i += T) {
                prop_blob pb = h->get_node_properties(ids[i]);
                sink += pb.size;
                if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
                ++done;
            }
        }
        total += done;
        #pragma omp barrier
        #pragma omp single
        ns = ns_since(t0);
        if (sink == 42) std::printf(" ");                    // keep sink live
    }
    return ns / (double)total;
}

// One cosine on up to 4096 cache-hot vectors: the scorer's cost when it reads
// from a prefetch buffer (same as next_vs_cosine's cosine_<D>d_compute_only).
// `data` holds REAL embeddings read from this DB (see main).
static double time_cosine_compute_only(const std::vector<float> &data, size_t dim,
                                       size_t calls)
{
    const size_t vecs = data.size() / dim;
    double sink = 0.0;
    for (size_t k = 0; k < vecs; ++k)                        // warm-up, untimed
        sink += linkpred::cosine_from_ptrs(&data[k * dim],
                                           &data[((k * 7 + 1) % vecs) * dim], dim);
    auto t0 = Clock::now();
    for (size_t k = 0; k < calls; ++k) {
        size_t i = k % vecs, j = (k * 7 + 1) % vecs;
        sink += linkpred::cosine_from_ptrs(&data[i * dim], &data[j * dim], dim);
    }
    double ns = ns_since(t0);
    if (sink == 42.0) std::printf(" ");                      // keep sink live
    return ns / (double)calls;
}

int main(int argc, char *argv[])
{
    std::vector<int> threads = {1, 2, 4, 8};
    size_t calls  = 2000000;
    int    rounds = 3;
    std::string wt_cache, out_csv;
    std::vector<char *> rest;
    for (int i = 0; i < argc; ++i) {
        std::string a(argv[i]);
        if      (a.rfind("--calls=", 0) == 0)    calls    = std::stoull(a.substr(8));
        else if (a.rfind("--rounds=", 0) == 0)   rounds   = std::stoi(a.substr(9));
        else if (a.rfind("--wt-cache=", 0) == 0) wt_cache = a.substr(11);
        else if (a.rfind("--out=", 0) == 0)      out_csv  = a.substr(6);
        else if (a.rfind("--threads=", 0) == 0) {    // comma list, e.g. 1,2,4,8,16
            threads.clear();
            std::stringstream ss(a.substr(10));
            for (std::string t; std::getline(ss, t, ',');)
                if (!t.empty() && std::stoi(t) > 0) threads.push_back(std::stoi(t));
        }
        else rest.push_back(argv[i]);
    }
    if (rounds < 1) rounds = 1;
    if (threads.empty()) { std::fprintf(stderr, "ERROR: --threads is empty\n"); return 1; }

    CmdLineApp cli((int)rest.size(), rest.data());
    if (!cli.parse_args()) return -1;
    cmdline_opts opts = cli.get_parsed_opts();
    opts.read_only      = true;
    opts.create_new     = false;
    opts.has_node_props = true;
    opts.prop_mode      = EMBEDDED;
    if (!wt_cache.empty())
        opts.conn_config += (opts.conn_config.empty() ? "" : ",") + ("cache_size=" + wt_cache);

    int max_t = *std::max_element(threads.begin(), threads.end());
    GraphEngine engine(max_t + 1, opts);
    std::string checkpoint = engine.make_checkpoint();
    GraphBase *g = engine.create_ro_graph_handle(checkpoint);
    // One handle per thread, opened one at a time (create_ro_graph_handle writes
    // shared engine state; see linkpred open_handles).
    std::vector<GraphBase *> hs;
    for (int i = 0; i < max_t; ++i) hs.push_back(engine.create_ro_graph_handle(checkpoint));

    // All node ids in DB order, then shuffled (same seed as next_vs_cosine's
    // single_fetch_random).
    std::vector<node_id_t> ids;
    {
        NodeCursor *nc = g->get_node_iter();
        nc->set_key_range({0, OutOfBand_ID_MAX});
        node n;
        nc->next(&n);
        while (n.id != OutOfBand_ID_MAX) { ids.push_back(n.id); nc->next(&n); }
        nc->close();
        delete nc;
    }
    std::shuffle(ids.begin(), ids.end(), std::mt19937(7));

    size_t db_dim = 0;
    if (!ids.empty()) {
        prop_blob pb = g->get_node_properties(ids.front());
        db_dim = pb.size / sizeof(float);
        if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
    }

    // Real embeddings for the cosine: the first (up to) 4096 nodes in DB order.
    std::vector<float> emb;
    if (db_dim > 0) {
        std::vector<node_id_t> first = ids;            // ids is shuffled; take DB order
        std::sort(first.begin(), first.end());
        first.resize(std::min<size_t>(first.size(), 4096));
        for (node_id_t id : first) {
            prop_blob pb = g->get_node_properties(id);
            if (pb.size == db_dim * sizeof(float) && pb.data) {
                const float *f = reinterpret_cast<const float *>(pb.data);
                emb.insert(emb.end(), f, f + db_dim);
            }
            if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
        }
    }

    std::vector<std::vector<double>> par(threads.size());
    std::vector<double> cc;
    for (int r = 0; r < rounds; ++r) {
        for (size_t k = 0; k < threads.size(); ++k)
            par[k].push_back(time_fetch_parallel(hs, threads[k], ids, calls));
        if (db_dim > 0) cc.push_back(time_cosine_compute_only(emb, db_dim, calls));
    }

    std::vector<std::pair<std::string, double>> rows;
    std::string d = std::to_string(db_dim) + "d";
    double cos_c = cc.empty() ? 0.0 : median(cc);
    rows.push_back({"cosine_" + d + "_compute_only", cos_c});
    for (size_t k = 0; k < threads.size(); ++k) {
        std::string T = std::to_string(threads[k]);
        double f = median(par[k]);
        rows.push_back({"par" + T + "_ns_per_fetch", f});
        rows.push_back({"par" + T + "_speedup", median(par[0]) / f});
        if (cos_c > 0) rows.push_back({"par" + T + "/cosine_" + d + "_compute_only", f / cos_c});
    }

    std::printf("graph=%s nodes=%zu db_dim=%zu calls>=%zu rounds=%d wt_cache=%s (median)\n",
                opts.db_name.c_str(), ids.size(), db_dim, calls, rounds,
                wt_cache.empty() ? "default" : wt_cache.c_str());
    std::printf("cosine vectors: DB embeddings (real), %zu nodes\n", emb.size() / (db_dim ? db_dim : 1));
    std::printf("%-36s %10s\n", "Operation", "value");
    for (const auto &[op, v] : rows) std::printf("%-36s %10.2f\n", op.c_str(), v);

    if (!out_csv.empty()) {
        bool fresh = !std::ifstream(out_csv).good();
        std::ofstream out(out_csv, std::ios::app);
        if (fresh)
            out << "timestamp,db_dir,db_name,nodes,db_dim,calls,rounds,wt_cache,operation,value\n";
        char ts[32];
        std::time_t now = std::time(nullptr);
        std::strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        for (const auto &[op, v] : rows)
            out << ts << "," << opts.db_dir << "," << opts.db_name << "," << ids.size()
                << "," << db_dim << "," << calls << "," << rounds << ","
                << (wt_cache.empty() ? "default" : wt_cache) << "," << op << "," << v << "\n";
        std::fprintf(stderr, "appended %zu rows to %s\n", rows.size(), out_csv.c_str());
    }

    for (GraphBase *h : hs) h->close(false);
    g->close(false);
    engine.close_graph();
    return 0;
}
