// fg_linkpred_bench_db.cpp — DB-native link prediction (structural + embedding).
// Exhaustive candidates: scores ALL pairs (u, v) with u < v.
//
// Flow of main:
//   1. parse flags            (linkpred_options.h; --help lists them)
//   2. open DB + handles      open_handles()
//   3. preload (pvector)      preload_embeddings()      (linkpred_exhaustive.h)
//   4. per trial: run         run_selected()            → RunResult
//   5. report                 print_result / CSVs       (linkpred_output.h)
// See steps.md for a file-by-file walkthrough.
//
// Selection (run_selected):
//   --impl=reference --variant=X     single-threaded versions (linkpred_reference.h)
//                                    (alias: --reference); + --candidates=fof|khop
//                                    → run_reference_fof_structural_cursor / _khop_structural_bfs (cn / jaccard)
//                                      or _fof_embedding / _khop_embedding_bfs (cosine / dot)
//   --impl=pipeline --candidates=... pipeline, cn / jaccard (linkpred_pipeline.h)
//                                    exhaustive | fof | khop (--k) from linkpred_candidates.h
//   --impl=direct --candidates=...   hand-written baselines: fof → linkpred_fof.h,
//                                    khop → linkpred_khop.h, exhaustive → linkpred_exhaustive.h;
//                                    cosine / dot + fof|khop: same candidates, v's embedding
//                                    read per candidate (run_direct_fof_embedding_cursor / _khop)
//   --impl=direct (default), no --candidates: original path (linkpred_exhaustive.h)
//     cn / jaccard                   structural, cursor
//     cn / jaccard + csr             structural, adjacency preloaded once (in-memory ceiling)
//     any method + tiled             DB reads, each v read once per --tile block of u's
//     cosine / dot + pvector         embeddings preloaded
//     cosine / dot + cursor          embeddings read per pair
// --variant=all (default) becomes cursor (structural, or embedding with fof / khop)
// / pvector (embedding, exhaustive).
// --threads=1 runs the same threaded code with one thread.
//
// Accumulation (threaded path only):
//   --accum=local   (default) each thread keeps its top-k heap and counters in
//                   local variables and writes them to per_*[tid] once at the end.
//   --accum=shared  the original version: each thread updates per_*[tid] for
//                   every pair (false sharing). Kept for comparison.
//   Results are identical; only timing differs. Marked accum=shared on stdout
//   and *_shared in the CSV variation column.
//
// --impl=reference runs are marked reference=1 on stdout and *_ref in the CSV
// variation column, and are for reading / cross-checking, not for timing.
//
// Diagnostic prints: one [config] banner and one [trial N/M] line per trial,
// both before any timer. run_selected is silent.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "mem_usage.h"
#include "pvector.h"
#include "command_line.h"
#include "common_defs.h"
#include "graph_engine.h"
#include "times.h"
#include "common_util.h"

#include "linkpred_options.h"
#include "linkpred_types.h"
#include "linkpred_output.h"
#include "linkpred_reference.h"   // used only with --impl=reference
// #include "linkpred_exhaustive.h"  // pre-split original; replaced by the two family files below (2026-11-17)
#include "linkpred_structural_exhaustive.h"  // exhaustive structural: cursor, csr (LinkpredCsr, preload_csr), tiled
#include "linkpred_embedding_exhaustive.h"   // exhaustive embedding: cursor, pvector (preload_embeddings), tiled
#include "linkpred_candidates.h"  // per-u generators for --candidates
#include "linkpred_pipeline.h"    // --candidates runs (structural)
#include "linkpred_structural_fof.h"   // --candidates=fof  --impl=direct (structural: cursor, wedge)
#include "linkpred_structural_fof_compact.h"   // --variant=wedge_compact | outcursor_wedge_compact (fof, khop2)
#include "linkpred_embedding_fof.h"    // --candidates=fof  --impl=direct (embedding: cursor)
#include "linkpred_structural_khop.h"  // --candidates=khop --impl=direct (structural: cursor_bfs, wedge, outcursor_nested)
#include "linkpred_embedding_khop.h"   // --candidates=khop --impl=direct (embedding: cursor_bfs, outcursor_nested)


