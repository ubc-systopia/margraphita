// next_vs_cosine.cpp — is the DB fetch or the score the dominant per-item cost?
//
// RENAMED for clarity (2026-11-03). Operation names now follow
//   <cursor>_<action>_<variant>
// so it's obvious which cursor is used and what "fetch" means in each case.
//
// Measures, in ns per call:
//   node_cursor_next               NodeCursor::next() — sequential scan of nodes,
//                                  returns node metadata (id, in_deg, out_deg)
//   node_prop_fetch                get_node_properties(id), ids in DB order
//                                  (direct point lookup, NOT a cursor; --props only)
//   node_prop_fetch_random         same, ids shuffled (random point lookups)
//
//   in_cursor_next_seq             InCursor::next() averaged over a full pass;
//                                  the returned adjlist is walked via edge_sink
//   in_cursor_next_rand            reset + set_key_range({id,id}) + InCursor::next()
//                                  per random id; the adjlist is walked via edge_sink
//   in/out_cursor_next_rand_hit_pct  % of those reads that returned the requested
//                                  node (rest: no neighbours on that cursor)
//   in_cursor_next_rand_dL_H       same, restricted to nodes with in_degree in [L,H)
//   out_cursor_next_seq            OutCursor::next(), analogously
//   out_cursor_next_rand           OutCursor::next() with random ids
//   out_cursor_next_rand_dL_H      stratified by out_degree bucket
//
//   cosine_<D>d_seq                linkpred::cosine_from_ptrs, one in-memory D-d
//                                  vector per graph node, DB order
//   cosine_<D>d_random             same vectors, shuffled (same shuffle as the
//                                  random DB ops)
//   cosine_<D>d_compute_only       --vecs vectors kept in CPU cache: arithmetic
//                                  cost alone (a scorer reading a prefetch buffer)
//
// Two kinds of ratio, for two questions:
//   ratio_<op>/cosine_<D>d_{seq,random}      DB vs in-memory (pvector): each DB
//       op over the cosine with the same access pattern and data size
//   prefetch_<op>/cosine_<D>d_compute_only   can prefetching hide the fetch?
//       < 1 one scorer outlasts the fetch; > 1 need ~that many fetches in
//       flight (whether that scales: fetch_random_parallel.cpp)
//
// WHY THE CURSOR LOOPS ITERATE a.edgelist:
//   next() returns a pointer to the neighbor list, but the cost of CONSUMING
//   that list (walking every neighbor) is the operation the FoF/sampling
//   workloads actually pay. Without the walk, next() costs the same regardless
//   of degree — ~60 ns — and the degree stratification collapses to a flat line.
//   edge_sink accumulates a real use of each neighbor id so the compiler cannot
//   elide the loop; the sum is never read (a 'if (sink == 42)' guard keeps it
//   live for the optimizer). This mirrors what time_fetch already does with its
//   own sink.
//
// NOTE ON CURSOR METHOD NAMES:
//   graph->get_node_iter()   (NodeCursor)
//   graph->get_innbd_iter()  (InCursor,  verified from existing code)
//   graph->get_outnbd_iter() (OutCursor, by analogy — adjust if your codebase
//                             names it get_outnb_d_iter / get_outnbid / etc.)
//
// Reading the DB-vs-memory ratio (any DB op / cosine):
//   < 5    fetch is cheap   -> prefetching won't help much
//   5..20  prefetch might help
//   > 20   fetch dominates  -> prefetching is essential
//
// Usage:
//   next_vs_cosine -p <db_dir> -m <db_name> -g adj -r
//                  [--props] [--dims=64,128] [--out=FILE.csv]
//                  [--calls=N] [--rounds=R] [--vecs=V]
//   --props   open node_props (EMBEDDED): measure node_prop_fetch, probe the DB dim
//   --dims    extra cosine dims to time besides the DB's (default: none; 128 if
//             neither --props nor --dims is given)
//   --out     append one CSV row per reported value (long format)
//   --vecs    vector count for cosine_<D>d_compute_only only (default 4096)
//   --wt-cache  WiredTiger cache size, e.g. 16MB (default: WiredTiger's own);
//             drop the OS page cache too for a cold run
//
// See next_vs_cosine.md for results.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "command_line.h"
#include "common_defs.h"
#include "graph_engine.h"
#include "linkpred_scores.h"   // linkpred::cosine_from_ptrs

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

