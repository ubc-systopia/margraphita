// linkpred_options.h -- command-line flags for the link-prediction benchmark.
//
// Two binaries share this struct:
//   fg_linkpred_bench_db         threaded benchmark (direct or pipeline impl)
//   fg_linkpred_bench_reference  single-threaded reference runner
//
// THREE orthogonal axes pick which code runs:
//
//   --impl=direct|pipeline|reference      execution engine
//   --variant=NAME                        data-access pattern
//   --candidates=exhaustive|fof|khop|uniformRandom   pair-generation strategy
//
// --variant=all resolves before capability lookup:
//   bench: cursor for structural, pvector for embedding
//   ref:   run every applicable variant and compare
//
// Candidate parameters:
//   --khop=N        hop count for --candidates=khop (default 2).
//                   A parameter, not an axis. Does NOT enable khop on its
//                   own -- --candidates=khop is the only way to pick khop.
//                   Warning fires if --khop is set without --candidates=khop.
//   --samples=K     ids drawn per u for --candidates=uniformRandom (default 100)
//   --seed=S        RNG seed for --candidates=uniformRandom (default 42)
//
// Deprecated aliases (still accepted):
//   --reference              --> --impl=reference
//   --variation=X            --> --variant=X
//   --ref-variant=X          --> --variant=X  (structural names mapped too)
//   --k=N                    --> --khop=N
//
// The impl_matrix below is the single source of truth for which
// (impl, family, variant, candidates) combinations exist.

#pragma once

#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <vector>


// ================================================================
// Options
// ================================================================

struct Options {
    // ---- shared: read by both binaries ----
    std::string method  = "cn";   // --method:  cn | jaccard | cosine | dot
    int         topk    = 10;     // --topk

    // ---- engine selection (both binaries; ref forces impl=reference) ----
    std::string impl    = "direct"; // --impl:     direct | pipeline | reference
    std::string variant = "all";    // --variant:  all | cursor | csr | csr_cursor | tiled | wedge | outcursor_wedge | preloaded_ids | preloaded_adj | outcursor_bfs | nested | outcursor_nested | nested_cached
                                    //            | cursor_cached | preloaded_db | pvector

    // ---- candidates block (structural only; any impl) ----
    // --candidates picks the strategy. --khop configures the khop strategy.
    // --khop alone does NOT enable khop; --candidates=khop does.
    std::string candidates;         // --candidates: "" | exhaustive | fof | khop | uniformRandom
                                    //   ""      = exhaustive for direct/reference;
                                    //             error for pipeline (must be explicit)
    int         k_hops  = 2;        // --khop:     hop count, only used when
                                    //             --candidates=khop (deprecated alias: --k=N)
    int         samples = 100;      // --samples:  ids per u for --candidates=uniformRandom
    int         seed    = 42;       // --seed:     RNG seed for --candidates=uniformRandom

    // ---- tiled-only ----
    int         tile    = 64;       // --tile: u's per block for --variant=tiled; each
                                    //   v is read once per block (DB reads ~ n / tile)

    // ---- pipeline-only ----
    std::string cursor_scope = "thread"; // --cursor-scope: thread | block
    std::string direction = "both";      // --direction: both | out (directed DBs, needs -d)
    std::string eval = "recon";          // --eval: recon | holdout (needs --split-file)
    std::string split_file;              // --split-file: test pairs from tools/make_edge_split.py

    // ---- threading (bench only) ----
    int         threads    = 1;       // --threads
    int         num_trials = 1;       // --num-trials
    int         warmup     = 0;       // --warmup
    std::string cache_size;            // --cache_size: WT cache, e.g. 4GB; empty = the DB's WiredTiger.basecfg value (10GB on our DBs)
    std::string checkpoint = "last";   // --checkpoint: last = read the latest unnamed checkpoint
                                       //   (WiredTigerCheckpoint, nothing written); named = old path,
                                       //   engine.make_checkpoint() adds a named checkpoint per run that
                                       //   is never dropped (DB files and open time grow every run)
    std::string accum      = "local"; // --accum:  local | shared