// ---------------------------------------------------------------- helpers
// Main's handle plus one read-only handle per worker thread (g_thread_handles),
// all on the same checkpoint. One handle per thread also when threads == 1.
//
// Opened one at a time on purpose: create_ro_graph_handle writes the engine's
// shared opts (checkpoint_name, read_only, num_edges) and checkpoint_node_count,
// and copies opts into each handle, so calling it from several threads at once
// is a data race. (bfs_parallel.cpp opens handles in parallel and has the same
// race.) Handle opening is a one-time cost counted in load_ms.
static GraphBase *open_handles(GraphEngine &engine, const Options &o)
{
    // --checkpoint=last (default): read the latest unnamed checkpoint, which the
    // previous clean close wrote; no new checkpoint. named: the old path, one named
    // checkpoint per run, never dropped (976 had piled up on s12: 294 MB files for
    // 572 KB of data, ~15 s to open).
    std::string checkpoint = (o.checkpoint == "named") ? engine.make_checkpoint()
                                                       : std::string("WiredTigerCheckpoint");
    GraphBase *g = engine.create_ro_graph_handle(checkpoint);   // main's handle

    g_thread_handles.resize(o.threads);
    for (int i = 0; i < o.threads; ++i) {
        g_thread_handles[i] = engine.create_ro_graph_handle(checkpoint);   // one session each
    }
    return g;
}

