// fg_linkpred_bench_reference.cpp — simplified runner for linkpred_reference.h variants.
//
// Purpose: run every reference variant that applies to the chosen method on
// one graph, and compare their results. This is a research / sanity-check
// tool, not a benchmark. It does not:
//   - spawn threads
//   - run trials
//   - write CSVs
//   - respect --threads, --accum, --num-trials, --warmup, --out, --topk-out
//
// What it does:
//   - parses the same flags as fg_linkpred_bench_db (so the same commands
//     work, minus the flags above)
//   - opens the DB read-only on one handle
//   - preloads embeddings if --method is cosine or dot (for the pvector
//     variant; harmless for the other embedding variants)
//   - runs every applicable reference variant, timed
//   - prints a comparison table
//   - exits 1 if any variant disagrees with the first one (pairs, edges,
//     top-k)
//
// --variant=X restricts the run to a single variant (default "all").
// (The old --ref-variant=X still works; parse_lp_options maps it.)
// --impl defaults to reference here; any other --impl is an error.
// Restricting to one
// variant is useful for profiling: running one variant per process avoids
// cross-variant cache warming (e.g. preloaded_db following cursor would see
// warm pages it didn't pay to load). For gprof, one variant per process is
// essential: gmon.out would otherwise mix samples from every variant run in
// the same process.
//
// What "applicable" means:
//   --method=cn|jaccard       → structural (--variant=cursor),
//                               structural_preloaded_ids (--variant=preloaded_ids),
//                               structural_preloaded_adj (--variant=preloaded_adj),
//                               structural_tiled<T> (--variant=tiled --tile=T)
//   --method=cosine|dot       → cursor, cursor_cached, preloaded_db, pvector
//   --candidates=fof          → fof_structural only  (run_reference_fof_structural_cursor)
//   --candidates=khop --khop=N→ khop<N>_structural only (run_reference_khop_structural_cursor_bfs)
//   cosine|dot + fof / khop   → fof_embedding / khop<N>_embedding (run_reference_*_embedding)
//
// Exit code:
//   0 = all variants agree, or only one variant ran (nothing to compare)
//   1 = a variant disagrees with the baseline (the first to run)
//   2 = setup error (couldn't open DB, etc.)

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "command_line.h"
#include "common_defs.h"
#include "graph_engine.h"

// #include "linkpred_exhaustive.h"   // pre-split original (2026-11-17)
#include "linkpred_embedding_exhaustive.h"   // preload_embeddings only
                                   // if linkpred_reference.h ever grows an
                                   // embedding preload helper, switch to it
                                   // and drop this include.
#include "linkpred_options.h"
#include "linkpred_reference.h"
#include "linkpred_types.h"

// ---------------------------------------------------------------- helpers

struct RunRow {
    std::string name;
    RunResult   result;
    double      time_ms;
};

static void print_header()
{
    printf("%-24s %10s %10s %8s %10s %8s\n",
           "variant", "pairs", "true_edges", "top-k", "time_ms", "status");
    printf("%-24s %10s %10s %8s %10s %8s\n",
           "-------", "-----", "----------", "-----", "-------", "------");
}

static void print_row(const RunRow &r)
{
    printf("%-24s %10zu %10zu %8zu %10.1f %8s\n",
           r.name.c_str(),
           r.result.pairs_scored,
           r.result.true_edges,
           r.result.top_k.size(),
           r.time_ms,
           "OK");
}

// Compare against the baseline. Returns "" on success, error description on failure.
// Pairs (u, v) and labels must match exactly. Scores are compared with a
// small relative tolerance, because cosine_from_bytes and cosine use
// different internal accumulations — same math, possible float-repr drift.
static std::string compare(const RunRow &baseline, const RunRow &other)
{
    if (baseline.result.pairs_scored != other.result.pairs_scored)
        return "pairs_scored mismatch";
    if (baseline.result.true_edges != other.result.true_edges)
        return "true_edges mismatch";
    if (baseline.result.top_k.size() != other.result.top_k.size())
        return "top_k size mismatch";

    for (size_t i = 0; i < baseline.result.top_k.size(); ++i) {
        const auto &a = baseline.result.top_k[i];
        const auto &b = other.result.top_k[i];
        if (a.u != b.u || a.v != b.v)
            return "top_k[" + std::to_string(i) + "] pair mismatch";
        if (a.label != b.label)
            return "top_k[" + std::to_string(i) + "] label mismatch";
        double scale = std::max(1.0, std::fabs((double)a.score));
        if (std::fabs((double)a.score - (double)b.score) / scale > 1e-5)
            return "top_k[" + std::to_string(i) + "] score mismatch";
    }
    return "";
}

template <typename Fn>
static void timed_run(std::vector<RunRow> &rows, const std::string &name, Fn fn)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    RunResult result = fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    rows.push_back({name, std::move(result), ms});
}

// ---------------------------------------------------------------- main