    // ---- output (bench only) ----
    std::string out_csv;              // --out
    std::string topk_out;             // --topk-out
    std::string out_root;             // --out-root: DIR/<family>/<run type>/{metrics,topk}.csv + logs/<graph>/
    std::string source = "unknown";   // --source
};


// ================================================================
// Method families
// ================================================================

inline bool method_is_structural(const Options &o)
{ return o.method == "cn" || o.method == "jaccard"; }

inline bool method_is_embedding(const Options &o)
{ return o.method == "cosine" || o.method == "dot"; }


// ================================================================
// Impl capability matrix (impl x family x variant x candidates)
// ================================================================
// Every combination the codebase implements. Structural rows carry an
// explicit candidates value. Embedding rows use "" (candidate axis N/A).
//
// Empty --candidates normalizes to "exhaustive" for direct/reference before
// the matrix is consulted. Pipeline requires explicit --candidates.
//
// Adding a new combination: implement the function, wire the dispatch branch,
// add one row here. Forgetting the row fails at runtime (rejection of a combo
// you support) -- loud, not silent. --help auto-updates.

struct ImplCapability {
    const char *impl;
    const char *family;
    const char *variant;
    const char *candidates;   // "" = no candidate axis; explicit otherwise
    const char *function;     // for messages only
};