// Picks and runs the function for --impl / --method / --variant / --candidates.
// o.variant is already resolved (never "all" here; see resolve_variant_all).
// Silent: no IO. Diagnostics live in main() outside the timers.
static RunResult run_selected(GraphBase *g, const Options &o,
                              const std::vector<node_id_t> &nodes,
                              const pvector<std::vector<float>> &all_emb,
                              const LinkpredCsr &csr)
{
    bool structural = method_is_structural(o);

    if (o.impl == "reference") {
        // Plain single-threaded versions (linkpred_reference.h), on main's g.
        // --candidates=fof / khop pick the candidate reference (cursor variant only;
        // validate_lp_options enforces that). Empty / exhaustive → by --variant.
        if (o.candidates == "fof" && o.variant == "wedge") return run_reference_fof_structural_wedge(g, o);
        if (o.candidates == "fof")  return structural ? run_reference_fof_structural_cursor(g, o)
                                                  : run_reference_fof_embedding_cursor(g, o);
        if (o.candidates == "khop") {
            if (structural) {
                if (o.variant == "wedge")            return run_reference_khop_structural_wedge(g, o);
                if (o.variant == "outcursor_bfs")    return run_reference_khop_structural_outcursor_bfs(g, o);
                if (o.variant == "nested")           return run_reference_khop_structural_nested(g, o);
                if (o.variant == "outcursor_nested") return run_reference_khop_structural_outcursor_nested(g, o);
            } else {
                if (o.variant == "nested")           return run_reference_khop_embedding_nested(g, o);
                if (o.variant == "outcursor_nested") return run_reference_khop_embedding_outcursor_nested(g, o);
                if (o.variant == "nested_cached")    return run_reference_khop_embedding_nested_cached(g, o);
            }
        }
        if (o.candidates == "khop") return structural ? run_reference_khop_structural_cursor_bfs(g, o)
                                                  : run_reference_khop_embedding_cursor_bfs(g, o);
        if (o.candidates == "uniformRandom")
            return structural ? run_reference_uniformRandom_structural_cursor(g, o, o.samples, (uint64_t)o.seed)
                              : run_reference_uniformRandom_embedding_cursor(g, o, o.samples, (uint64_t)o.seed);
        if (structural) {
            if (o.variant == "preloaded_ids") return run_reference_exhaustive_structural_preloaded_ids(g, o);
            if (o.variant == "preloaded_adj") return run_reference_exhaustive_structural_csr(g, o);
            if (o.variant == "tiled")         return run_reference_exhaustive_structural_tiled(g, o);
            return run_reference_exhaustive_structural_cursor(g, o);                    // cursor
        }
        if (o.variant == "pvector")       return run_reference_exhaustive_embedding_pvector(g, nodes, all_emb, o);
        if (o.variant == "cursor_cached") return run_reference_exhaustive_embedding_cursor_cached(g, o);
        if (o.variant == "preloaded_db")  return run_reference_exhaustive_embedding_preloaded_db(g, o);
        if (o.variant == "tiled")         return run_reference_exhaustive_embedding_tiled(g, o);
        return run_reference_exhaustive_embedding_cursor(g, o);                  // cursor
    }
    if (!structural && o.impl == "direct" && candidates_fof_or_khop(o)) {
        // Embedding over structural candidates: same walk, v's embedding per candidate.
        if (o.variant == "csr" && o.candidates == "fof")
            return run_direct_fof_embedding_csr(csr, all_emb, o);                         // in-memory CSR + pvector
        if (o.variant == "csr" && o.candidates == "khop")
            return run_direct_khop_embedding_csr(csr, all_emb, o);                        // in-memory CSR + pvector, any k
        if (o.candidates == "fof") return run_direct_fof_embedding_cursor(g, o);     // linkpred_embedding_fof.h
        if (o.variant == "outcursor_nested")
            return run_direct_khop_embedding_outcursor_nested(g, o);                     // linkpred_embedding_khop.h (k = 2)
        return run_direct_khop_embedding_cursor_bfs(g, o);                               // linkpred_khop.h
    }
    if (structural && !o.candidates.empty() && o.impl == "direct") {
        // Hand-written baselines (no pipeline), structural only.
        if (o.variant == "csr" && o.candidates == "fof")
            return run_direct_fof_structural_csr_wedge(csr, o);               // in-memory CSR (preloaded before timing)
        if (o.variant == "csr" && o.candidates == "khop")
            return run_direct_khop_structural_csr_wedge(csr, o);              // in-memory CSR, k = 2
        if (o.variant == "csr_cursor" && o.candidates == "fof")
            return run_direct_fof_structural_csr_cursor(csr, o);              // cursor algorithm over in-memory CSR
        if (o.variant == "csr_cursor" && o.candidates == "khop")
            return run_direct_khop_structural_csr_cursor(csr, o);             // cursor BFS over in-memory CSR, any k
        if (o.candidates == "fof" && (o.variant == "wedge_compact" || o.variant == "outcursor_wedge_compact"))
            return run_direct_fof_structural_wedge_compact(g, o);    // linkpred_structural_fof_compact.h: dense node index
        if (o.candidates == "khop" && (o.variant == "wedge_compact" || o.variant == "outcursor_wedge_compact"))
            return run_direct_khop_structural_wedge_compact(g, o);   // same, khop2 set
        if (o.candidates == "fof" && (o.variant == "wedge" || o.variant == "outcursor_wedge"))
            return run_direct_fof_structural_wedge(g, o);   // linkpred_structural_fof.h; outcursor_wedge: OutCursor reads
        if (o.candidates == "fof")  return run_direct_fof_structural_cursor(g, o);   // linkpred_fof.h
        if (o.candidates == "khop" && (o.variant == "wedge" || o.variant == "outcursor_wedge"))
            return run_direct_khop_structural_wedge(g, o);                    // linkpred_fof.h
        if (o.candidates == "khop" && o.variant == "outcursor_nested")
            return run_direct_khop_structural_outcursor_nested(g, o);           // linkpred_structural_khop.h (k = 2)
        if (o.candidates == "khop") return run_direct_khop_structural_cursor_bfs(g, o);   // linkpred_structural_khop.h
        if (o.variant == "csr")     return run_direct_exhaustive_structural_csr(csr, o);  // linkpred_structural_exhaustive.h
        if (o.variant == "tiled")   return run_direct_exhaustive_structural_tiled(g, o);  // linkpred_structural_exhaustive.h
        return run_direct_exhaustive_structural_cursor(g, o);                            // linkpred_structural_exhaustive.h
    }
    if (o.impl == "pipeline") {
        // Pipeline (linkpred_pipeline.h): pluggable candidates.
        // structural -> run_pipeline_structural; embedding -> run_pipeline_embedding_cursor
        // (or _pvector with --variant=pvector, embeddings preloaded).
        // (validate_lp_options guarantees --candidates is set.)
        auto run = [&](auto make_cands) {
            if (structural)              return run_pipeline_structural(g, o, make_cands);
            if (o.variant == "pvector")  return run_pipeline_embedding_pvector(g, nodes, all_emb, o, make_cands);
            return run_pipeline_embedding_cursor(g, o, make_cands);
        };
        if (o.candidates == "fof")
            return run([](GraphBase *h) { return linkpred::FofCandidates(h); });
        if (o.candidates == "khop")
            return run([&](GraphBase *h) { return linkpred::KHopCandidates(h, o.k_hops); });
        return run([](GraphBase *h) { return linkpred::ExhaustiveCandidates(h); });
    }
    // --impl=direct without --candidates: the original exhaustive path.
    if (structural) {
        if (o.variant == "csr")   return run_direct_exhaustive_structural_csr(csr, o);   // preloaded, no DB calls
        if (o.variant == "tiled") return run_direct_exhaustive_structural_tiled(g, o);   // DB, v read once per block
        return run_direct_exhaustive_structural_cursor(g, o);
    }
    if (o.variant == "pvector")
        return run_direct_exhaustive_embedding_pvector(nodes, all_emb, o);   // uses handles
    if (o.variant == "tiled")
        return run_direct_exhaustive_embedding_tiled(g, o);               // DB, v read once per block
    return run_direct_exhaustive_embedding_cursor(g, o);                 // per-pair DB reads
}