// ---------------- NodeCursor + property fetch ----------------

static size_t node_pass(NodeCursor *nc, std::vector<node_id_t> *ids)
{
    nc->reset();
    nc->set_key_range({0, OutOfBand_ID_MAX});
    size_t calls = 0;
    node n;
    nc->next(&n);
    ++calls;
    while (n.id != OutOfBand_ID_MAX) {
        if (ids) ids->push_back(n.id);
        nc->next(&n);
        ++calls;
    }
    return calls;
}

static double time_node_cursor_next(GraphBase *g, size_t min_calls)
{
    NodeCursor *nc = g->get_node_iter();
    node_pass(nc, nullptr);
    size_t calls = 0;
    auto t0 = Clock::now();
    while (calls < min_calls) calls += node_pass(nc, nullptr);
    double ns = ns_since(t0);
    nc->close();
    delete nc;
    return ns / (double)calls;
}

static double time_node_prop_fetch(GraphBase *g, const std::vector<node_id_t> &ids,
                                   size_t min_calls, size_t &blob_bytes)
{
    size_t sink = 0;
    for (node_id_t id : ids) {
        prop_blob pb = g->get_node_properties(id);
        sink += pb.size;
        if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
    }
    blob_bytes = ids.empty() ? 0 : sink / ids.size();
    size_t calls = 0;
    auto t0 = Clock::now();
    while (calls < min_calls) {
        for (node_id_t id : ids) {
            prop_blob pb = g->get_node_properties(id);
            sink += pb.size;
            if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
        }
        calls += ids.size();
    }
    double ns = ns_since(t0);
    if (sink == 42) std::printf(" ");    // keep sink live
    return ns / (double)calls;
}

// Cosine timings, three variants so each DB op has a like-for-like partner:
//   compute_only  `vecs` vectors (default 4096) kept in CPU cache, fixed stride:
//                 the arithmetic cost alone, i.e. what a scorer reading from a
//                 prefetch buffer pays. Used only in the prefetch_* rows.
//   seq           one vector per graph node (same data volume as the DB rows),
//                 DB order: call k scores (u, v), v = vec[k % n] new every call,
//                 u = vec[(k / 64) % n] reused for 64 calls (like an outer-loop u).
//                 Partner of the in-order DB ops.
//   random        same vectors, same shuffle as node_prop_fetch_random.
//                 Partner of the random DB ops.
// A fixed small set would sit in cache and make the cosine look cheaper than in
// a real run (s20, 128-d: the real set is 512 MB).
//
// The vectors are the REAL embeddings stored in this DB (read once, untimed,
// with get_node_properties), so the cosine scores the same data the fetch rows
// read. Only --dims extras that differ from the DB's dim use random vectors;
// their rows are marked _synthetic.
// `src` holds the per-node vectors; the first `vecs` of them are copied into a
// small buffer that stays in cache.
static double time_cosine_compute_only(const std::vector<float> &src, size_t dim,
                                       size_t vecs, size_t calls)
{
    vecs = std::min(vecs, src.size() / dim);
    std::vector<float> data(src.begin(), src.begin() + vecs * dim);
    double sink = 0.0;
    for (size_t k = 0; k < vecs; ++k)
        sink += linkpred::cosine_from_ptrs(&data[k * dim],
                                           &data[((k * 7 + 1) % vecs) * dim], dim);
    auto t0 = Clock::now();
    for (size_t k = 0; k < calls; ++k) {
        size_t i = k % vecs, j = (k * 7 + 1) % vecs;
        sink += linkpred::cosine_from_ptrs(&data[i * dim], &data[j * dim], dim);
    }
    double ns = ns_since(t0);
    if (sink == 42.0) std::printf(" ");  // keep sink live
    return ns / (double)calls;
}

