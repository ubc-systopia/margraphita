// ML access benchmark, revision 4 (2026-11-24: verification modes, all opt-in).
//   --stats        WiredTiger statistics=(fast); per measured phase reports WT pages/bytes read
//                  into its cache and the process's real block-device reads (/proc/self/io
//                  read_bytes) and major faults, per batch, in the last 4 CSV columns
//   --sorted-batch sort the seeds of each batch before running it (locality test)
//   --deg-split    sample: run low-degree and high-degree seeds (split at the median) separately
//   --khop-split   khop: extra rows timing neighbour retrieval and deduplication separately
//   --write-modes  interm (db): extra write rows via a raw WiredTiger cursor on table:node_props
//                  (update only, overwriting insert as Flexograph does, search only, and
//                  64 updates per explicit transaction) and an independent-read row
// Revision 3 behaviour is unchanged without these flags; the CSV gains 4 columns.
// ML access benchmark, revision 3. One backend per process, single thread.
// --backend=db|mem; --access=random|sequential; --batch=B groups independent seeds.
// --seed-file=FILE accepts whitespace-separated unsigned node IDs, validated before timing.
// DB mode with seed file avoids full node scan. Memory mode preloads from DB.
// --ops=vfeat,efeat,sample,khop,interm; vfeat requires --props.
// sample: outgoing uniform sampling with replacement, --fanout=10,5.
// khop: distinct reachable nodes excluding seed, --hops=2.
// efeat: DB only, neighbours vs neighbours plus edge weight.
// interm: --dim=64; DB writes require --allow-write and a scratch DB.
// Usage: ml_access_bench -p DIR -m NAME -g adj -r [-d]
//   [--backend=db] [--access=random] [--batch=1] [--seed-file=FILE]
//   [--queries=2000] [--warmup=200] [--seed=7] [--wt-cache=SIZE] [--out=FILE]
// queries and warmup count batches; latency is per batch.
// avg_items_per_batch: bytes (vfeat), neighbours/samples/nodes, or vectors (interm).
// items_per_sec uses those same operation-specific units.
// process_peak_rss_kib is cumulative Linux process high-water RSS, not per-op memory.
// CSV header must match exactly; use a new output file for this revision.
// CSV: op,variant,db,params,p50_us,p95_us,p99_us,max_us,batches_per_sec,n_batches,avg_items_per_batch,preload_ms,batch,access,seed,wt_cache,process_peak_rss_kib,seed_queries_per_sec,items_per_sec,wt_pages_read_per_batch,wt_bytes_read_per_batch,os_read_bytes_per_batch,major_faults_per_batch

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cstdlib>
#include <sys/resource.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "../ldbc/bench_harness.h"
#include "command_line.h"
#include "common_defs.h"
#include "graph_engine.h"
#include "common_util.h"

// percentiles from explicit samples (ms), for rows timed inside a query
static BenchResult samples_result(std::vector<double> v)
{
    BenchResult r; if (v.empty()) return r;
    std::sort(v.begin(), v.end()); size_t n = v.size(); double tot = 0; for (double x : v) tot += x;
    r.n_measured = (int64_t)n; r.p50_ms = v[n * 50 / 100]; r.p95_ms = v[n * 95 / 100];
    r.p99_ms = v[n * 99 / 100]; r.max_ms = v.back();
    r.throughput_ops_per_sec = tot > 0 ? n / (tot / 1000.0) : 0; return r;
}
static long long proc_read_bytes()
{
    std::ifstream f("/proc/self/io"); std::string k; long long v;
    while (f >> k >> v) if (k == "read_bytes:") return v;
    return -1;
}
static long long wt_stat(WT_SESSION *s, int key)
{
    if (!s) return -1;
    WT_CURSOR *c = nullptr;
    if (s->open_cursor(s, "statistics:", nullptr, nullptr, &c) != 0) return -1;
    c->set_key(c, key); long long v = -1;
    if (c->search(c) == 0) { const char *d, *pv; int64_t val; c->get_value(c, &d, &pv, &val); v = val; }
    c->close(c); return v;
}

static std::vector<int> parse_ints(const std::string &s)
{
    std::vector<int> v; std::stringstream ss(s);
    for (std::string t; std::getline(ss, t, ',');) if (!t.empty()) v.push_back(std::stoi(t));
    return v;
}