inline constexpr ImplCapability impl_matrix[] = {
    // ---- direct: linkpred_exhaustive.h + linkpred_fof.h + linkpred_khop.h ----
    {"direct",    "structural", "cursor",        "exhaustive", "run_direct_exhaustive_structural_cursor"},
    {"direct",    "structural", "cursor",        "fof",        "run_direct_fof_structural_cursor"},
    {"direct",    "structural", "wedge",         "fof",        "run_direct_fof_structural_wedge (CN counted along u->w->v)"},
    {"direct",    "structural", "cursor",        "khop",       "run_direct_khop_structural_cursor_bfs (reads --khop=N)"},
    {"direct",    "structural", "wedge",         "khop",       "run_direct_khop_structural_wedge (k = 2 only)"},
    {"direct",    "structural", "outcursor_wedge", "fof",      "run_direct_fof_structural_wedge, N(u) / N(w) via OutCursor"},
    {"direct",    "structural", "outcursor_wedge", "khop",     "run_direct_khop_structural_wedge, OutCursor reads (k = 2 only)"},
    {"direct",    "structural", "wedge_compact", "fof",        "run_direct_fof_structural_wedge_compact (arrays by dense node index, blocks over nodes)"},
    {"direct",    "structural", "wedge_compact", "khop",       "run_direct_khop_structural_wedge_compact (k = 2 only)"},
    {"direct",    "structural", "outcursor_wedge_compact", "fof",  "run_direct_fof_structural_wedge_compact, OutCursor reads"},
    {"direct",    "structural", "outcursor_wedge_compact", "khop", "run_direct_khop_structural_wedge_compact, OutCursor reads (k = 2 only)"},
    {"direct",    "structural", "csr",           "exhaustive", "preload_csr + run_direct_exhaustive_structural_csr"},
    {"direct",    "structural", "csr",           "fof",        "preload_csr + run_direct_fof_structural_csr_wedge (wedge over in-memory adjacency)"},
    {"direct",    "structural", "csr",           "khop",       "preload_csr + run_direct_khop_structural_csr_wedge (k = 2 only)"},
    {"direct",    "structural", "csr_cursor",    "fof",        "preload_csr + run_direct_fof_structural_csr_cursor (cursor algorithm, in-memory adjacency)"},
    {"direct",    "structural", "csr_cursor",    "khop",       "preload_csr + run_direct_khop_structural_csr_cursor (reads --khop=N)"},
    {"direct",    "embedding",  "csr",           "fof",        "preload_csr + preload_embeddings + run_direct_fof_embedding_csr (no DB reads)"},
    {"direct",    "embedding",  "csr",           "khop",       "preload_csr + preload_embeddings + run_direct_khop_embedding_csr (reads --khop=N)"},
    {"direct",    "structural", "tiled",         "exhaustive", "run_direct_exhaustive_tiled (reads --tile=N)"},
    {"direct",    "embedding",  "cursor",        "",           "run_direct_exhaustive_embedding_cursor"},
    {"direct",    "embedding",  "pvector",       "",           "run_direct_exhaustive_embedding_pvector"},
    {"direct",    "embedding",  "tiled",         "",           "run_direct_exhaustive_tiled (reads --tile=N)"},
    {"direct",    "embedding",  "cursor",        "fof",        "run_direct_fof_embedding_cursor (v's embedding per candidate)"},
    {"direct",    "embedding",  "cursor",        "khop",       "run_direct_khop_embedding_cursor_bfs (reads --khop=N)"},
    {"direct",    "structural", "outcursor_nested", "khop",    "run_direct_khop_structural_outcursor_nested (k = 2 only)"},
    {"direct",    "embedding",  "outcursor_nested", "khop",    "run_direct_khop_embedding_outcursor_nested (k = 2 only)"},

    // ---- pipeline: linkpred_pipeline.h + linkpred_candidates.h ----
    {"pipeline",  "structural", "cursor",        "exhaustive", "run_pipeline_structural + ExhaustiveCandidates"},
    {"pipeline",  "structural", "cursor",        "fof",        "run_pipeline_structural + FofCandidates"},
    {"pipeline",  "structural", "cursor",        "khop",       "run_pipeline_structural + KHopCandidates (reads --khop=N)"},
    {"pipeline",  "embedding",  "cursor",        "exhaustive", "run_pipeline_embedding_cursor + ExhaustiveCandidates"},
    {"pipeline",  "embedding",  "cursor",        "fof",        "run_pipeline_embedding_cursor + FofCandidates"},
    {"pipeline",  "embedding",  "cursor",        "khop",       "run_pipeline_embedding_cursor + KHopCandidates (reads --khop=N)"},
    {"pipeline",  "embedding",  "pvector",       "exhaustive", "run_pipeline_embedding_pvector + ExhaustiveCandidates (preloaded)"},
    {"pipeline",  "embedding",  "pvector",       "fof",        "run_pipeline_embedding_pvector + FofCandidates (preloaded)"},
    {"pipeline",  "embedding",  "pvector",       "khop",       "run_pipeline_embedding_pvector + KHopCandidates (preloaded)"},

    // ---- reference: linkpred_reference.h (single-threaded) ----
    {"reference", "structural", "cursor",        "exhaustive", "run_reference_exhaustive_structural_cursor"},
    {"reference", "structural", "cursor",        "fof",        "run_reference_fof_structural_cursor"},
    {"reference", "structural", "wedge",         "fof",        "run_reference_fof_structural_wedge"},
    {"reference", "structural", "cursor",        "khop",       "run_reference_khop_structural_cursor_bfs (reads --khop=N)"},
    {"reference", "structural", "wedge",         "khop",       "run_reference_khop_structural_wedge (k = 2 only)"},
    {"reference", "structural", "outcursor_bfs", "khop",       "run_reference_khop_structural_outcursor_bfs (reads --khop=N)"},
    {"reference", "structural", "nested",        "khop",       "run_reference_khop_structural_nested (k = 2 only)"},
    {"reference", "structural", "outcursor_nested", "khop",    "run_reference_khop_structural_outcursor_nested (k = 2 only)"},
    {"reference", "structural", "cursor",        "uniformRandom", "run_reference_uniformRandom_structural_cursor (reads --samples, --seed)"},
    {"reference", "structural", "preloaded_ids", "exhaustive", "run_reference_exhaustive_structural_preloaded_ids"},
    {"reference", "structural", "preloaded_adj", "exhaustive", "run_reference_exhaustive_structural_csr"},
    {"reference", "structural", "tiled",         "exhaustive", "run_reference_exhaustive_structural_tiled (reads --tile=N)"},
    {"reference", "embedding",  "cursor",        "",           "run_reference_exhaustive_embedding_cursor"},
    {"reference", "embedding",  "cursor",        "fof",        "run_reference_fof_embedding_cursor"},
    {"reference", "embedding",  "cursor",        "khop",       "run_reference_khop_embedding_cursor_bfs (reads --khop=N)"},
    {"reference", "embedding",  "nested",        "khop",       "run_reference_khop_embedding_nested (k = 2 only)"},
    {"reference", "embedding",  "outcursor_nested", "khop",    "run_reference_khop_embedding_outcursor_nested (k = 2 only)"},
    {"reference", "embedding",  "nested_cached", "khop",       "run_reference_khop_embedding_nested_cached (k = 2 only)"},
    {"reference", "embedding",  "cursor",        "uniformRandom", "run_reference_uniformRandom_embedding_cursor (reads --samples, --seed)"},
    {"reference", "embedding",  "cursor_cached", "",           "run_reference_exhaustive_embedding_cursor_cached"},
    {"reference", "embedding",  "preloaded_db",  "",           "run_reference_exhaustive_embedding_preloaded_db"},
    {"reference", "embedding",  "pvector",       "",           "run_reference_exhaustive_embedding_pvector"},
    {"reference", "embedding",  "tiled",         "",           "run_reference_exhaustive_embedding_tiled (reads --tile=N)"},
};