// `data` holds n vectors of `dim` floats; `order` is the visit order
// (identity = seq, shuffled = random).
static double time_cosine_over(const std::vector<float> &data, size_t dim,
                               const std::vector<uint32_t> &order, size_t calls)
{
    const size_t n = order.size();
    double sink = 0.0;
    for (size_t k = 0; k < n; ++k)                  // warm-up pass, untimed
        sink += linkpred::cosine_from_ptrs(&data[(size_t)order[(k / 64) % n] * dim],
                                           &data[(size_t)order[k] * dim], dim);
    auto t0 = Clock::now();
    for (size_t k = 0; k < calls; ++k) {
        size_t u = order[(k / 64) % n], v = order[k % n];
        sink += linkpred::cosine_from_ptrs(&data[u * dim], &data[v * dim], dim);
    }
    double ns = ns_since(t0);
    if (sink == 42.0) std::printf(" ");  // keep sink live
    return ns / (double)calls;
}

// ---------------- InCursor / OutCursor (adjlist) ----------------

// Full sequential pass over an adjlist cursor. Optional degree collection on
// the untimed warm-up pass. The timed loop walks a.edgelist via edge_sink so
// the measured cost includes consuming the neighbor list (this is what the
// degree stratification is trying to expose).
template <typename CursorT>
static double time_adjlist_seq(CursorT *cursor, size_t min_calls,
                               std::vector<std::pair<node_id_t, size_t>> *degrees)
{
    adjlist a{0, 0};

    // Warm-up pass (+ optional degree collection; untimed)
    cursor->reset();
    cursor->set_key_range({0, OutOfBand_ID_MAX});
    cursor->next(&a);
    while (a.node_id != OutOfBand_ID_MAX) {
        if (degrees) {
            size_t d = 0;
            for (node_id_t v : a.edgelist) { (void)v; ++d; }
            degrees->push_back({a.node_id, d});
        }
        cursor->next(&a);
    }

    uint64_t edge_sink = 0;
    size_t calls = 0;
    auto t0 = Clock::now();
    while (calls < min_calls) {
        cursor->reset();
        cursor->set_key_range({0, OutOfBand_ID_MAX});
        cursor->next(&a);
        ++calls;
        while (a.node_id != OutOfBand_ID_MAX) {
            for (node_id_t v : a.edgelist) { edge_sink += v; }   // walk the list
            cursor->next(&a);
            ++calls;
        }
    }
    double ns = ns_since(t0);
    if (edge_sink == 42) std::printf(" ");    // keep edge_sink live
    return ns / (double)calls;
}

// Random access: for each id, reset the cursor, set the range to exactly [id, id]
// and read one record, then walk the returned neighbor list.
//
// reset() is required: AdjOut/InCursor::set_key_range does not clear has_next,
// so once a read hits the table end or a skipped node, every later next() would
// return "no record" at almost no cost. Without it ~26% of reads on s12 were such
// no-ops and the random rows were ~1/3 too low. (Same rule as linkpred: never
// drop reset().) The range is [id, id], not [id, id+1], so a node with no
// neighbours returns nothing instead of the next node's list.
//
// `hits` counts reads that returned the requested id; the rest are nodes the
// cursor skips because they have no neighbours (reported as *_hit_pct).
template <typename CursorT>
static double time_adjlist_rand(CursorT *cursor, const std::vector<node_id_t> &ids,
                                size_t min_calls, double *hit_pct = nullptr)
{
    if (ids.empty()) return 0.0;
    adjlist a{0, 0};

    cursor->reset();
    cursor->set_key_range({ids[0], ids[0]});
    cursor->next(&a);
    if (a.node_id != ids[0]) {
        std::fprintf(stderr,
            "NOTE: adjlist_rand: first id %llu..%llu returned %llu (expected %llu); "
            "likely a node with no neighbours on this cursor\n",
            (unsigned long long)ids[0], (unsigned long long)ids[0],
            (unsigned long long)a.node_id, (unsigned long long)ids[0]);
    }
    for (size_t i = 1; i < ids.size(); ++i) {        // warm-up, untimed
        cursor->reset();
        cursor->set_key_range({ids[i], ids[i]});
        cursor->next(&a);
    }

    uint64_t edge_sink = 0;
    size_t calls = 0, hits = 0;
    auto t0 = Clock::now();
    while (calls < min_calls) {
        for (node_id_t id : ids) {
            cursor->reset();
            cursor->set_key_range({id, id});
            cursor->next(&a);
            hits += (a.node_id == id);
            for (node_id_t v : a.edgelist) { edge_sink += v; }   // walk the list
        }
        calls += ids.size();
    }
    double ns = ns_since(t0);
    if (edge_sink == 42) std::printf(" ");    // keep edge_sink live
    if (hit_pct) *hit_pct = 100.0 * (double)hits / (double)calls;
    return ns / (double)calls;
}