struct Csr { std::vector<uint64_t> off; std::vector<node_id_t> nbr; std::vector<node_id_t> ids; };
static size_t slot_of(const Csr &c, node_id_t id)   // ids sorted
{
    auto it = std::lower_bound(c.ids.begin(), c.ids.end(), id);
    return (it != c.ids.end() && *it == id) ? (size_t)(it - c.ids.begin()) : SIZE_MAX;
}

// one adjacency list through the DB (as next_vs_cosine's random read)
static bool db_nbrs(OutCursor *oc, node_id_t u, adjlist &a)
{
    a.edgelist.clear(); a.node_id = OutOfBand_ID_MAX;
    oc->reset(); oc->set_key_range({u, u}); oc->next(&a);
    return a.node_id == u;
}

int main(int argc, char *argv[])
{
    std::string backend = "db", access = "random", seed_file;
    int batch = 1;
    std::string ops_s = "vfeat,efeat,sample,khop", out_csv, wt_cache, fanout_s = "10,5";
    int queries = 2000, warmup = 200, hops = 2, dim = 64; unsigned seed = 7;
    bool props = false, allow_write = false, stats = false, sorted_batch = false, deg_split = false, khop_split = false, write_modes = false;
    std::vector<char *> rest;
    for (int i = 0; i < argc; ++i) {
        std::string a(argv[i]);
        if (a.rfind("--backend=", 0) == 0) backend = a.substr(10);
        else if (a.rfind("--access=", 0) == 0) access = a.substr(9);
        else if (a.rfind("--seed-file=", 0) == 0) seed_file = a.substr(12);
        else if (a.rfind("--batch=", 0) == 0) batch = std::stoi(a.substr(8));
        else if (a.rfind("--ops=", 0) == 0)      ops_s = a.substr(6);
        else if (a.rfind("--queries=", 0) == 0)  queries = std::stoi(a.substr(10));
        else if (a.rfind("--warmup=", 0) == 0)   warmup = std::stoi(a.substr(9));
        else if (a.rfind("--fanout=", 0) == 0)   fanout_s = a.substr(9);
        else if (a.rfind("--hops=", 0) == 0)     hops = std::stoi(a.substr(7));
        else if (a.rfind("--dim=", 0) == 0)      dim = std::stoi(a.substr(6));
        else if (a.rfind("--seed=", 0) == 0)     seed = (unsigned)std::stoul(a.substr(7));
        else if (a.rfind("--out=", 0) == 0)      out_csv = a.substr(6);
        else if (a.rfind("--wt-cache=", 0) == 0) wt_cache = a.substr(11);
        else if (a == "--props")                 props = true;
        else if (a == "--allow-write")           allow_write = true;
        else if (a == "--stats")                 stats = true;
        else if (a == "--sorted-batch")          sorted_batch = true;
        else if (a == "--deg-split")             deg_split = true;
        else if (a == "--khop-split")            khop_split = true;
        else if (a == "--write-modes")           write_modes = true;
        else rest.push_back(argv[i]);
    }
    std::vector<int> fanout = parse_ints(fanout_s);
    if ((backend != "db" && backend != "mem") ||
        (access != "random" && access != "sequential") || queries <= 0 || warmup < 0 ||
        batch <= 0 || dim <= 0 || hops < 0 || fanout.empty() ||
        std::any_of(fanout.begin(), fanout.end(), [](int k){return k < 0;})) {
        std::fprintf(stderr, "Invalid benchmark parameters\n"); return 2;
    }
    const bool db = backend == "db";
    std::unordered_set<std::string> ops;
    { std::stringstream ss(ops_s); for (std::string t; std::getline(ss, t, ',');) ops.insert(t); }
    if (db && ops.count("interm") && !allow_write) {
        std::fprintf(stderr, "interm writes node properties: pass --allow-write and use a scratch DB copy\n");
        return 2;
    }
    if (ops.count("vfeat") && !props) { std::fprintf(stderr, "vfeat needs --props (a DB with node embeddings)\n"); return 2; }

    CmdLineApp cli((int)rest.size(), rest.data());
    if (!cli.parse_args()) return -1;
    cmdline_opts opts = cli.get_parsed_opts();
    opts.create_new = false;
    opts.read_only  = !db || !ops.count("interm");
    if (!wt_cache.empty())
        opts.conn_config += (opts.conn_config.empty() ? "" : ",") + ("cache_size=" + wt_cache);
    if (stats)
        opts.conn_config += (opts.conn_config.empty() ? "" : ",") + std::string("statistics=(fast)");
    if (props || ops.count("interm")) { opts.has_node_props = true; opts.prop_mode = EMBEDDED; }

    GraphEngine engine(1, opts);
    GraphBase *g;
    std::string checkpoint;
    if (opts.read_only) { checkpoint = engine.make_checkpoint(); g = engine.create_ro_graph_handle(checkpoint); }
    else g = engine.create_graph_handle();

    // ---- one pass: node ids + in-memory CSR (the "mem" variant's preload) ----
    Csr csr;
    auto t0 = std::chrono::steady_clock::now();
    if (!db || seed_file.empty()) {
        NodeCursor *nc = g->get_node_iter();
        nc->set_key_range({0, OutOfBand_ID_MAX});
        node n; nc->next(&n);
        while (n.id != OutOfBand_ID_MAX) { csr.ids.push_back(n.id); nc->next(&n); }
        nc->close(); delete nc;
        std::sort(csr.ids.begin(), csr.ids.end());
        if (!db && (ops.count("sample") || ops.count("khop"))) {
        csr.off.assign(csr.ids.size() + 1, 0);
        std::vector<std::vector<node_id_t>> tmp(csr.ids.size());
        OutCursor *oc = g->get_outnbd_iter();
        oc->set_key_range({0, OutOfBand_ID_MAX});
        adjlist a; oc->next(&a);
        while (a.node_id != OutOfBand_ID_MAX) {
            size_t s = slot_of(csr, a.node_id);
            if (s != SIZE_MAX) tmp[s] = a.edgelist;
            a.edgelist.clear(); oc->next(&a);
        }
        oc->close(); delete oc;
        for (size_t i = 0; i < tmp.size(); ++i) csr.off[i + 1] = csr.off[i] + tmp[i].size();
        csr.nbr.reserve(csr.off.back());
        for (auto &v : tmp) { csr.nbr.insert(csr.nbr.end(), v.begin(), v.end()); std::vector<node_id_t>().swap(v); }
        }
    }
    double preload_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::vector<node_id_t> seeds;
    if (!seed_file.empty()) {
        std::ifstream in(seed_file);
        std::string token;
        if (!in.is_open()) { std::fprintf(stderr, "Cannot open seed file\n"); return 2; }
        while (in >> token) {
            unsigned long long raw;
            try {
                if (token.empty() || token.find_first_not_of("0123456789") != std::string::npos)
                    throw std::invalid_argument("seed");
                raw = std::stoull(token);
                if (raw >= OutOfBand_ID_MAX) throw std::out_of_range("seed");
            } catch (const std::exception &) {
                std::fprintf(stderr, "Invalid seed ID: %s\n", token.c_str()); return 2;
            }
            node_id_t id = static_cast<node_id_t>(raw);
            if (db ? !g->has_node(id) : slot_of(csr, id) == SIZE_MAX) {
                std::fprintf(stderr, "Unknown seed ID: %s\n", token.c_str()); return 2;
            }
            seeds.push_back(id);
        }
        if (!in.eof() || in.bad() || seeds.empty()) { std::fprintf(stderr, "Invalid seed file\n"); return 2; }
    } else {
        seeds = csr.ids;
    }
    if (seeds.empty()) { std::fprintf(stderr, "empty seed list\n"); return 1; }
    std::mt19937_64 rng(seed);
    if (access == "random") std::shuffle(seeds.begin(), seeds.end(), rng);
    else std::sort(seeds.begin(), seeds.end());
    if (db) std::vector<node_id_t>().swap(csr.ids);
    size_t qi = 0;
    std::vector<node_id_t> bq; size_t bi = 0;           // the current batch's seeds
    auto raw_seed = [&]() { node_id_t u = seeds[qi]; qi = (qi + 1) % seeds.size(); return u; };
    auto next_seed = [&]() { return bi < bq.size() ? bq[bi++] : raw_seed(); };
    WT_SESSION *stat_s = nullptr;
    if (stats) engine.get_connection()->open_session(engine.get_connection(), nullptr, nullptr, &stat_s);
    std::fprintf(stderr, "backend=%s seeds=%zu CSR edges=%zu setup_ms=%.1f\n",
                 backend.c_str(), seeds.size(), csr.nbr.size(), preload_ms);

    OutCursor *oc = g->get_outnbd_iter();
    adjlist a;
    const char *csv_header = "op,variant,db,params,p50_us,p95_us,p99_us,max_us,batches_per_sec,n_batches,avg_items_per_batch,preload_ms,batch,access,seed,wt_cache,process_peak_rss_kib,seed_queries_per_sec,items_per_sec,wt_pages_read_per_batch,wt_bytes_read_per_batch,os_read_bytes_per_batch,major_faults_per_batch";
    FILE *out = nullptr;
    if (!out_csv.empty()) {
        std::ifstream probe(out_csv);
        bool fresh = !probe.good() || probe.peek() == EOF;
        if (!fresh) {
            std::string existing;
            std::getline(probe, existing);
            if (!existing.empty() && existing.back() == '\r') existing.pop_back();
            if (existing != csv_header) {
                std::fprintf(stderr, "CSV header mismatch: %s; use a new output file\n", out_csv.c_str()); return 2;
            }
        }
        probe.close();
        out = std::fopen(out_csv.c_str(), "a");
        if (!out) { std::perror(out_csv.c_str()); return 2; }
        if (fresh) std::fprintf(out, "%s\n", csv_header);
    }
    std::printf("%s\n", csv_header);
    double st_pages = -1, st_bytes = -1, st_os = -1, st_flt = -1;   // per batch, last timed phase
    auto emit = [&](const char *op, const char *var, std::string params, const BenchResult &r, double items) {
        if (sorted_batch) params += ";sorted_batch";
        char line[1280];
        struct rusage usage{}; getrusage(RUSAGE_SELF, &usage);
        std::snprintf(line, sizeof line, "%s,%s,%s,\"%s\",%.3f,%.3f,%.3f,%.3f,%.1f,%lld,%.2f,%.1f,%d,%s,%u,%s,%ld,%.1f,%.1f\n", op, var,
                      opts.db_name.c_str(), params.c_str(), r.p50_ms * 1e3, r.p95_ms * 1e3, r.p99_ms * 1e3,
                      r.max_ms * 1e3, r.throughput_ops_per_sec, (long long)r.n_measured, items, preload_ms, batch, access.c_str(), seed, wt_cache.c_str(), usage.ru_maxrss, r.throughput_ops_per_sec * batch, r.throughput_ops_per_sec * items);
        size_t L = std::strlen(line); if (L && line[L - 1] == '\n') line[--L] = 0;
        std::snprintf(line + L, sizeof line - L, ",%.2f,%.1f,%.1f,%.3f\n", st_pages, st_bytes, st_os, st_flt);
        std::fputs(line, stdout); if (out) { std::fputs(line, out); std::fflush(out); }
    };
    double items = 0; size_t sink = 0;
    auto timed = [&](auto fn, size_t start = 0) {
        items = 0; qi = start % seeds.size(); rng.seed(seed);
        auto query = [&] {
            bq.clear(); for (int i = 0; i < batch; ++i) bq.push_back(raw_seed());
            if (sorted_batch) std::sort(bq.begin(), bq.end());
            bi = 0; for (int i = 0; i < batch; ++i) fn();
            bq.clear(); bi = 0;
        };
        for (int i = 0; i < warmup; ++i) query();
        items = 0;
        long long p0 = wt_stat(stat_s, WT_STAT_CONN_CACHE_READ), b0 = wt_stat(stat_s, WT_STAT_CONN_CACHE_BYTES_READ),
                  o0 = proc_read_bytes();
        struct rusage u0{}; getrusage(RUSAGE_SELF, &u0);
        BenchResult r = run_timed(query, 0, queries);
        long long p1 = wt_stat(stat_s, WT_STAT_CONN_CACHE_READ), b1 = wt_stat(stat_s, WT_STAT_CONN_CACHE_BYTES_READ),
                  o1 = proc_read_bytes();
        struct rusage u1{}; getrusage(RUSAGE_SELF, &u1);
        st_pages = (stats && p0 >= 0) ? (double)(p1 - p0) / queries : -1;
        st_bytes = (stats && b0 >= 0) ? (double)(b1 - b0) / queries : -1;
        st_os    = (o0 >= 0) ? (double)(o1 - o0) / queries : -1;
        st_flt   = (double)(u1.ru_majflt - u0.ru_majflt) / queries;
        items /= (double)queries; return r;
    };

    // ---- vertex features ----
    if (ops.count("vfeat")) {
        std::vector<std::vector<uint8_t>> mem;
        auto prep = std::chrono::steady_clock::now();
        if (!db) mem.resize(csr.ids.size());
        for (size_t i = 0; i < mem.size(); ++i) {
            prop_blob pb = g->get_node_properties(csr.ids[i]);
            if (pb.data) { mem[i].assign(pb.data, pb.data + pb.size); delete[] const_cast<uint8_t *>(pb.data); }
        }
        std::vector<uint8_t> buf(1 << 16);
        if (!db) preload_ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now()-prep).count();
        if (db) {
        auto r_db = timed([&] { prop_blob pb = g->get_node_properties(next_seed());
                                if (pb.data) { std::memcpy(buf.data(), pb.data, std::min(pb.size, buf.size()));
                                               delete[] const_cast<uint8_t *>(pb.data); }
                                items += pb.size; });
        emit("vfeat", "db", "get_node_properties", r_db, items);
        } else {
        auto r_mem = timed([&] { const auto &v = mem[slot_of(csr, next_seed())];
                                 std::memcpy(buf.data(), v.data(), std::min(v.size(), buf.size())); items += v.size(); });
        emit("vfeat", "mem", "vector copy", r_mem, items);
        }
    }
    // ---- edge features ----
    if (db && ops.count("efeat")) {
        auto r_n = timed([&] { if (db_nbrs(oc, next_seed(), a)) items += a.edgelist.size(); });
        emit("efeat", "db", "nbrs_only", r_n, items);
        auto r_e = timed([&] { node_id_t u = next_seed();
                               if (db_nbrs(oc, u, a)) for (node_id_t v : a.edgelist) {
                                   edge e = g->get_edge(u, v); sink += (size_t)e.edge_weight; items += 1; } });
        emit("efeat", "db", "nbrs+get_edge", r_e, items);
    }
    // ---- neighbour sampling ----
    if (ops.count("sample")) {
        std::string fp = "fanout=" + fanout_s;
        auto sample_db = [&] {
            std::vector<node_id_t> frontier{next_seed()}, nxt;
            for (int k : fanout) {
                nxt.clear();
                for (node_id_t u : frontier)
                    if (db_nbrs(oc, u, a) && !a.edgelist.empty()) {
                        std::uniform_int_distribution<size_t> d(0, a.edgelist.size() - 1);
                        for (int j = 0; j < k; ++j) nxt.push_back(a.edgelist[d(rng)]);
                    }
                items += nxt.size(); frontier.swap(nxt);
            }
        };
        auto sample_mem = [&] {
            std::vector<node_id_t> frontier{next_seed()}, nxt;
            for (int k : fanout) {
                nxt.clear();
                for (node_id_t u : frontier) {
                    size_t s = slot_of(csr, u); if (s == SIZE_MAX) continue;
                    uint64_t lo = csr.off[s], deg = csr.off[s + 1] - lo; if (!deg) continue;
                    std::uniform_int_distribution<uint64_t> d(0, deg - 1);
                    for (int j = 0; j < k; ++j) nxt.push_back(csr.nbr[lo + d(rng)]);
                }
                items += nxt.size(); frontier.swap(nxt);
            }
        };
        if (db) { auto r = timed(sample_db); emit("sample", "db", fp, r, items); }
        else { auto r = timed(sample_mem); emit("sample", "mem", fp, r, items); }
        if (deg_split) {
            std::vector<std::pair<uint64_t, node_id_t>> dv;
            for (node_id_t u : seeds) {
                uint64_t d = 0;
                if (db) d = g->get_out_degree(u);
                else { size_t s2 = slot_of(csr, u); if (s2 != SIZE_MAX) d = csr.off[s2 + 1] - csr.off[s2]; }
                dv.push_back({d, u});
            }
            std::vector<uint64_t> ds; for (auto &p2 : dv) ds.push_back(p2.first);
            std::nth_element(ds.begin(), ds.begin() + ds.size() / 2, ds.end());
            uint64_t med = ds[ds.size() / 2];
            std::vector<node_id_t> all = seeds, lo, hi;
            for (auto &p2 : dv) (p2.first <= med ? lo : hi).push_back(p2.second);
            for (int part = 0; part < 2; ++part) {
                seeds = part ? hi : lo; if (seeds.empty()) continue;
                std::string pp = fp + (part ? ";deg>" : ";deg<=") + std::to_string(med);
                if (db) { auto r = timed(sample_db); emit("sample", "db", pp, r, items); }
                else { auto r = timed(sample_mem); emit("sample", "mem", pp, r, items); }
            }
            seeds = all;
        }
    }
    // ---- multi-hop retrieval ----
    if (ops.count("khop")) {
        std::string hp = "hops=" + std::to_string(hops);
        auto khop = [&](bool use_db) {
            std::unordered_set<node_id_t> seen; std::vector<node_id_t> frontier{next_seed()}, nxt;
            seen.insert(frontier[0]);
            for (int h = 0; h < hops; ++h) {
                nxt.clear();
                for (node_id_t u : frontier) {
                    if (use_db) { if (!db_nbrs(oc, u, a)) continue;
                                  for (node_id_t v : a.edgelist) if (seen.insert(v).second) nxt.push_back(v); }
                    else { size_t s = slot_of(csr, u); if (s == SIZE_MAX) continue;
                           for (uint64_t i = csr.off[s]; i < csr.off[s + 1]; ++i)
                               if (seen.insert(csr.nbr[i]).second) nxt.push_back(csr.nbr[i]); }
                }
                frontier.swap(nxt);
            }
            items += seen.size() - 1;
        };
        if (db) { auto r = timed([&] { khop(true); }); emit("khop", "db", hp, r, items); }
        else { auto r = timed([&] { khop(false); }); emit("khop", "mem", hp, r, items); }
        if (khop_split) {   // same work, with neighbour retrieval and dedup timed separately per query
            std::vector<double> t_ret, t_dd; std::vector<node_id_t> tmp;
            auto khop2 = [&] {
                double ret = 0, dd = 0; std::unordered_set<node_id_t> seen;
                std::vector<node_id_t> frontier{next_seed()}, nxt; seen.insert(frontier[0]);
                for (int h = 0; h < hops; ++h) {
                    nxt.clear();
                    for (node_id_t u : frontier) {
                        auto c0 = std::chrono::steady_clock::now();
                        tmp.clear();
                        if (db) { if (db_nbrs(oc, u, a)) tmp.assign(a.edgelist.begin(), a.edgelist.end()); }
                        else { size_t s2 = slot_of(csr, u);
                               if (s2 != SIZE_MAX) tmp.assign(csr.nbr.begin() + csr.off[s2], csr.nbr.begin() + csr.off[s2 + 1]); }
                        auto c1 = std::chrono::steady_clock::now();
                        for (node_id_t v : tmp) if (seen.insert(v).second) nxt.push_back(v);
                        auto c2 = std::chrono::steady_clock::now();
                        ret += std::chrono::duration<double, std::milli>(c1 - c0).count();
                        dd  += std::chrono::duration<double, std::milli>(c2 - c1).count();
                    }
                    frontier.swap(nxt);
                }
                t_ret.push_back(ret); t_dd.push_back(dd); items += seen.size() - 1;
            };
            timed(khop2); t_ret.erase(t_ret.begin(), t_ret.end() - std::min(t_ret.size(), (size_t)queries * batch));
            t_dd.erase(t_dd.begin(), t_dd.end() - std::min(t_dd.size(), (size_t)queries * batch));
            emit("khop", db ? "db" : "mem", hp + ";part=retrieval", samples_result(t_ret), items);
            emit("khop", db ? "db" : "mem", hp + ";part=dedup", samples_result(t_dd), items);
        }
    }
    // ---- temporary representations ----
    if (ops.count("interm")) {
        std::string dp = "dim=" + std::to_string(dim);
        std::vector<float> vec(dim, 0.5f), mem(db ? 0 : (size_t)csr.ids.size() * dim, 0.0f), rd(dim);
        if (db) {
        auto r_w = timed([&] { g->set_node_properties(next_seed(), reinterpret_cast<const uint8_t *>(vec.data()),
                                                     vec.size() * sizeof(float)); items += 1; });
        emit("interm", "db", dp + ";write", r_w, items);
        auto r_r = timed([&] { prop_blob pb = g->get_node_properties(next_seed());
                               if (pb.data) { std::memcpy(rd.data(), pb.data, std::min(pb.size, rd.size() * sizeof(float)));
                                              delete[] const_cast<uint8_t *>(pb.data); } items += 1; });
        emit("interm", "db", dp + ";read", r_r, items);
        if (write_modes) {
            size_t half = seeds.size() / 2;   // nodes the write runs above did not touch (for 1M-node graphs)
            auto r_ri = timed([&] { prop_blob pb = g->get_node_properties(next_seed());
                               if (pb.data) { std::memcpy(rd.data(), pb.data, std::min(pb.size, rd.size() * sizeof(float)));
                                              delete[] const_cast<uint8_t *>(pb.data); } items += 1; }, half);
            emit("interm", "db", dp + ";read_independent", r_ri, items);
            WT_CONNECTION *wc = engine.get_connection(); WT_SESSION *ws = nullptr; WT_CURSOR *wcur = nullptr, *icur = nullptr;
            if (wc->open_session(wc, nullptr, nullptr, &ws) == 0 &&
                ws->open_cursor(ws, "table:node_props", nullptr, "overwrite=true", &wcur) == 0 &&
                ws->open_cursor(ws, "table:node_props", nullptr, nullptr, &icur) == 0) {   // default config, as AdjList opens node_props_cursor
                WT_ITEM item; item.data = vec.data(); item.size = vec.size() * sizeof(float);
                auto r_u = timed([&] { CommonUtil::set_key(wcur, next_seed()); wcur->set_value(wcur, &item);
                                       wcur->update(wcur); items += 1; });
                emit("interm", "db", dp + ";write_wt_update_only", r_u, items);
                auto r_iu = timed([&] { CommonUtil::set_key(icur, next_seed()); icur->set_value(icur, &item);
                                        icur->insert(icur); items += 1; });   // overwrite=true: the same call set_node_properties makes
                emit("interm", "db", dp + ";write_wt_insert_overwrite", r_iu, items);
                auto r_s = timed([&] { CommonUtil::set_key(wcur, next_seed()); wcur->search(wcur); wcur->reset(wcur); items += 1; });
                emit("interm", "db", dp + ";wt_search_only", r_s, items);
                std::vector<double> tx; size_t txq = (size_t)queries / 8 + 1; qi = 0;
                for (size_t q = 0; q < txq; ++q) {
                    auto c0 = std::chrono::steady_clock::now();
                    ws->begin_transaction(ws, nullptr);
                    for (int j = 0; j < 64; ++j) { CommonUtil::set_key(wcur, raw_seed()); wcur->set_value(wcur, &item); wcur->update(wcur); }
                    ws->commit_transaction(ws, nullptr);
                    tx.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - c0).count());
                }
                emit("interm", "db", dp + ";write_txn64_per_txn", samples_result(tx), 64);
            }
            if (ws) ws->close(ws, nullptr);
        }
        } else {
        auto r_mw = timed([&] { std::memcpy(&mem[slot_of(csr, next_seed()) * dim], vec.data(), dim * sizeof(float)); items += 1; });
        emit("interm", "mem", dp + ";write", r_mw, items);
        auto r_mr = timed([&] { std::memcpy(rd.data(), &mem[slot_of(csr, next_seed()) * dim], dim * sizeof(float)); items += 1; });
        emit("interm", "mem", dp + ";read", r_mr, items);
        }
    }
    if (!db && ops.count("efeat")) std::fprintf(stderr, "efeat: memory variant not implemented; skipped\n");
    if (sink == 42) std::printf(" ");
    oc->close(); delete oc;
    if (out) std::fclose(out);
    return 0;
}