int main(int argc, char *argv[])
{
    Options o;
    std::vector<char *> clean_argv;
    if (!parse_lp_options(argc, argv, o, clean_argv, "reference")) return 2;
    if (o.impl != "reference") {
        fprintf(stderr, "ERROR: the reference runner only runs --impl=reference "
                "(got --impl=%s); use fg_linkpred_bench_db for direct / pipeline\n",
                o.impl.c_str());
        return 2;
    }

    CmdLineApp cli((int)clean_argv.size(), clean_argv.data());
    if (!cli.parse_args()) return 2;
    cmdline_opts opts = cli.get_parsed_opts();
    opts.read_only  = true;
    opts.create_new = false;
    if (!o.cache_size.empty())                                  // --cache_size -> WT conn config
        opts.conn_config += (opts.conn_config.empty() ? "" : ",") + ("cache_size=" + o.cache_size);

    // Match fg_linkpred_bench_db: embedding methods need the node_props
    // table opened, or get_node_properties() returns empty blobs and every
    // cosine comes out 0.0 (silent false-pass for the reference comparison).
    if (method_is_embedding(o)) {
        opts.has_node_props = true;
        opts.prop_mode      = EMBEDDED;
        fprintf(stderr, "[ref] embedding mode: reading node_props as blobs\n");
    }

    // One handle is enough — the references are single-threaded.
    GraphEngine engine(1, opts);
    // --checkpoint=last (default): latest unnamed checkpoint, nothing written;
    // named: old path, one never-dropped named checkpoint per run (see bench_db).
    std::string ckpt = (o.checkpoint == "named") ? engine.make_checkpoint()
                                                 : std::string("WiredTigerCheckpoint");
    GraphBase *g = engine.create_ro_graph_handle(ckpt);

    // Preload embeddings for embedding methods. Needed by the pvector
    // variant; harmless for the cursor variants (they ignore all_emb).
    std::vector<node_id_t> nodes;
    pvector<std::vector<float>> all_emb;
    if (method_is_embedding(o)) {
        preload_embeddings(g, nodes, all_emb);
        if (all_emb.empty()) {
            fprintf(stderr,
                "WARNING: no embeddings preloaded — embedding variants "
                "will produce empty results\n");
        }
    }

    printf("=== Reference variants comparison ===\n");
    printf("graph:   %s\n", opts.db_name.c_str());
    printf("method:  %s\n", o.method.c_str());
    printf("topk:    %d\n", o.topk);
    if (o.variant != "all")
        printf("variant: %s (only)\n", o.variant.c_str());
    if (method_is_embedding(o))
        printf("nodes:   %zu (preloaded with embeddings)\n", nodes.size());
    printf("\n");

    std::vector<RunRow> rows;

    // want(variant) = true when --variant=all (default) or --variant=<variant>.
    // Table names stay as before (structural, structural_preloaded_ids, ...).
    // Used below to gate each timed_run, so a single-variant run for
    // profiling touches only that variant's code path.
    auto want = [&](const char *name) {
        return o.variant == "all" || o.variant == name;
    };

    // --candidates=fof / khop: run that candidate reference only. The
    // exhaustive variants score a different pair set, so comparing against
    // them would always mismatch. (fof: --variant=cursor|wedge|all. khop structural: cursor (BFS)|outcursor_bfs|nested|outcursor_nested|wedge|all;
    // khop embedding: cursor (BFS)|nested|outcursor_nested|nested_cached|all. nested* and wedge are k = 2 only.)
    if (o.candidates == "uniformRandom") {
        std::string nm = std::string("uniformRandom") + std::to_string(o.samples)
                         + (method_is_structural(o) ? "_structural" : "_embedding");
        timed_run(rows, nm, [&]{
            return method_is_structural(o)
                ? run_reference_uniformRandom_structural_cursor(g, o, o.samples, (uint64_t)o.seed)
                : run_reference_uniformRandom_embedding_cursor(g, o, o.samples, (uint64_t)o.seed);
        });
    } else if (!method_is_structural(o) && o.candidates == "fof") {
        timed_run(rows, "fof_embedding", [&]{
            return run_reference_fof_embedding_cursor(g, o);
        });
    } else if (!method_is_structural(o) && o.candidates == "khop") {
        // cursor = BFS (any k); nested* are k = 2 only.
        const bool k2 = (o.k_hops == 2);
        if (want("cursor"))
            timed_run(rows, "khop" + std::to_string(o.k_hops) + "_embedding", [&]{
                return run_reference_khop_embedding_cursor_bfs(g, o); });
        if (k2 && want("nested"))
            timed_run(rows, "khop2_embedding_nested", [&]{
                return run_reference_khop_embedding_nested(g, o); });
        if (k2 && want("outcursor_nested"))
            timed_run(rows, "khop2_embedding_outcursor_nested", [&]{
                return run_reference_khop_embedding_outcursor_nested(g, o); });
        if (k2 && want("nested_cached"))
            timed_run(rows, "khop2_embedding_nested_cached", [&]{
                return run_reference_khop_embedding_nested_cached(g, o); });
    } else if (o.candidates == "fof") {
        if (want("cursor")) {
            timed_run(rows, "fof_structural", [&]{
                return run_reference_fof_structural_cursor(g, o);
            });
        }
        if (want("wedge")) {
            timed_run(rows, "fof_structural_wedge", [&]{
                return run_reference_fof_structural_wedge(g, o);
            });
        }
    } else if (o.candidates == "khop") {
        if (want("cursor")) {
            timed_run(rows, "khop" + std::to_string(o.k_hops) + "_structural", [&]{
                return run_reference_khop_structural_cursor_bfs(g, o);
            });
        }
        if (want("outcursor_bfs")) {
            timed_run(rows, "khop" + std::to_string(o.k_hops) + "_structural_outcursor_bfs", [&]{
                return run_reference_khop_structural_outcursor_bfs(g, o);
            });
        }
        // nested* and wedge are k = 2 only
        if (want("nested") && o.k_hops == 2) {
            timed_run(rows, "khop2_structural_nested", [&]{
                return run_reference_khop_structural_nested(g, o);
            });
        }
        if (want("outcursor_nested") && o.k_hops == 2) {
            timed_run(rows, "khop2_structural_outcursor_nested", [&]{
                return run_reference_khop_structural_outcursor_nested(g, o);
            });
        }
        if (want("wedge") && o.k_hops == 2) {   // wedge counting is k = 2 only
            timed_run(rows, "khop2_structural_wedge", [&]{
                return run_reference_khop_structural_wedge(g, o);
            });
        }
    } else if (method_is_structural(o)) {
        if (want("cursor")) {
            timed_run(rows, "structural", [&]{
                return run_reference_exhaustive_structural_cursor(g, o);
            });
        }
        if (want("preloaded_ids")) {
            timed_run(rows, "structural_preloaded_ids", [&]{
                return run_reference_exhaustive_structural_preloaded_ids(g, o);
            });
        }
        if (want("preloaded_adj")) {
            timed_run(rows, "structural_preloaded_adj", [&]{
                return run_reference_exhaustive_structural_csr(g, o);
            });
        }
        if (want("tiled")) {
            timed_run(rows, "structural_tiled" + std::to_string(o.tile), [&]{
                return run_reference_exhaustive_structural_tiled(g, o);
            });
        }
        // #NOTE: Old code for testing no reset and cursor (preserved for now)
        // timed_run(rows, "structural_no_reset", [&]{
        //     return run_reference_exhaustive_structural_no_reset(g, o);
        // });
        // timed_run(rows, "structural_fresh_cursor", [&]{
        //     return run_reference_exhaustive_structural_fresh_cursor(g, o);
        // });
    } else {
        // embedding method — run all four variants (or one, if --variant=X).
        if (want("cursor")) {
            timed_run(rows, "cursor", [&]{
                return run_reference_exhaustive_embedding_cursor(g, o);
            });
        }
        if (want("cursor_cached")) {
            timed_run(rows, "cursor_cached", [&]{
                return run_reference_exhaustive_embedding_cursor_cached(g, o);
            });
        }
        if (want("preloaded_db")) {
            timed_run(rows, "preloaded_db", [&]{
                return run_reference_exhaustive_embedding_preloaded_db(g, o);
            });
        }
        if (want("pvector")) {
            timed_run(rows, "pvector", [&]{
                return run_reference_exhaustive_embedding_pvector(g, nodes, all_emb, o);
            });
        }
        if (want("tiled")) {
            timed_run(rows, "embedding_tiled" + std::to_string(o.tile), [&]{
                return run_reference_exhaustive_embedding_tiled(g, o);
            });
        }
    }

    if (rows.empty()) {
        fprintf(stderr, "ERROR: no reference variants applicable for this method "
                "(and --variant=%s matched nothing)\n", o.variant.c_str());
        g->close(false);
        engine.close_graph();
        return 2;
    }

    print_header();
    for (const auto &row : rows) print_row(row);

    bool all_ok = true;
    for (size_t i = 1; i < rows.size(); ++i) {
        std::string err = compare(rows[0], rows[i]);
        if (!err.empty()) {
            printf("\nMISMATCH: %s vs %s — %s\n",
                   rows[0].name.c_str(), rows[i].name.c_str(), err.c_str());
            all_ok = false;
        }
    }

    if (all_ok) {
        if (rows.size() == 1) {
            printf("\nOnly 1 variant ran — nothing to compare.\n");
        } else {
            printf("\nAll %zu variants agree on pairs_scored, true_edges, top_k.\n",
                   rows.size());
        }
    } else {
        printf("\nFAIL — one or more variants disagree with the baseline.\n");
    }

    g->close(false);
    engine.close_graph();
    return all_ok ? 0 : 1;
}