static std::vector<node_id_t> sample_by_degree(
    const std::vector<std::pair<node_id_t, size_t>> &degrees,
    size_t lo, size_t hi, size_t max_count)
{
    std::vector<node_id_t> out;
    for (const auto &[id, d] : degrees)
        if (d >= lo && d < hi) out.push_back(id);
    if (out.size() > max_count) {
        std::mt19937 rng(12345);
        std::shuffle(out.begin(), out.end(), rng);
        out.resize(max_count);
    }
    return out;
}

// ---------------- main ----------------

int main(int argc, char *argv[])
{
    bool   props  = false;
    size_t calls  = 2000000;
    int    rounds = 5;
    size_t vecs   = 4096;
    std::vector<size_t> extra_dims;
    std::string out_csv;
    std::string wt_cache;                      // e.g. 16MB; "" = WiredTiger default
    std::vector<char *> rest;
    for (int i = 0; i < argc; ++i) {
        std::string a(argv[i]);
        if      (a == "--props")                   props  = true;
        else if (a.rfind("--out=", 0) == 0)        out_csv = a.substr(6);
        else if (a.rfind("--wt-cache=", 0) == 0)   wt_cache = a.substr(11);
        else if (a.rfind("--dims=", 0) == 0) {
            std::stringstream ss(a.substr(7));
            for (std::string t; std::getline(ss, t, ',');)
                if (!t.empty()) extra_dims.push_back(std::stoull(t));
        }
        else if (a.rfind("--calls=", 0) == 0)      calls  = std::stoull(a.substr(8));
        else if (a.rfind("--rounds=", 0) == 0)     rounds = std::stoi(a.substr(9));
        else if (a.rfind("--vecs=", 0) == 0)       vecs   = std::stoull(a.substr(7));
        else rest.push_back(argv[i]);
    }
    if (rounds < 1) rounds = 1;

    CmdLineApp cli((int)rest.size(), rest.data());
    if (!cli.parse_args()) return -1;
    cmdline_opts opts = cli.get_parsed_opts();
    opts.read_only  = true;
    opts.create_new = false;
    // Smaller WiredTiger cache -> more reads miss it (OS page cache or disk).
    // For a fully cold run also drop the OS page cache first.
    if (!wt_cache.empty())
        opts.conn_config += (opts.conn_config.empty() ? "" : ",") + ("cache_size=" + wt_cache);
    if (props) {
        opts.has_node_props = true;
        opts.prop_mode      = EMBEDDED;
    }

    GraphEngine engine(1, opts);
    std::string checkpoint = engine.make_checkpoint();
    GraphBase *g = engine.create_ro_graph_handle(checkpoint);

    std::vector<node_id_t> ids;
    {
        NodeCursor *nc = g->get_node_iter();
        node_pass(nc, &ids);
        nc->close();
        delete nc;
    }

    std::vector<node_id_t> ids_rand = ids;
    std::shuffle(ids_rand.begin(), ids_rand.end(), std::mt19937(7));

    size_t db_dim = 0;
    if (props && !ids.empty()) {
        prop_blob pb = g->get_node_properties(ids.front());
        db_dim = pb.size / sizeof(float);
        if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
    }
    std::vector<size_t> dims;
    if (db_dim > 0) dims.push_back(db_dim);
    for (size_t d : extra_dims)
        if (std::find(dims.begin(), dims.end(), d) == dims.end()) dims.push_back(d);
    if (dims.empty()) {
        dims.push_back(128);
        std::fprintf(stderr, "NOTE: no --props and no --dims; cosine at 128-d only\n");
    }

    // ---------- measurements ----------
    std::vector<double> ncn, npf, npfr;                     // node cursor / node props
    std::vector<std::vector<double>> cos_c(dims.size()), cos_s(dims.size()), cos_r(dims.size());

    // One vector per graph node for each dim (same data volume as the DB rows)
    // and the two visit orders: DB order and the same shuffle as ids_rand.
    const size_t n_vec = ids.size();
    std::vector<uint32_t> ord_seq(n_vec), ord_rand;
    for (size_t i = 0; i < n_vec; ++i) ord_seq[i] = (uint32_t)i;
    ord_rand = ord_seq;
    std::shuffle(ord_rand.begin(), ord_rand.end(), std::mt19937(7));
    // Vectors for the cosines: the DB's own embeddings for its dim (read once,
    // untimed, in DB order, so index i = ids[i]); random only for --dims extras.
    std::vector<std::vector<float>> cos_data(dims.size());
    std::vector<std::string> cos_tag(dims.size());
    size_t bad_blobs = 0;
    for (size_t k = 0; k < dims.size(); ++k) {
        const size_t D = dims[k];
        cos_data[k].assign(n_vec * D, 0.0f);
        if (D == db_dim) {
            for (size_t i = 0; i < n_vec; ++i) {
                prop_blob pb = g->get_node_properties(ids[i]);
                if (pb.size == D * sizeof(float) && pb.data)
                    std::memcpy(&cos_data[k][i * D], pb.data, D * sizeof(float));
                else
                    ++bad_blobs;                    // missing / wrong size: left as zeros
                if (pb.data) delete[] const_cast<uint8_t *>(pb.data);
            }
            if (bad_blobs)
                std::fprintf(stderr, "WARN: %zu of %zu nodes had no %zu-d embedding (zeros used)\n",
                             bad_blobs, n_vec, D);
        } else {
            cos_tag[k] = "_synthetic";
            std::mt19937 rng(42);
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            for (float &x : cos_data[k]) x = dist(rng);
        }
    }
    std::vector<double> in_seq, in_rand, out_seq, out_rand;
    double in_hit = 0.0, out_hit = 0.0;
    size_t blob_bytes = 0;

    for (int r = 0; r < rounds; ++r) {
        ncn.push_back(time_node_cursor_next(g, calls));
        if (props) npf.push_back(time_node_prop_fetch(g, ids, calls, blob_bytes));
        if (props) npfr.push_back(time_node_prop_fetch(g, ids_rand, calls, blob_bytes));
        for (size_t k = 0; k < dims.size(); ++k) {
            cos_c[k].push_back(time_cosine_compute_only(cos_data[k], dims[k], vecs, calls));
            cos_s[k].push_back(time_cosine_over(cos_data[k], dims[k], ord_seq, calls));
            cos_r[k].push_back(time_cosine_over(cos_data[k], dims[k], ord_rand, calls));
        }

        {
            InCursor *c = g->get_innbd_iter();
            in_seq.push_back(time_adjlist_seq(c, calls, nullptr));
            c->close(); delete c;
        }
        {
            OutCursor *c = g->get_outnbd_iter();
            out_seq.push_back(time_adjlist_seq(c, calls, nullptr));
            c->close(); delete c;
        }
        {
            InCursor *c = g->get_innbd_iter();
            in_rand.push_back(time_adjlist_rand(c, ids_rand, calls, &in_hit));
            c->close(); delete c;
        }
        {
            OutCursor *c = g->get_outnbd_iter();
            out_rand.push_back(time_adjlist_rand(c, ids_rand, calls, &out_hit));
            c->close(); delete c;
        }
    }

    // ---------- degree stratification (once) ----------
    std::vector<std::pair<node_id_t, size_t>> in_degrees, out_degrees;
    {
        InCursor *c = g->get_innbd_iter();
        time_adjlist_seq(c, 1, &in_degrees);
        c->close(); delete c;
    }
    {
        OutCursor *c = g->get_outnbd_iter();
        time_adjlist_seq(c, 1, &out_degrees);
        c->close(); delete c;
    }

    std::vector<std::string> strat_labels;
    std::vector<double> in_strat, out_strat;
    for (size_t lo = 1, hi = 10; lo <= 10000; lo *= 10, hi *= 10) {
        auto bucket_in  = sample_by_degree(in_degrees,  lo, hi, 500);
        auto bucket_out = sample_by_degree(out_degrees, lo, hi, 500);
        if (bucket_in.empty() && bucket_out.empty()) continue;

        std::string label = "d" + std::to_string(lo);
        if (hi - 1 >= 10000) label += "p";
        else                 label += "_" + std::to_string(hi - 1);
        strat_labels.push_back(label);

        double in_ns = 0.0, out_ns = 0.0;
        if (!bucket_in.empty()) {
            InCursor *c = g->get_innbd_iter();
            in_ns = time_adjlist_rand(c, bucket_in, calls);
            c->close(); delete c;
        }
        if (!bucket_out.empty()) {
            OutCursor *c = g->get_outnbd_iter();
            out_ns = time_adjlist_rand(c, bucket_out, calls);
            c->close(); delete c;
        }
        in_strat.push_back(in_ns);
        out_strat.push_back(out_ns);
    }

    // ---------- rows ----------
    std::vector<std::pair<std::string, double>> rows;
    double nc = median(ncn);
    rows.push_back({"node_cursor_next", nc});
    if (props) rows.push_back({"node_prop_fetch",        median(npf)});
    if (props) rows.push_back({"node_prop_fetch_random", median(npfr)});
    rows.push_back({"in_cursor_next_seq",   median(in_seq)});
    rows.push_back({"in_cursor_next_rand",  median(in_rand)});
    rows.push_back({"out_cursor_next_seq",  median(out_seq)});
    rows.push_back({"out_cursor_next_rand", median(out_rand)});
    // % of random reads that returned the requested node (the rest have no
    // neighbours on that cursor and cost little); a low value means the
    // *_rand rows are averaging in no-op reads.
    rows.push_back({"in_cursor_next_rand_hit_pct",  in_hit});
    rows.push_back({"out_cursor_next_rand_hit_pct", out_hit});
    for (size_t k = 0; k < strat_labels.size(); ++k) {
        rows.push_back({"in_cursor_next_rand_"  + strat_labels[k], in_strat[k]});
        rows.push_back({"out_cursor_next_rand_" + strat_labels[k], out_strat[k]});
    }
    for (size_t k = 0; k < dims.size(); ++k) {
        std::string d = std::to_string(dims[k]) + "d" + cos_tag[k];
        rows.push_back({"cosine_" + d + "_compute_only", median(cos_c[k])});
        rows.push_back({"cosine_" + d + "_seq",          median(cos_s[k])});
        rows.push_back({"cosine_" + d + "_random",       median(cos_r[k])});
    }

    // DB vs in-memory: each DB op over the cosine with the same access pattern
    // and data size (in-order ops / cosine_<D>d_seq, random ops / _random).
    for (size_t k = 0; k < dims.size(); ++k) {
        std::string d = std::to_string(dims[k]) + "d" + cos_tag[k];
        double cs = median(cos_s[k]), cr = median(cos_r[k]);
        rows.push_back({"ratio_node_cursor_next/cosine_" + d + "_seq", nc / cs});
        if (props) rows.push_back({"ratio_node_prop_fetch/cosine_" + d + "_seq",          median(npf)  / cs});
        if (props) rows.push_back({"ratio_node_prop_fetch_random/cosine_" + d + "_random", median(npfr) / cr});
        rows.push_back({"ratio_in_cursor_next_seq/cosine_"   + d + "_seq",    median(in_seq)   / cs});
        rows.push_back({"ratio_out_cursor_next_seq/cosine_"  + d + "_seq",    median(out_seq)  / cs});
        rows.push_back({"ratio_in_cursor_next_rand/cosine_"  + d + "_random", median(in_rand)  / cr});
        rows.push_back({"ratio_out_cursor_next_rand/cosine_" + d + "_random", median(out_rand) / cr});
        for (size_t st = 0; st < strat_labels.size(); ++st) {
            if (in_strat[st] > 0)
                rows.push_back({"ratio_in_cursor_next_rand_" + strat_labels[st] + "/cosine_" + d + "_random",
                                in_strat[st] / cr});
            if (out_strat[st] > 0)
                rows.push_back({"ratio_out_cursor_next_rand_" + strat_labels[st] + "/cosine_" + d + "_random",
                                out_strat[st] / cr});
        }
    }

    // Can prefetching hide the fetch? In a prefetch pipeline the scorer reads
    // from a small, already-filled buffer (cache-hot), so compare with
    // compute_only:  < 1 one scorer outlasts the fetch (hideable);
    //                > 1 need about that many fetches in flight
    //                    (does that scale? see fetch_random_parallel.cpp).
    for (size_t k = 0; k < dims.size(); ++k) {
        std::string d = std::to_string(dims[k]) + "d" + cos_tag[k];
        double cc = median(cos_c[k]);
        if (props) rows.push_back({"prefetch_node_prop_fetch/cosine_" + d + "_compute_only",        median(npf)  / cc});
        if (props) rows.push_back({"prefetch_node_prop_fetch_random/cosine_" + d + "_compute_only", median(npfr) / cc});
        rows.push_back({"prefetch_in_cursor_next_rand/cosine_"  + d + "_compute_only", median(in_rand)  / cc});
        rows.push_back({"prefetch_out_cursor_next_rand/cosine_" + d + "_compute_only", median(out_rand) / cc});
    }

    std::printf("graph=%s nodes=%zu db_dim=%zu calls>=%zu rounds=%d vecs=%zu wt_cache=%s (median)\n",
                opts.db_name.c_str(), ids.size(), db_dim, calls, rounds, vecs,
                wt_cache.empty() ? "default" : wt_cache.c_str());
    std::printf("cosine vectors: %s\n",
                db_dim > 0 ? (bad_blobs ? "DB embeddings (some missing, see WARN)" : "DB embeddings (real)")
                           : "synthetic (no --props)");
    std::printf("%-52s %10s\n", "Operation", "ns/call");
    for (const auto &[op, v] : rows) std::printf("%-52s %10.2f\n", op.c_str(), v);

    if (!out_csv.empty()) {
        bool fresh = !std::ifstream(out_csv).good();
        std::ofstream out(out_csv, std::ios::app);
        if (fresh)
            out << "timestamp,db_dir,db_name,nodes,db_dim,blob_bytes,calls,rounds,vecs,wt_cache,operation,value\n";
        char ts[32];
        std::time_t now = std::time(nullptr);
        std::strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        for (const auto &[op, v] : rows)
            out << ts << "," << opts.db_dir << "," << opts.db_name << "," << ids.size()
                << "," << db_dim << "," << blob_bytes << "," << calls << "," << rounds
                << "," << vecs << "," << (wt_cache.empty() ? "default" : wt_cache)
                << "," << op << "," << v << "\n";
        std::fprintf(stderr, "appended %zu rows to %s\n", rows.size(), out_csv.c_str());
    }

    g->close(false);
    engine.close_graph();
    return 0;
}