inline bool find_capability(const std::string &impl,
                             const std::string &family,
                             const std::string &variant,
                             const std::string &candidates)
{
    for (const auto &c : impl_matrix)
        if (impl       == c.impl       &&
            family     == c.family     &&
            variant    == c.variant    &&
            candidates == c.candidates)
            return true;
    return false;
}

// fof / khop candidates: embeddings only have the cursor (lookup) scoring there,
// so --variant=all resolves to cursor instead of pvector.
inline bool candidates_fof_or_khop(const Options &o)
{
    return o.candidates == "fof" || o.candidates == "khop";
}

// Any non-exhaustive candidate set (fof, khop, uniformRandom): cursor scoring only.
inline bool candidates_not_exhaustive(const Options &o)
{
    return candidates_fof_or_khop(o) || o.candidates == "uniformRandom";
}

inline void print_capabilities_for(const std::string &impl,
                                    const std::string &family)
{
    for (const auto &c : impl_matrix) {
        if (impl == c.impl && family == c.family) {
            const char *cand = (*c.candidates) ? c.candidates : "(n/a)";
            fprintf(stderr, "         --variant=%-16s --candidates=%-12s  %s\n",
                    c.variant, cand, c.function);
        }
    }
}

inline void print_capability_matrix()
{
    fprintf(stderr, "\n impl / family -> variant x candidates matrix:\n");
    fprintf(stderr, " %-10s %-12s %-16s %-12s %s\n",
            "impl", "family", "variant", "candidates", "function");
    fprintf(stderr, " %-10s %-12s %-16s %-12s %s\n",
            "----------", "------------", "----------------", "------------", "--------");
    for (const auto &c : impl_matrix) {
        const char *cand = (*c.candidates) ? c.candidates : "(n/a)";
        fprintf(stderr, " %-10s %-12s %-16s %-12s %s\n",
                c.impl, c.family, c.variant, cand, c.function);
    }
    fprintf(stderr,
        "\n notes:\n"
        "   --khop=N is a parameter for --candidates=khop (default 2); it does\n"
        "     not enable khop on its own\n"
        "   --impl=pipeline requires --candidates=exhaustive|fof|khop (no default)\n"
        "   --impl=direct and --impl=reference default to --candidates=exhaustive\n"
        "   reference: fof with --variant=cursor|wedge; khop with cursor|outcursor_bfs|nested|outcursor_nested|wedge\n"
        "              (embedding: cursor|nested|outcursor_nested|nested_cached); nested* and wedge are k = 2 only\n"
        "   embedding rows ignore --candidates (always exhaustive enumeration)\n");
}


