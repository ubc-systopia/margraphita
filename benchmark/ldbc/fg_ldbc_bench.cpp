// fg_ldbc_bench.cpp — LDBC SNB benchmark binary for Flexograph
//
// Two modes:
//   --validate  Output JSON lines (one per query) to stdout — compatible with
//               validate_results.py for correctness checking against NeuG.
//               Replaces test/validate_queries as the --fg-bin.
//
//   (default)   Benchmark mode — runs all queries with warmup and measures
//               latency percentiles, emits CSV rows to stdout (or --out FILE).
//
// Usage:
//   fg_ldbc_bench <db_dir> [adj|splitekey] [--embedded] [--db-name=NAME]
//                 [--validate]
//                 [--warmup N=50]    [--queries N=500]
//                 [--warmup-bi N=3]  [--queries-bi N=10]
//                 [--threads N=omp_max]
//                 [--sf N=0]
//                 [--no-cache]
//                 [--out FILE]

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#include "adj_list.h"
#include "bench_harness.h"
#include "common_defs.h"
#include "edgekey_split.h"
#include "graph_engine.h"
#include "ldbc_queries.h"
#include "ldbc_queries_opt.h"
#include "prop_schema.h"

// ─── helpers ──────────────────────────────────────────────────────────────────

// Load one column by name from a comma-delimited CSV (header in row 0).
// Returns an empty vector on any error.
static std::vector<node_id_t> load_csv_col_u64(const std::string& path,
                                                const std::string& col_name)
{
    std::vector<node_id_t> result;
    FILE *f = fopen(path.c_str(), "r");
    if (!f) {
        fprintf(stderr, "[bench] cannot open param file: %s\n", path.c_str());
        return result;
    }
    char hdr[512];
    if (!fgets(hdr, sizeof(hdr), f)) { fclose(f); return result; }

    // Find column index from comma-separated header
    int col_idx = -1, ci = 0;
    char hdr_copy[512];
    strncpy(hdr_copy, hdr, sizeof(hdr_copy));
    for (char *tok = strtok(hdr_copy, ",\r\n"); tok; tok = strtok(nullptr, ",\r\n"), ++ci)
        if (strcmp(tok, col_name.c_str()) == 0) { col_idx = ci; break; }

    if (col_idx < 0) {
        fprintf(stderr, "[bench] column '%s' not found in %s\n",
                col_name.c_str(), path.c_str());
        fclose(f);
        return result;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        int fi = 0; uint64_t val = 0;
        for (char *t = strtok(line, ",\r\n"); t; t = strtok(nullptr, ",\r\n"), ++fi)
            if (fi == col_idx) { val = strtoull(t, nullptr, 10); break; }
        result.push_back((node_id_t)val);
    }
    fclose(f);
    return result;
}