// ---------------------------------------------------------------- main
int main(int argc, char *argv[])
{
    auto t_start = std::chrono::high_resolution_clock::now();   // T0: full run start

    mem_util::MemoryCounter memory_usage;                       // reports on scope exit

    Options o;
    std::vector<char *> clean_argv;
    if (!parse_lp_options(argc, argv, o, clean_argv, "direct")) return 1;
    resolve_variant_all(o);   // bench runs one variant: all -> family default

    CmdLineApp cli((int)clean_argv.size(), clean_argv.data());
    if (!cli.parse_args()) return -1;
    cmdline_opts opts = cli.get_parsed_opts();
    opts.read_only  = true;
    opts.create_new = false;
    setup_out_root(o, opts.db_name);  // --out-root: per-run-type folder + console copy to logs/<db name>/
    if (!o.cache_size.empty())                                  // --cache_size -> WT conn config
        opts.conn_config += (opts.conn_config.empty() ? "" : ",") + ("cache_size=" + o.cache_size);
    else {
        // If No set  --cache_size: WiredTiger does NOT fall back to its 100 MB default here.
        // It uses the cache_size stored in the DB's WiredTiger.basecfg, written when
        // the DB was created (10GB for our ingests; time checked 2026-11-17 with the
        // "cache: maximum bytes configured" statistic). 
        std::ifstream bc(opts.db_dir + "/" + opts.db_name + "/WiredTiger.basecfg");
        std::string line, base;
        while (std::getline(bc, line)) {
            size_t p = line.find("cache_size=");
            if (p != std::string::npos) {
                base = line.substr(p + 11);
                base = base.substr(0, base.find_first_of(",)\n"));
            }
        }
        o.cache_size = base.empty() ? "default" : base + "(basecfg)";   // logging only
    }

    bool is_embedding = method_is_embedding(o);
    if (is_embedding) {
        opts.has_node_props = true;                             // need node_props table
        opts.prop_mode      = EMBEDDED;                         // blobs inline with node
        std::cerr << "[linkpred] embedding mode: reading node_props as blobs\n";
    }

    // ---- DB open + handle creation (part of "with DB open" cost) ----
    Times t;
    t.start();
    g_graph_directed = opts.is_directed;                        // --direction=both needs -d
    if (!opts.is_directed && opts.db_name.find("_rd") != std::string::npos)
        std::cerr << "WARNING: " << opts.db_name << " looks directed (_rd) but was opened without -d:"
                  << " in-lists are not read, so --direction=both has no effect\n";
    if (o.eval == "holdout") {
        size_t nt = lp_load_split(o.split_file);
        if (nt == 0) { std::cerr << "ERROR: no test pairs read from " << o.split_file << "\n"; return 1; }
        std::cerr << "[linkpred] eval=holdout: " << nt << " test pairs hidden (" << o.split_file << ")\n";
        if (!method_is_structural(o))
            std::cerr << "WARNING: holdout with embeddings: the embeddings were computed on the full"
                      << " graph, so they have seen the test pairs (use embeddings made on the"
                      << " training graph for a fair number)\n";
    }
    std::cerr << "[linkpred] direction=" << o.direction
              << (lp_both_directions(o.direction) ? " (out U in)" : " (out only)") << "\n";
    GraphEngine engine(o.threads + 1, opts);                    // +1 for main's handle
    GraphBase *g = open_handles(engine, o);
    t.stop();
    std::cout << "Graph loaded in " << t.t_secs() << "s"
              << " (threads=" << o.threads
              << ", variant=" << o.variant << ")\n";

    // ---- Preload phase (also part of "with DB open" cost) ----
    // For pvector: read all embeddings into memory once, so hot loop
    // never touches WT. For csr: same for the adjacency (structural).
    // For cursor: skip — reads per pair instead.
    std::vector<node_id_t> nodes;
    pvector<std::vector<float>> all_emb;
    LinkpredCsr csr;
    size_t preload_dim = 0;
    // embedding --variant=csr (fof / khop) preloads both: CSR for candidates, pvector for scores
    bool pv_mode = is_embedding && (o.variant == "pvector" || o.variant == "csr");

    if (pv_mode) preload_dim = preload_embeddings(g, nodes, all_emb);           // linkpred_embedding_exhaustive.h
    if (o.variant == "csr" || o.variant == "csr_cursor") preload_csr(g, csr, lp_both_directions(o.direction));   // linkpred_structural_exhaustive.h

    // T1: DB open + preload complete. This is the "with DB open" prepay.
    double load_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now() - t_start).count() / 1000000.0;
    std::cerr << "[timing] load_ms=" << load_ms << "\n";

    // Resolve emb_dim for CSV (pvector learned it above; cursor probes here)
    size_t emb_dim = is_embedding ? (pv_mode ? preload_dim : 0) : 0;
    if (is_embedding && !pv_mode) {
        prop_blob probe = g->get_node_properties(0);
        emb_dim = probe.size / sizeof(float);
        if (probe.data) delete[] const_cast<uint8_t*>(probe.data);
    }
    std::string graph_type_str =
        (opts.type == GraphType::SplitEKey) ? "split_ekey" : "adj";
    std::string db_path_str = opts.db_dir + "/" + opts.db_name;

    // ── Startup banner: config dump, once, before any timer ──
    std::cerr << "[config]"
              << " impl="       << o.impl
              << " method="     << o.method
              << " candidates=" << (o.candidates.empty() ? "exhaustive" : o.candidates)
              << " variant="    << o.variant
              << " threads="    << o.threads
              << " trials="     << o.num_trials
              << " warmup="     << o.warmup
              << " topk="       << o.topk;
    if (o.candidates == "khop")
        std::cerr << " k=" << o.k_hops;
    if (o.candidates == "uniformRandom")
        std::cerr << " samples=" << o.samples << " seed=" << o.seed;
    std::cerr << " db=" << db_path_str << "\n";

    // ---- Warm-up (--warmup=N): not counted in results, not written to any CSV ----
    // Fills the DB / CPU caches so the trials that follow are "warm".
    for (int w = 0; w < o.warmup; ++w) {
        auto w0 = std::chrono::high_resolution_clock::now();
        run_selected(g, o, nodes, all_emb, csr);
        double wms = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::high_resolution_clock::now() - w0).count() / 1000000.0;
        std::cerr << "[warmup " << (w + 1) << "/" << o.warmup << "] time_ms=" << wms
                  << " (not recorded)\n";
    }

    // ---- Trial loop ----
    for (int trial = 0; trial < o.num_trials; ++trial) {
        // cache=cold: the first algorithm run in this process (no warm-up before it).
        //   The DB cache starts empty, but the OS page cache may still hold the DB
        //   files from an earlier process; for a fully cold run, drop OS caches first.
        // cache=warm: a warm-up or an earlier trial already ran in this process.
        const char *cache = (o.warmup == 0 && trial == 0) ? "cold" : "warm";

        // ── Per-trial progress, before the timer ──
        std::cerr << "[trial " << (trial + 1) << "/" << o.num_trials << "]"
                  << " cache="   << cache
                  << " impl="    << o.impl
                  << " variant=" << o.variant << "\n";

        rusage ru0 = rusage_now();                               // memory / IO before
        auto t0 = std::chrono::high_resolution_clock::now();    // algo timer start

        linkpred::g_missing_embedding_pairs = 0;                   // per trial
        linkpred::g_last_tie_stats = {};                           // filled by merge_topk
        RunResult r = run_selected(g, o, nodes, all_emb, csr);

        auto t1 = std::chrono::high_resolution_clock::now();
        TrialMem mem = trial_mem(ru0, rusage_now());           // memory / IO after
        double ms = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        t1 - t0).count() / 1000000.0;
        double total_ms = load_ms + ms;

        print_result(o, r, ms, load_ms, total_ms, cache);
        if (size_t miss = linkpred::g_missing_embedding_pairs.load())
            std::cerr << "WARNING: " << miss << " of " << r.pairs_scored
                      << " pairs had no embedding on one side (scored 0)\n";

        append_result_csv(o.out_csv, o, db_path_str, opts.db_name,
                          graph_type_str, emb_dim, trial, r, ms, load_ms, total_ms, cache, mem);
        append_topk_csv(o.topk_out, o, db_path_str, opts.db_name,
                        graph_type_str, trial, r);
    }

    // ---- Cleanup ----
    for (auto *h : g_thread_handles) if (h) h->close(false);
    g->close(false);
    engine.close_graph();
    return 0;
}