// ================================================================
// --help
// ================================================================

inline void print_lp_usage()
{
    fprintf(stderr,
        "Link-prediction flags (plus DB flags: -p <dir> -m <name> -g adj -r ...):\n"
        "\n"
        " shared:\n"
        "  --method=cn|jaccard|cosine|dot    scoring (cn/jaccard structural, cosine/dot embedding)\n"
        "  --topk=K                          keep the K best pairs (default 10)\n"
        "\n"
        " execution engine and variant:\n"
        "  --impl=direct|pipeline|reference  direct    = hand-written threaded (default)\n"
        "                                    pipeline  = threaded + generator\n"
        "                                    reference = single-threaded (linkpred_reference.h)\n"
        "  --variant=NAME                    data-access pattern; valid names depend on\n"
        "                                    impl + method family (see matrix below).\n"
        "                                    all = family default (cursor for structural,\n"
        "                                          pvector for embedding); ref runner: run\n"
        "                                          every applicable variant\n"
        "\n"
        " candidates (structural only):\n"
        "  --candidates=exhaustive|fof|khop|uniformRandom  pair-generation strategy\n"
        "                                    uniformRandom: reference only; --samples ids per u\n"
        "                                    drawn uniformly from (u, max_id] (PyG/OGB-style)\n"
        "                                    direct:    default is exhaustive\n"
        "                                    pipeline:  required (no default)\n"
        "                                    reference: default is exhaustive; fof / khop\n"
        "                                               need --variant=cursor (or all)\n"
        "  --khop=N                          hop count when --candidates=khop (default 2).\n"
        "                                    Parameter only -- does not enable khop on its own.\n"
        "  --samples=K                       ids per u for --candidates=uniformRandom (default 100)\n"
        "  --seed=S                          RNG seed for --candidates=uniformRandom (default 42)\n"
        "  --cursor-scope=thread|block       pipeline only: cursor lifetime (default thread)\n"
        "  --direction=both|out              directed DBs (open with -d): both = neighbours are\n"
        "                                    out- and in-lists, scored as undirected (default);\n"
        "                                    out = out-lists only (before 2026-11-22)\n"
        "  --eval=recon|holdout              recon = labels are edges of the graph (default);\n"
        "                                    holdout = test pairs from --split-file are hidden\n"
        "                                    from every list, label 1 = test pair, training\n"
        "                                    edges are not scored (fof / khop, direct only)\n"
        "  --split-file=FILE                 test pairs (u<TAB>v), tools/make_edge_split.py\n"
        "  --tile=N                          --variant=tiled: u's per block (default 64); each v\n"
        "                                    is read once per block instead of once per pair\n"
        "\n"
        " threading (fg_linkpred_bench_db only):\n"
        "  --threads=N                       worker threads (default 1)\n"
        "  --num-trials=N                    repeat N times (default 1)\n"
        "  --warmup=N                        untimed warmups before trials (default 0)\n"
        "  --cache_size=SIZE                 WiredTiger cache, e.g. 512MB, 4GB (default: the DB's\n"
        "                                    WiredTiger.basecfg value, 10GB on our DBs)\n"
        "  --checkpoint=last|named           RO handles read: last = latest checkpoint, writes nothing\n"
        "                                    (default); named = new named checkpoint per run (old)\n"
        "  --accum=local|shared              where threads keep results (default local)\n"
        "\n"
        " output (fg_linkpred_bench_db only):\n"
        "  --out=FILE                        metrics CSV (appended)\n"
        "  --topk-out=FILE                   top-k pairs CSV (appended)\n"
        "  --out-root=DIR                    write to DIR/<structural|embedding>/<impl>_<candidates>_<variant>/\n"
        "                                    metrics.csv + topk.csv (all graphs, appended) and\n"
        "                                    logs/<graph>/<time>_t<threads>.log (console copy)\n"
        "  --source=NAME                     embedding source label\n"
        "\n"
        " deprecated (still accepted, use the new form):\n"
        "  --reference                       = --impl=reference\n"
        "  --variation=X                     = --variant=X\n"
        "  --ref-variant=X                   = --variant=X (structural -> cursor,\n"
        "                                      structural_preloaded_ids -> preloaded_ids,\n"
        "                                      structural_preloaded_adj -> preloaded_adj)\n"
        "  --k=N                             = --khop=N\n");

    print_capability_matrix();

    fprintf(stderr, "\n  --help                            print this and exit\n");
}