static node_id_t count_nodes_of_type(GraphBase &g, VertexType vt)
{
    // Walk counters 0..N until get_node_properties returns nothing.
    // This assumes a dense sequential assignment (as built by snb_bulk_load).
    // Stop when the blob is null for 4 consecutive IDs (to tolerate minor gaps).
    node_id_t n = 0;
    int misses = 0;
    for (uint64_t c = 0; misses < 4; c++) {
        node_id_t nid = MAKE_TYPED_ID(vt, c);
        prop_blob pb  = g.get_node_properties(nid);
        if (pb.data) { n = (node_id_t)(c + 1); misses = 0; }
        else         { misses++; }
        if (c > 10000000ULL) break;  // safety: never scan more than 10M nodes
    }
    return n;
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr,
            "Usage: %s <db_dir> [adj|splitekey] [--embedded] [--db-name=NAME]\n"
            "           [--validate]\n"
            "           [--warmup N=50]   [--queries N=500]\n"
            "           [--warmup-bi N=3] [--queries-bi N=10]\n"
            "           [--threads N]     [--sf N=0]\n"
            "           [--no-cache]      [--out FILE]\n",
            argv[0]);
        return 1;
    }

    // ---- Parse arguments ----
    std::string db_dir  = argv[1];
    GraphType   gtype   = GraphType::SplitEKey;
    bool        emb     = false;
    std::string db_name = "ldbc_snb_queries";
    bool        validate_mode = false;
    int         warmup_n      = 50;
    int         queries_n     = 500;
    int         warmup_bi_n   = 3;
    int         queries_bi_n  = 10;
    int         n_threads     = omp_get_max_threads();
    int         sf            = 0;
    bool        no_cache      = false;
    std::string params_dir;
    FILE       *csv_out       = stdout;
    std::string out_path;

    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if      (a == "adj")                       gtype = GraphType::Adj;
        else if (a == "splitekey")                 gtype = GraphType::SplitEKey;
        else if (a == "--embedded")                emb   = true;
        else if (a.rfind("--db-name=", 0) == 0)   db_name = a.substr(10);
        else if (a == "--validate")                validate_mode = true;
        else if (a == "--no-cache")                no_cache = true;
        else if (a.rfind("--params-dir=", 0) == 0) params_dir = a.substr(13);
        else if (a == "--params-dir" && i+1 < argc) params_dir = argv[++i];
        else if (a.rfind("--warmup=", 0) == 0)    warmup_n    = std::stoi(a.substr(9));
        else if (a.rfind("--queries=", 0) == 0)   queries_n   = std::stoi(a.substr(10));
        else if (a.rfind("--warmup-bi=", 0) == 0) warmup_bi_n = std::stoi(a.substr(12));
        else if (a.rfind("--queries-bi=", 0) == 0) queries_bi_n = std::stoi(a.substr(13));
        else if (a.rfind("--threads=", 0) == 0)   n_threads   = std::stoi(a.substr(10));
        else if (a.rfind("--sf=", 0) == 0)        sf          = std::stoi(a.substr(5));
        else if (a.rfind("--out=", 0) == 0)       out_path    = a.substr(6);
        // space-separated forms
        else if (a == "--warmup"    && i+1 < argc) warmup_n     = std::stoi(argv[++i]);
        else if (a == "--queries"   && i+1 < argc) queries_n    = std::stoi(argv[++i]);
        else if (a == "--warmup-bi" && i+1 < argc) warmup_bi_n  = std::stoi(argv[++i]);
        else if (a == "--queries-bi"&& i+1 < argc) queries_bi_n = std::stoi(argv[++i]);
        else if (a == "--threads"   && i+1 < argc) n_threads    = std::stoi(argv[++i]);
        else if (a == "--sf"        && i+1 < argc) sf           = std::stoi(argv[++i]);
        else if (a == "--out"       && i+1 < argc) out_path     = argv[++i];
        else { fprintf(stderr, "Unknown arg: %s\n", argv[i]); return 1; }
    }

    if (!out_path.empty()) {
        csv_out = fopen(out_path.c_str(), "w");
        if (!csv_out) {
            fprintf(stderr, "Cannot open output file: %s\n", out_path.c_str());
            return 1;
        }
    }

    // ---- Open DB ----
    graph_opts opts;
    opts.create_new      = false;
    opts.is_directed     = true;
    opts.read_optimize   = true;
    opts.has_node_props  = true;
    opts.has_edge_props  = true;
    opts.prop_mode       = emb ? EMBEDDED : COLUMNAR;
    opts.type            = gtype;
    opts.db_name         = db_name;
    opts.db_dir          = db_dir;
    opts.conn_config     = "cache_size=4GB";
    opts.stat_log        = "./";

    const char *gtype_str = (gtype == GraphType::Adj) ? "adj" : "splitekey";
    const char *mode_str  = emb ? "emb" : "col";

    fprintf(stderr, "[fg_ldbc_bench] db=%s type=%s mode=%s\n",
            db_dir.c_str(), gtype_str, mode_str);

    GraphEngine engine(1, opts);
    GraphBase  *gp = engine.create_graph_handle();
    GraphBase  &g  = *gp;

    bool has_props = !emb;

    // Fallback single-node parameters (used in --validate mode and as defaults)
    node_id_t pid0  = MAKE_TYPED_ID(VT_PERSON, 0);
    node_id_t pid1  = MAKE_TYPED_ID(VT_PERSON, 1);
    node_id_t post0 = MAKE_TYPED_ID(VT_POST,   0);
    node_id_t cx    = MAKE_TYPED_ID(VT_COUNTRY, 0);
    node_id_t cy    = MAKE_TYPED_ID(VT_COUNTRY, 1);
    const int64_t INT64_MAX_VAL = 9223372036854775807LL;

    // ── Load parameter sets (if --params-dir given) ──────────────────────────
    std::vector<node_id_t> person_fgids, post_fgids;
    if (!params_dir.empty()) {
        person_fgids = load_csv_col_u64(params_dir + "/person_params.csv", "fg_typed_id");
        post_fgids   = load_csv_col_u64(params_dir + "/post_params.csv",   "fg_typed_id");
        fprintf(stderr, "[bench] params: %zu persons, %zu posts from %s\n",
                person_fgids.size(), post_fgids.size(), params_dir.c_str());
    }
    if (person_fgids.empty()) person_fgids = {pid0};
    if (post_fgids.empty())   post_fgids   = {post0};

    ParamCycle<node_id_t> pid_cycle(person_fgids);
    ParamCycle<node_id_t> post_cycle(post_fgids);

    // ─── --validate mode ──────────────────────────────────────────────────────

    if (validate_mode) {
        fprintf(stderr, "[fg_ldbc_bench] running 18 validation queries...\n");
        run_r1(g, pid0);
        run_r2(g, pid0, has_props);
        run_r3(g, pid0, pid1);
        run_x1(g, post0);
        run_x2(g, post0);
        run_x3(g, pid0, INT64_MAX_VAL, has_props);
        run_x5(g, post0, 0LL, INT64_MAX_VAL);
        run_a1(g, pid0);
        run_a2(g, pid0, 0LL, INT64_MAX_VAL, has_props);
        run_a3(g, pid1, 0LL, INT64_MAX_VAL, has_props);
        run_bi1(g, has_props);
        run_bi12(g);
        run_bi2(g, has_props);
        run_ic7(g, pid0, has_props);
        run_ic8(g, pid0, has_props);
        run_ic9(g, pid0, INT64_MAX_VAL, has_props);
        run_ic5(g, pid0, 0LL, has_props);
        run_ic3(g, pid0, cx, cy);
        fprintf(stderr, "[fg_ldbc_bench] done.\n");
        return 0;
    }

    // ─── Benchmark mode ───────────────────────────────────────────────────────

    // Step 0: count nodes, print baseline RSS
    fprintf(stderr, "[bench] Step 0: counting nodes\n");
    node_id_t post_count   = count_nodes_of_type(g, VT_POST);
    size_t    person_count = (size_t)count_nodes_of_type(g, VT_PERSON);
    fprintf(stderr, "[bench]   posts=%llu  persons=%zu  RSS=%zu MB\n",
            (unsigned long long)post_count, person_count, rss_mb());

    bench_csv_header(csv_out);

    auto emit = [&](const char *exp, const char *params, const BenchResult &r) {
        bench_csv_row(csv_out, exp, gtype_str, mode_str, sf, params, r);
        fflush(csv_out);
    };

    // Step 1: Write queries — single call each (non-repeatable)
    fprintf(stderr, "[bench] Step 1: write queries\n");
    {
        // W1 / INS-1: insert a new person with all required edges
        node_id_t new_pid = MAKE_TYPED_ID(VT_PERSON, 999999);
        // Fixed representative targets (compact IDs — actual nodes in the DB)
        node_id_t city0 = MAKE_TYPED_ID(VT_CITY,       0);
        node_id_t tag0  = MAKE_TYPED_ID(VT_TAG,        0);
        node_id_t uni0  = MAKE_TYPED_ID(VT_UNIVERSITY, 0);
        node_id_t comp0 = MAKE_TYPED_ID(VT_COMPANY,    0);
        auto t0 = Clock_t::now();
        node n; n.id = new_pid;
        g.add_node(n, false);
        if (has_props) {
            uint8_t buf[SNBPersonSchema::TOTAL_SIZE] = {};
            SNBPersonSchema::set_creation_date(buf, 1700000000000LL);
            g.set_node_properties(new_pid, buf, SNBPersonSchema::TOTAL_SIZE);
        }
        // isLocatedIn: Person -> City (structural, no edge props)
        edge e_city; e_city.src_id = new_pid; e_city.dst_id = city0;
        g.add_edge(e_city, false);
        // hasInterest: Person -> Tag (structural, no edge props)
        edge e_tag; e_tag.src_id = new_pid; e_tag.dst_id = tag0;
        g.add_edge(e_tag, false);
        // studyAt: Person -> University (classYear prop)
        edge e_uni; e_uni.src_id = new_pid; e_uni.dst_id = uni0;
        g.add_edge(e_uni, false);
        if (has_props) {
            uint8_t buf[SNBStudyAtSchema::TOTAL_SIZE] = {};
            SNBStudyAtSchema::set_class_year(buf, 2020);
            g.set_edge_properties(new_pid, uni0, buf, SNBStudyAtSchema::TOTAL_SIZE);
        }
        // workAt: Person -> Company (workFrom prop)
        edge e_comp; e_comp.src_id = new_pid; e_comp.dst_id = comp0;
        g.add_edge(e_comp, false);
        if (has_props) {
            uint8_t buf[SNBWorkAtSchema::TOTAL_SIZE] = {};
            SNBWorkAtSchema::set_work_from(buf, 2010);
            g.set_edge_properties(new_pid, comp0, buf, SNBWorkAtSchema::TOTAL_SIZE);
        }
        double w1_ms = Ms_t(Clock_t::now() - t0).count();
        emit("w1_insert_person", "pid=999999", run_once(w1_ms));
    }
    {
        // INS-4: insert a new forum with hasModerator and hasTag edges
        node_id_t new_forum = MAKE_TYPED_ID(VT_FORUM, 999999);
        node_id_t mod_pid   = pid_cycle.next();
        node_id_t tag0      = MAKE_TYPED_ID(VT_TAG, 0);
        auto t0 = Clock_t::now();
        node nf; nf.id = new_forum;
        g.add_node(nf, false);
        if (has_props) {
            const char *title = "New Forum";
            size_t bsz = SNBForumSchema::TOTAL_SIZE + strlen(title) + 1;
            std::vector<uint8_t> buf(bsz, 0);
            SNBForumSchema::set_creation_date(buf.data(), 1700000000000LL);
            SNBForumSchema::set_moderator_id(buf.data(), (int64_t)mod_pid);
            SNBForumSchema::set_title(buf.data(), title);
            g.set_node_properties(new_forum, buf.data(), bsz);
        }
        // hasModerator: Forum -> Person (structural, no edge props)
        edge e_mod; e_mod.src_id = new_forum; e_mod.dst_id = mod_pid;
        g.add_edge(e_mod, false);
        // hasTag: Forum -> Tag (structural, no edge props)
        edge e_tag; e_tag.src_id = new_forum; e_tag.dst_id = tag0;
        g.add_edge(e_tag, false);
        double ins4_ms = Ms_t(Clock_t::now() - t0).count();
        emit("ins4_insert_forum", "forum=999999;mod=sampled", run_once(ins4_ms));
    }
    {
        // W2 / INS-8: insert a knows edge (both directions, per spec)
        node_id_t w2_src = pid_cycle.next();
        node_id_t w2_dst = pid_cycle.next();
        auto t0 = Clock_t::now();
        edge e_fwd; e_fwd.src_id = w2_src; e_fwd.dst_id = w2_dst;
        g.add_edge(e_fwd, false);
        if (has_props) {
            uint8_t buf[SNBKnowsSchema::TOTAL_SIZE] = {};
            SNBKnowsSchema::set_creation_date(buf, 1700000000000LL);
            g.set_edge_properties(w2_src, w2_dst, buf, SNBKnowsSchema::TOTAL_SIZE);
        }
        edge e_rev; e_rev.src_id = w2_dst; e_rev.dst_id = w2_src;
        g.add_edge(e_rev, false);
        if (has_props) {
            uint8_t buf[SNBKnowsSchema::TOTAL_SIZE] = {};
            SNBKnowsSchema::set_creation_date(buf, 1700000000000LL);
            g.set_edge_properties(w2_dst, w2_src, buf, SNBKnowsSchema::TOTAL_SIZE);
        }
        double w2_ms = Ms_t(Clock_t::now() - t0).count();
        emit("w2_insert_knows", "src=sampled;dst=sampled", run_once(w2_ms));
    }
    {
        // W3 / INS-6: insert a post with hasCreator, containerOf, isLocatedIn, hasTag
        node_id_t new_post_id = MAKE_TYPED_ID(VT_POST, post_count + 1);
        node_id_t w3_author   = pid_cycle.next();
        node_id_t forum0      = MAKE_TYPED_ID(VT_FORUM,   0);
        node_id_t country0    = MAKE_TYPED_ID(VT_COUNTRY, 0);
        node_id_t tag0        = MAKE_TYPED_ID(VT_TAG,     0);
        auto t0 = Clock_t::now();
        node n2; n2.id = new_post_id;
        g.add_node(n2, false);
        if (has_props) {
            const char *content = "benchmark post";
            size_t bsz = SNBPostSchema::TOTAL_SIZE + strlen(content) + 1;
            std::vector<uint8_t> buf(bsz, 0);
            SNBPostSchema::set_creation_date(buf.data(), 1700000000000LL);
            SNBPostSchema::set_length(buf.data(), (int32_t)strlen(content));
            SNBPostSchema::set_content(buf.data(), content);
            g.set_node_properties(new_post_id, buf.data(), bsz);
        }
        // hasCreator: Post -> Person (structural)
        edge e_creator; e_creator.src_id = new_post_id; e_creator.dst_id = w3_author;
        g.add_edge(e_creator, false);
        // containerOf: Forum -> Post (structural)
        edge e_container; e_container.src_id = forum0; e_container.dst_id = new_post_id;
        g.add_edge(e_container, false);
        // isLocatedIn: Post -> Country (structural)
        edge e_loc; e_loc.src_id = new_post_id; e_loc.dst_id = country0;
        g.add_edge(e_loc, false);
        // hasTag: Post -> Tag (structural)
        edge e_tag; e_tag.src_id = new_post_id; e_tag.dst_id = tag0;
        g.add_edge(e_tag, false);
        double w3_ms = Ms_t(Clock_t::now() - t0).count();
        emit("w3_insert_post_with_creator", "author=sampled", run_once(w3_ms));
    }
    {
        // X4 / INS-2: insert a likes edge (Person -> Post)
        node_id_t x4_pid  = pid_cycle.next();
        node_id_t x4_post = post_cycle.next();
        auto t0 = Clock_t::now();
        edge e3; e3.src_id = x4_pid; e3.dst_id = x4_post;
        g.add_edge(e3, false);
        if (has_props) {
            uint8_t buf[SNBLikesSchema::TOTAL_SIZE] = {};
            SNBLikesSchema::set_creation_date(buf, 1700000000000LL);
            g.set_edge_properties(x4_pid, x4_post, buf, SNBLikesSchema::TOTAL_SIZE);
        }
        double x4_ms = Ms_t(Clock_t::now() - t0).count();
        emit("x4_insert_likes", "pid=sampled;post=sampled", run_once(x4_ms));
    }
    {
        // INS-3: insert a likes edge (Person -> Comment)
        node_id_t ins3_pid     = pid_cycle.next();
        node_id_t ins3_comment = MAKE_TYPED_ID(VT_COMMENT, 0);
        auto t0 = Clock_t::now();
        edge e4; e4.src_id = ins3_pid; e4.dst_id = ins3_comment;
        g.add_edge(e4, false);
        if (has_props) {
            uint8_t buf[SNBLikesSchema::TOTAL_SIZE] = {};
            SNBLikesSchema::set_creation_date(buf, 1700000000000LL);
            g.set_edge_properties(ins3_pid, ins3_comment, buf, SNBLikesSchema::TOTAL_SIZE);
        }
        double ins3_ms = Ms_t(Clock_t::now() - t0).count();
        emit("ins3_insert_likes_comment", "pid=sampled;comment=0", run_once(ins3_ms));
    }
    {
        // INS-5: insert a hasMember edge (Forum -> Person)
        node_id_t ins5_forum = MAKE_TYPED_ID(VT_FORUM, 0);
        node_id_t ins5_pid   = pid_cycle.next();
        auto t0 = Clock_t::now();
        edge e5; e5.src_id = ins5_forum; e5.dst_id = ins5_pid;
        g.add_edge(e5, false);
        if (has_props) {
            uint8_t buf[SNBHasMemberSchema::TOTAL_SIZE] = {};
            SNBHasMemberSchema::set_creation_date(buf, 1700000000000LL);
            g.set_edge_properties(ins5_forum, ins5_pid, buf, SNBHasMemberSchema::TOTAL_SIZE);
        }
        double ins5_ms = Ms_t(Clock_t::now() - t0).count();
        emit("ins5_insert_hasmember", "forum=0;pid=sampled", run_once(ins5_ms));
    }
    {
        // INS-7: insert a new comment with hasCreator, replyOf, isLocatedIn, hasTag
        node_id_t new_comment = MAKE_TYPED_ID(VT_COMMENT, 999999);
        node_id_t ins7_author = pid_cycle.next();
        node_id_t post0       = MAKE_TYPED_ID(VT_POST,    0);
        node_id_t country0    = MAKE_TYPED_ID(VT_COUNTRY, 0);
        node_id_t tag0        = MAKE_TYPED_ID(VT_TAG,     0);
        auto t0 = Clock_t::now();
        node nc; nc.id = new_comment;
        g.add_node(nc, false);
        if (has_props) {
            const char *content = "benchmark comment";
            size_t bsz = SNBCommentSchema::TOTAL_SIZE + strlen(content) + 1;
            std::vector<uint8_t> buf(bsz, 0);
            SNBCommentSchema::set_creation_date(buf.data(), 1700000000000LL);
            SNBCommentSchema::set_length(buf.data(), (int32_t)strlen(content));
            SNBCommentSchema::set_content(buf.data(), content);
            g.set_node_properties(new_comment, buf.data(), bsz);
        }
        // hasCreator: Comment -> Person (structural)
        edge e_creator; e_creator.src_id = new_comment; e_creator.dst_id = ins7_author;
        g.add_edge(e_creator, false);
        // replyOf: Comment -> Post (structural)
        edge e_reply; e_reply.src_id = new_comment; e_reply.dst_id = post0;
        g.add_edge(e_reply, false);
        // isLocatedIn: Comment -> Country (structural)
        edge e_loc; e_loc.src_id = new_comment; e_loc.dst_id = country0;
        g.add_edge(e_loc, false);
        // hasTag: Comment -> Tag (structural)
        edge e_tag; e_tag.src_id = new_comment; e_tag.dst_id = tag0;
        g.add_edge(e_tag, false);
        double ins7_ms = Ms_t(Clock_t::now() - t0).count();
        emit("ins7_insert_comment", "comment=999999;author=sampled", run_once(ins7_ms));
    }

    // Step 2: Read/aggregate/IC queries
    fprintf(stderr, "[bench] Step 2: read/aggregate/IC queries (warmup=%d, measure=%d)\n",
            warmup_n, queries_n);

    emit("r1", "pid=sampled",
         run_timed([&]{ tq_r1_person_profile(g, pid_cycle.next(), has_props); }, warmup_n, queries_n));
    emit("r2", "pid=sampled",
         run_timed([&]{ tq_r2_friends_sorted_by_date(g, pid_cycle.next(), has_props); }, warmup_n, queries_n));
    emit("r3", "pid=sampled;dst=sampled",
         run_timed([&]{ auto a = pid_cycle.next(), b = pid_cycle.next();
                        tq_r3_bfs_shortest_path(g, a, b); }, warmup_n, queries_n));
    emit("r3_opt", "pid=sampled;dst=sampled",
         run_timed([&]{ auto a = pid_cycle.next(), b = pid_cycle.next();
                        tq_r3_bfs_shortest_path_bidir(g, a, b, person_count); }, warmup_n, queries_n));
    emit("x1", "post=sampled",
         run_timed([&]{ tq_x1_post_profile(g, post_cycle.next()); }, warmup_n, queries_n));
    emit("x2", "post=sampled",
         run_timed([&]{ tq_x2_post_author(g, post_cycle.next()); }, warmup_n, queries_n));
    emit("x3", "pid=sampled;cutoff=MAX",
         run_timed([&]{ tq_x3_ic2_friends_recent_posts(g, pid_cycle.next(), INT64_MAX_VAL, has_props); },
                   warmup_n, queries_n));
    emit("x3_opt", "pid=sampled;cutoff=MAX",
         run_timed([&]{ tq_x3_ic2_friends_recent_posts_flat_scan(g, pid_cycle.next(), INT64_MAX_VAL, has_props); },
                   warmup_n, queries_n));
    emit("x5", "post=sampled;lo=0;hi=MAX",
         run_timed([&]{ tq_x5_count_likes_in_range(g, post_cycle.next(), 0LL, INT64_MAX_VAL); },
                   warmup_n, queries_n));
    emit("a1", "pid=sampled",
         run_timed([&]{ tq_a1_degree_count(g, pid_cycle.next()); }, warmup_n, queries_n));
    emit("a2", "pid=sampled;lo=0;hi=MAX",
         run_timed([&]{ tq_a2_knows_in_date_range(g, pid_cycle.next(), 0LL, INT64_MAX_VAL, has_props); },
                   warmup_n, queries_n));
    emit("a3", "pid=sampled;lo=0;hi=MAX",
         run_timed([&]{ tq_a3_posts_liked_in_range(g, pid_cycle.next(), 0LL, INT64_MAX_VAL, has_props); },
                   warmup_n, queries_n));
    emit("ic3", "pid=sampled",
         run_timed([&]{ tq_ic3_fof_by_country(g, pid_cycle.next(), cx, cy); }, warmup_n, queries_n));
    emit("ic3_opt", "pid=sampled",
         run_timed([&]{ tq_ic3_fof_by_country_citycache(g, pid_cycle.next(), cx, cy); }, warmup_n, queries_n));
    emit("ic5", "pid=sampled;since=0",
         run_timed([&]{ tq_ic5_forums_by_friend_membership(g, pid_cycle.next(), 0LL, has_props); },
                   warmup_n, queries_n));
    emit("ic7", "pid=sampled",
         run_timed([&]{ tq_ic7_message_likes(g, pid_cycle.next(), has_props); }, warmup_n, queries_n));
    emit("ic8", "pid=sampled",
         run_timed([&]{ tq_ic8_latest_replies(g, pid_cycle.next(), has_props); }, warmup_n, queries_n));
    emit("ic9", "pid=sampled;cutoff=MAX",
         run_timed([&]{ tq_ic9_friends_messages_before(g, pid_cycle.next(), INT64_MAX_VAL, has_props); },
                   warmup_n, queries_n));

    // Step 3: BI queries (smaller warmup/measure because each call takes 100–2000 ms)
    fprintf(stderr, "[bench] Step 3: BI queries (warmup=%d, measure=%d)\n",
            warmup_bi_n, queries_bi_n);

    if (has_props) {
        emit("bi1", "total_posts",
             run_timed([&]{ tq_bi1_posting_summary(g, post_count); },
                       warmup_bi_n, queries_bi_n));
        emit("bi2", "lo1=1325376000000;hi1=1341100800000",
             run_timed([&]{ tq_bi2_message_count_two_windows(g,
                                1325376000000LL, 1341100800000LL,
                                1356998400000LL, 1372636800000LL); },
                       warmup_bi_n, queries_bi_n));
        emit("bi12", "max_date=MAX;min_length=0",
             run_timed([&]{ tq_bi12_message_distribution_fast(g, post_count); },
                       warmup_bi_n, queries_bi_n));
        emit("bi12_opt", "max_date=MAX;min_length=0",
             run_timed([&]{ tq_bi12_message_distribution_fast_dense(g, post_count, person_count); },
                       warmup_bi_n, queries_bi_n));
    } else {
        fprintf(stderr, "[bench]   BI queries skipped in EMBEDDED mode "
                        "(COLUMNAR only)\n");
    }

    // Step 4: Create checkpoint for parallel read-only sessions
    fprintf(stderr, "[bench] Step 4: creating checkpoint\n");
    std::string chkpt = engine.make_checkpoint();
    fprintf(stderr, "[bench]   checkpoint='%s'\n", chkpt.c_str());

    // Step 5: Build NeighborCache
    if (!no_cache) {
        fprintf(stderr, "[bench] Step 5: building NeighborCache\n");
        size_t rss_before = rss_mb();
        double cache_ms = 0.0;
        NeighborCache cache = build_neighbor_cache(g, &cache_ms);
        size_t rss_after = rss_mb();
        fprintf(stderr, "[bench]   cache built in %.1f ms, RSS delta=%zu MB\n",
                cache_ms, rss_after - rss_before);

        // Step 6: OPT-1 cached variants
        fprintf(stderr, "[bench] Step 6: OPT-1 cached variants\n");
        emit("x3_cached", "pid=sampled;cutoff=MAX",
             run_timed([&]{ tq_x3_ic2_friends_recent_posts_cached(
                                g, cache, pid_cycle.next(), INT64_MAX_VAL, has_props); },
                       warmup_n, queries_n));
        emit("ic7_cached", "pid=sampled",
             run_timed([&]{ tq_ic7_message_likes_cached(g, cache, pid_cycle.next(), has_props); },
                       warmup_n, queries_n));
        emit("ic9_cached", "pid=sampled;cutoff=MAX",
             run_timed([&]{ tq_ic9_friends_messages_before_cached(
                                g, cache, pid_cycle.next(), INT64_MAX_VAL, has_props); },
                       warmup_n, queries_n));
        emit("ic5_cached", "pid=sampled;since=0",
             run_timed([&]{ tq_ic5_forums_by_friend_membership_cached(
                                g, cache, pid_cycle.next(), 0LL, has_props); },
                       warmup_n, queries_n));
        emit("ic3_cached", "pid=sampled",
             run_timed([&]{ tq_ic3_fof_by_country_cached(cache, pid_cycle.next(), cx, cy); },
                       warmup_n, queries_n));
    } else {
        fprintf(stderr, "[bench] Step 5+6: skipped (--no-cache)\n");
    }

    // Step 7: Parallel BI variants (COLUMNAR mode only)
    if (has_props && !chkpt.empty()) {
        fprintf(stderr, "[bench] Step 7: parallel BI variants (%d threads)\n", n_threads);

        graph_opts ro_opts = opts;
        ro_opts.create_new       = false;
        ro_opts.read_optimize    = true;
        ro_opts.checkpoint_name  = chkpt;
        WT_CONNECTION *conn = engine.get_connection();

        char threads_param[64];
        snprintf(threads_param, sizeof(threads_param), "threads=%d", n_threads);

        auto t0 = Clock_t::now();
        bi1_posting_summary_parallel(conn, ro_opts, post_count, n_threads);
        double bi1_par_ms = Ms_t(Clock_t::now() - t0).count();
        emit("bi1_parallel", threads_param, run_once(bi1_par_ms));

        auto t1 = Clock_t::now();
        bi12_message_distribution_fast_parallel(conn, ro_opts, post_count, n_threads);
        double bi12_par_ms = Ms_t(Clock_t::now() - t1).count();
        emit("bi12_parallel", threads_param, run_once(bi12_par_ms));

        // OPT-3 variant (single-threaded but with dense qualifying array)
        {
            auto t2 = Clock_t::now();
            bi12_message_distribution_fast_v2(g, post_count);
            double bi12_v2_ms = Ms_t(Clock_t::now() - t2).count();
            emit("bi12_fast_v2", "max_date=MAX;min_length=0", run_once(bi12_v2_ms));
        }
    } else {
        fprintf(stderr, "[bench] Step 7: skipped (EMBEDDED mode or no checkpoint)\n");
    }

    // Step 8: Final RSS and total wall time
    fprintf(stderr, "[bench] Step 8: final RSS=%zu MB\n", rss_mb());

    if (csv_out != stdout) fclose(csv_out);
    return 0;
}