// ================================================================
// Parsing helpers
// ================================================================

inline bool parse_int_flag(const char *flag, const std::string &value, int &out)
{
    try {
        size_t used = 0;
        out = std::stoi(value, &used);
        if (used == value.size()) return true;
    } catch (...) {}
    fprintf(stderr, "ERROR: %s expects an integer, got '%s'\n", flag, value.c_str());
    return false;
}

inline bool one_of(const char *flag, const std::string &value,
                   std::initializer_list<const char *> allowed)
{
    for (const char *a : allowed) if (value == a) return true;
    std::string list;
    for (const char *a : allowed) list += (list.empty() ? "" : ", ") + std::string(a);
    fprintf(stderr, "ERROR: %s must be one of: %s (got '%s')\n",
            flag, list.c_str(), value.c_str());
    return false;
}


// ================================================================
// Validation
// ================================================================

inline bool validate_lp_options(const Options &o)
{
    // ---- scalar value checks ----
    if (!method_is_structural(o) && !method_is_embedding(o)) {
        fprintf(stderr, "ERROR: --method must be cn, jaccard, cosine, or dot\n");
        return false;
    }
    if (!one_of("--impl", o.impl, {"direct", "pipeline", "reference"})) return false;
    if (o.variant != "all" &&
        !one_of("--variant", o.variant,
                {"cursor", "csr", "csr_cursor", "tiled", "wedge", "outcursor_wedge", "wedge_compact", "outcursor_wedge_compact", "preloaded_ids", "preloaded_adj",
                 "cursor_cached", "preloaded_db", "pvector",
                 "outcursor_bfs", "nested", "outcursor_nested", "nested_cached"})) return false;
    if (!one_of("--cursor-scope", o.cursor_scope, {"thread", "block"})) return false;
    if (!one_of("--direction", o.direction, {"both", "out"})) return false;
    if (!one_of("--eval", o.eval, {"recon", "holdout"})) return false;
    if (o.eval == "holdout" && o.split_file.empty()) {
        fprintf(stderr, "ERROR: --eval=holdout needs --split-file=FILE (tools/make_edge_split.py)\n");
        return false;
    }
    if (o.eval == "holdout" && (o.impl != "direct" || (o.candidates != "fof" && o.candidates != "khop"))) {
        fprintf(stderr, "ERROR: --eval=holdout is implemented for --impl=direct with --candidates=fof|khop only\n");
        return false;
    }
    if (!one_of("--accum", o.accum, {"local", "shared"})) return false;
    if (!one_of("--checkpoint", o.checkpoint, {"last", "named"})) return false;
    if (o.k_hops < 1)  { fprintf(stderr, "ERROR: --khop must be >= 1\n");   return false; }
    if (o.tile < 1)    { fprintf(stderr, "ERROR: --tile must be >= 1\n");   return false; }
    if (o.samples < 1) { fprintf(stderr, "ERROR: --samples must be >= 1\n"); return false; }
    if (o.warmup < 0)  { fprintf(stderr, "ERROR: --warmup must be >= 0\n"); return false; }
    if (o.topk   <= 0) { fprintf(stderr, "ERROR: --topk must be > 0\n");    return false; }

    // ---- candidates: value check (only when set) ----
    if (!o.candidates.empty() &&
        !one_of("--candidates", o.candidates, {"exhaustive", "fof", "khop", "uniformRandom"}))
        return false;

    // (embedding methods take --candidates=fof|khop too; the matrix below
    //  rejects the rest; --impl=pipeline takes cosine / dot with cursor or pvector)

    // ---- pipeline: requires explicit candidates (no default) ----
    if (o.impl == "pipeline" && o.candidates.empty()) {
        fprintf(stderr,
            "ERROR: --impl=pipeline requires --candidates=exhaustive|fof|khop\n"
            "       (pipeline is generator-driven; pick one)\n");
        return false;
    }

    // ---- resolve --variant=all for capability check ----
    const std::string family = method_is_structural(o) ? "structural" : "embedding";
    std::string check_variant = o.variant;
    if (check_variant == "all")
        check_variant = (family == "structural" || candidates_not_exhaustive(o)) ? "cursor" : "pvector";

    // ---- resolve empty candidates -> "exhaustive" for direct/reference ----
    std::string check_candidates = o.candidates;
    if (check_candidates.empty() && method_is_structural(o))
        check_candidates = "exhaustive";
    if (check_candidates == "exhaustive" && !method_is_structural(o) && o.impl != "pipeline")
        check_candidates = "";                    // direct/reference embedding rows: "" = exhaustive (pipeline rows name it)

    // ---- matrix lookup ----
    if (!find_capability(o.impl, family, check_variant, check_candidates)) {
        fprintf(stderr,
            "ERROR: no capability for --impl=%s --method=%s (family=%s) "
            "--variant=%s --candidates=%s\n"
            "       Supported combinations for impl=%s, family=%s:\n",
            o.impl.c_str(), o.method.c_str(), family.c_str(),
            o.variant.c_str(),
            check_candidates.empty() ? "(n/a)" : check_candidates.c_str(),
            o.impl.c_str(), family.c_str());
        print_capabilities_for(o.impl, family);
        fprintf(stderr, "       Run --help for the full matrix.\n");
        return false;
    }

    // ---- khop wedge counts CN along u -> w -> v, so it is k = 2 only ----
    if (o.candidates == "khop" && o.k_hops != 2 &&
        (o.variant.find("wedge") != std::string::npos || o.variant.find("nested") != std::string::npos)) {
        fprintf(stderr, "ERROR: --variant=%s with --candidates=khop needs --khop=2 (got %d)\n",
                o.variant.c_str(), o.k_hops);
        return false;
    }

    // ---- soft warning: --khop set without candidates=khop ----
    if (o.k_hops != 2 && o.candidates != "khop") {
        fprintf(stderr,
            "NOTE: --khop=%d only applies with --candidates=khop; ignoring\n",
            o.k_hops);
    }

    return true;
}

// --variant=all -> the family default (cursor for structural, pvector for
// embedding). fg_linkpred_bench_db calls this after parsing because it runs
// one variant; the reference runner keeps "all" (= run every variant).
inline void resolve_variant_all(Options &o)
{
    if (o.variant == "all")
        o.variant = (method_is_structural(o) || candidates_not_exhaustive(o)) ? "cursor" : "pvector";
}

inline void adjust_lp_options(Options &o)
{
    if (o.threads < 1) o.threads = 1;
    if (o.impl == "reference" && o.threads > 1) {
        fprintf(stderr, "NOTE: --impl=reference is single-threaded; ignoring --threads=%d\n",
                o.threads);
        o.threads = 1;
    }
}


// ================================================================
// Parse
// ================================================================
// default_impl: bench passes "direct", ref passes "reference". Overridden by
// --impl=X or the deprecated --reference alias (which forces reference).
//
// --khop=N just sets k_hops. It does NOT change --candidates. If --candidates
// is not "khop", the value sits unused and validate_lp_options warns.

inline bool parse_lp_options(int argc, char *argv[],
                             Options &o,
                             std::vector<char *> &clean_argv,
                             const std::string &default_impl = "direct")
{
    o.impl = default_impl;

    bool impl_explicit      = false;
    bool reference_explicit = false;

    for (int i = 0; i < argc; ++i) {
        std::string a(argv[i]);
        bool ok = true;

        // ---- primary ----
        if      (a.rfind("--method=", 0)       == 0) o.method       = a.substr(9);
        else if (a.rfind("--topk=", 0)         == 0) ok = parse_int_flag("--topk", a.substr(7), o.topk);
        else if (a.rfind("--impl=", 0)         == 0) { o.impl = a.substr(7); impl_explicit = true; }
        else if (a.rfind("--variant=", 0)      == 0) o.variant      = a.substr(10);
        else if (a.rfind("--candidates=", 0)   == 0) o.candidates   = a.substr(13);
        else if (a.rfind("--khop=", 0)         == 0) ok = parse_int_flag("--khop", a.substr(7), o.k_hops);
        else if (a.rfind("--samples=", 0)      == 0) ok = parse_int_flag("--samples", a.substr(10), o.samples);
        else if (a.rfind("--seed=", 0)         == 0) ok = parse_int_flag("--seed", a.substr(7), o.seed);
        else if (a.rfind("--cursor-scope=", 0) == 0) o.cursor_scope = a.substr(15);
        else if (a.rfind("--direction=", 0) == 0)    o.direction = a.substr(12);
        else if (a.rfind("--eval=", 0) == 0)         o.eval = a.substr(7);
        else if (a.rfind("--split-file=", 0) == 0)   o.split_file = a.substr(13);
        else if (a.rfind("--tile=", 0)         == 0) ok = parse_int_flag("--tile", a.substr(7), o.tile);
        else if (a.rfind("--threads=", 0)      == 0) ok = parse_int_flag("--threads", a.substr(10), o.threads);
        else if (a.rfind("--num-trials=", 0)   == 0) ok = parse_int_flag("--num-trials", a.substr(13), o.num_trials);
        else if (a.rfind("--warmup=", 0)       == 0) ok = parse_int_flag("--warmup", a.substr(9), o.warmup);
        else if (a.rfind("--cache_size=", 0)   == 0) o.cache_size   = a.substr(13);
        else if (a.rfind("--checkpoint=", 0)   == 0) o.checkpoint   = a.substr(13);
        else if (a.rfind("--accum=", 0)        == 0) o.accum        = a.substr(8);
        else if (a.rfind("--out=", 0)          == 0) o.out_csv      = a.substr(6);
        else if (a.rfind("--topk-out=", 0)     == 0) o.topk_out     = a.substr(11);
        else if (a.rfind("--out-root=", 0)     == 0) o.out_root     = a.substr(11);
        else if (a.rfind("--source=", 0)       == 0) o.source       = a.substr(9);

        // ---- deprecated aliases ----
        else if (a == "--reference")                 reference_explicit = true;
        else if (a.rfind("--variation=", 0)    == 0) o.variant = a.substr(12);
        else if (a.rfind("--ref-variant=", 0)  == 0) {
            std::string v = a.substr(14);
            if      (v == "structural")               v = "cursor";
            else if (v == "structural_preloaded_ids") v = "preloaded_ids";
            else if (v == "structural_preloaded_adj") v = "preloaded_adj";
            o.variant = v;
        }
        else if (a.rfind("--k=", 0)            == 0) ok = parse_int_flag("--k", a.substr(4), o.k_hops);

        // ---- misc ----
        else if (a == "--help") { print_lp_usage(); std::exit(0); }
        else clean_argv.push_back(argv[i]);
        if (!ok) return false;
    }

    // Resolve --reference alias into impl=reference.
    if (reference_explicit) {
        if (impl_explicit && o.impl != "reference") {
            fprintf(stderr,
                "ERROR: --reference conflicts with --impl=%s "
                "(--reference is a deprecated alias for --impl=reference)\n",
                o.impl.c_str());
            return false;
        }
        o.impl = "reference";
    }

    if (!validate_lp_options(o)) return false;
    adjust_lp_options(o);
    return true;
}