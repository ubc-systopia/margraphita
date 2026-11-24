// linkpred_output.h — stdout summary and CSV writers for one link-prediction run.

#pragma once

#include <cstddef>
#include <sys/resource.h>   // getrusage: per-trial memory / IO columns
#include <unistd.h>         // dup2 / close: --out-root console copy
#include <cstdio>
#include <filesystem>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "linkpred_eval.h"
#include "linkpred_thread_handles.h"   // lp_both_directions (direction column)
#include "linkpred_options.h"
#include "linkpred_types.h"

// CSV "variation" column: which code path ran.
//   cursor / pvector                 original exhaustive path
//   csr                              structural, adjacency preloaded (--variant=csr)
//   + _csr                           direct_exhaustive with --variant=csr
//   tiled<T> / + _tiled<T>           --variant=tiled --tile=T (DB, v read once per block)
//   pipe_exhaustive / pipe_fof /
//   pipe_khop<k>                     --candidates pipeline
//   direct_exhaustive / direct_fof /
//   direct_khop<k>                   --candidates ... --impl=direct (hand-written)
//   + _blk                           --cursor-scope=block (pipeline only)
//   structural_preloaded_ids /
//   structural_preloaded_adj         --impl=reference --variant=preloaded_{ids,adj}
//                                    (cn / jaccard; same labels as before the rename)
//   cursor_cached / preloaded_db     --impl=reference embedding variants
//   fof / khop<k>                    --impl=reference --candidates=fof|khop
//   + _ref                           --impl=reference run
//   + _shared                        --accum=shared run
inline std::string variation_label(const Options &o)
{
    const bool ref = o.impl == "reference";
    std::string v;
    if (ref && (o.candidates == "fof" || o.candidates == "khop"))
        v = o.candidates + (o.candidates == "khop" ? std::to_string(o.k_hops) : "")
          + (o.variant == "wedge" ? "_wedge" : "");   // fof_ref / fof_wedge_ref / khop<k>_ref
    else if (!o.candidates.empty() && !ref)
        v = (o.impl == "direct" ? "direct_" : "pipe_")
          + o.candidates + (o.candidates == "khop" ? std::to_string(o.k_hops) : "")
          + (o.impl == "pipeline" && o.cursor_scope == "block" ? "_blk" : "")
          + (o.variant == "csr" ? "_csr" : "")
          + (o.variant == "wedge" ? "_wedge" : "")
          + (o.variant == "tiled" ? "_tiled" + std::to_string(o.tile) : "");
    else if (o.variant == "tiled")
        v = "tiled" + std::to_string(o.tile);    // tiled<tile>, tiled<tile>_ref
    else if (ref && method_is_structural(o) && o.variant != "cursor")
        v = "structural_" + o.variant;           // structural_preloaded_{ids,adj}_ref
    else
        v = o.variant;                           // cursor | pvector | cursor_cached | preloaded_db
    if (ref) v += "_ref";
    else if (o.accum == "shared") v += "_shared";
    return v;
}

// cache: "cold" (first algorithm run in this process) or "warm" — see main.
inline void print_result(const Options &o, const RunResult &r,
                         double time_ms, double load_ms, double total_ms,
                         const char *cache)
{
    double pk = linkpred::precision_at_k(r.top_k, (size_t)o.topk);
    double rk = linkpred::recall_at_k(r.top_k, (size_t)o.topk, r.true_edges);

    size_t top_k_positives = 0;
    for (const auto &sp : r.top_k) if (sp.label == 1) top_k_positives++;

    std::cout << "LinkPredDB method=" << o.method
              << " threads=" << o.threads
              << " reference=" << (o.impl == "reference" ? 1 : 0)   // 1 = linkpred_reference.h, not timed path
              << " accum=" << (o.impl == "reference" ? "none" : o.accum)
              << " candidates=" << (o.candidates.empty() ? std::string("original")
                                    : o.candidates + (o.candidates == "khop" ? std::to_string(o.k_hops) : ""))
              << " variant=" << o.variant          // resolved: cursor | wedge | csr | tiled | pvector | ...
              << " pairs_scored=" << r.pairs_scored
              << " true_edges=" << r.true_edges
              << " k=" << o.topk
              << " P@" << o.topk << "=" << pk
              << " R@" << o.topk << "=" << rk
              << " P@" << o.topk << "_tie_aware=" << linkpred::precision_at_k_tie_aware(r.top_k, (size_t)o.topk, linkpred::g_last_tie_stats)
              << " eval=" << o.eval
              << " ties_at_cutoff=" << (linkpred::g_last_tie_stats.valid ? std::to_string(linkpred::g_last_tie_stats.at_cutoff) : std::string("n/a"))
              << " top_k_positives=" << top_k_positives
              << " kept=" << r.top_k.size()
              << " time_ms=" << time_ms            // algorithm only ("without DB")
              << " load_ms=" << load_ms            // DB open + preload ("with DB" overhead)
              << " total_ms=" << total_ms          // load + algo ("with DB open")
              << " impl=" << (o.candidates.empty() && o.impl == "direct" ? "original" : o.impl)
              << " cursor_scope=" << (o.impl == "pipeline" ? o.cursor_scope : "n/a")
              << " cache=" << cache                // cold | warm
              << " warmup=" << o.warmup            // untimed runs before the trials
              << " cache_size=" << (o.cache_size.empty() ? "default" : o.cache_size)
              << "\n";

    std::cout << "Top-" << r.top_k.size() << ":\n";
    for (const auto &sp : r.top_k)
        std::cout << "  (" << sp.u << ", " << sp.v << ")"
                  << "  score=" << std::fixed << std::setprecision(4) << sp.score
                  << "  label=" << sp.label << "\n";
}

inline std::string now_str() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

// First line of a file ("" if missing or empty).
inline std::string first_line(const std::string &path)
{
    std::ifstream in(path);
    std::string line;
    if (in.good()) std::getline(in, line);
    return line;
}

// ---- --out-root: one folder per run type, all graphs in one CSV ----
//
//   <root>/<family>/<run type>/metrics.csv        every trial, every graph (appended)
//   <root>/<family>/<run type>/topk.csv
//   <root>/<family>/<run type>/logs/<db name>/<YYYYMMDD_HHMMSS>_t<threads>.log
//
//   family   = structural | embedding
//   run type = <impl>_<candidates>[k]_<variant>, e.g. direct_khop2_outcursor_nested,
//              reference_fof_wedge, direct_exhaustive_tiled. Only the parts that
//              choose the algorithm; threads, --tile, --cache_size, --accum, warmup
//              stay as CSV columns, so a sweep of them lands in the same CSV.
//   db name  = -m, which carries the graph type: adj_r_s12_e8, split_ekey_r_s12_e8,
//              adj_rd_soc-LiveJournal (adj vs split_ekey runs never share a folder)
inline std::string run_type_label(const Options &o)
{
    std::string c = o.candidates.empty() ? "exhaustive" : o.candidates;
    if (c == "khop") c += std::to_string(o.k_hops);
    return o.impl + "_" + c + "_" + o.variant;
}

inline FILE *g_out_root_tee = nullptr;
inline void close_out_root_tee()
{
    if (!g_out_root_tee) return;
    std::cout.flush(); std::cerr.flush(); fflush(stdout); fflush(stderr);
    close(STDOUT_FILENO); close(STDERR_FILENO);   // EOF for tee
    pclose(g_out_root_tee);                        // wait until the log is complete
    g_out_root_tee = nullptr;
}

// Sets o.out_csv / o.topk_out (unless given explicitly) and copies everything
// the program prints from here on into the per-run log (via tee; the console
// still shows it). Returns the log path, or "" if --out-root is not set.
inline std::string setup_out_root(Options &o, const std::string &db_name)
{
    if (o.out_root.empty()) return "";
    namespace fs = std::filesystem;
    const std::string &graph = db_name;    // adj_r_s12_e8 / split_ekey_r_s12_e8 / ...
    fs::path dir = fs::path(o.out_root)
                 / (method_is_structural(o) ? "structural" : "embedding")
                 / run_type_label(o);
    fs::path logs = dir / "logs" / graph;
    fs::create_directories(logs);
    if (o.out_csv.empty())  o.out_csv  = (dir / "metrics.csv").string();
    if (o.topk_out.empty()) o.topk_out = (dir / "topk.csv").string();

    std::time_t t = std::time(nullptr);
    char ts[32];
    std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", std::localtime(&t));
    std::string log = (logs / (std::string(ts) + "_t" + std::to_string(o.threads) + ".log")).string();

    std::cerr << "[out-root] " << dir.string() << "  log=" << log << "\n";
    std::cout.flush(); std::cerr.flush(); fflush(stdout); fflush(stderr);
    g_out_root_tee = popen(("tee -a '" + log + "'").c_str(), "w");
    if (g_out_root_tee) {
        dup2(fileno(g_out_root_tee), STDOUT_FILENO);
        dup2(fileno(g_out_root_tee), STDERR_FILENO);
        std::atexit(close_out_root_tee);   // after main's locals (MemoryCounter) print
    }
    return log;
}

// Per-trial process resource use, from getrusage before and after the timed run.
//   peak_rss_mb  : process peak resident memory so far (MB, ru_maxrss) -- includes
//                  the WiredTiger cache and any preload, so it grows with --cache_size
//   major_faults : page faults that had to read from disk during this trial
//   block_reads  : file-system block reads during this trial (ru_inblock)
struct TrialMem {
    long peak_rss_mb  = 0;
    long major_faults = 0;
    long block_reads  = 0;
};
inline rusage rusage_now() { rusage u{}; getrusage(RUSAGE_SELF, &u); return u; }
inline TrialMem trial_mem(const rusage &before, const rusage &after) {
    TrialMem m;
    m.peak_rss_mb  = after.ru_maxrss / 1024;                 // Linux: KB
    m.major_faults = after.ru_majflt  - before.ru_majflt;
    m.block_reads  = after.ru_inblock - before.ru_inblock;
    return m;
}

inline void append_result_csv(const std::string &path,
                              const Options &o,
                              const std::string &db_path,
                              const std::string &db_name,
                              const std::string &graph_type,
                              size_t emb_dim,
                              int trial,
                              const RunResult &r,
                              double time_ms, double load_ms, double total_ms,
                              const char *cache,
                              const TrialMem &mem = TrialMem{})
{
    if (path.empty()) return;

    bool structural = method_is_structural(o);
    std::string eff_var = variation_label(o);

    // New files get the cache,warmup columns (at the end). A file started
    // before they existed keeps its old columns, so rows never misalign.
    std::string header = first_line(path);
    bool exists     = !header.empty();
    bool with_cache = !exists || header.find(",cache,warmup") != std::string::npos;
    // cache_size column added later still; same rule: only new files get it.
    bool with_size  = !exists || header.find(",warmup,cache_size") != std::string::npos;
    // family / flag / memory columns (2026-11-16); same rule again.
    bool with_detail = !exists || header.find(",cache_size,family,") != std::string::npos;
    // direction + tie-aware columns (2026-11-22); same rule again.
    bool with_ties = !exists || header.find(",block_reads,direction,") != std::string::npos;
    if (with_detail && !with_ties)
        std::cerr << "[csv] " << path << " has no direction / tie-aware columns; "
                  << "use a new --out file to get them\n";
    if (with_size && !with_detail)
        std::cerr << "[csv] " << path << " has no family/flag/memory columns; "
                  << "use a new --out file to get them\n";
    if (with_cache && !with_size)
        std::cerr << "[csv] " << path << " has no cache_size column; "
                  << "use a new --out file to get it\n";
    if (!with_cache)
        std::cerr << "[csv] " << path << " has the old header (no cache,warmup); "
                  << "writing without those columns — use a new --out file to get them\n";

    std::ofstream out(path, std::ios::app);
    if (!exists) {
        out << "timestamp,db_path,db_name,graph_type,method,variation,"
            << "threads,topk,trial,embedding_source,embedding_dim,"
            << "pairs_scored,true_edges,top_k_positives,kept,"
            << "precision_at_k,recall_at_k,time_ms,load_ms,total_ms,"
            << "cache,warmup,cache_size,"
            << "family,impl,candidates,variant,khop,tile,accum,samples,seed,cursor_scope,"
            << "peak_rss_mb,major_faults,block_reads,"
            << "direction,p_at_k_tie_aware,ties_at_cutoff,tie_pos_at_cutoff,eval,split_file\n";
    }

    double pk = linkpred::precision_at_k(r.top_k, (size_t)o.topk);
    double rk = linkpred::recall_at_k(r.top_k, (size_t)o.topk, r.true_edges);
    size_t positives = 0;
    for (const auto &sp : r.top_k) if (sp.label == 1) positives++;

    out << now_str() << "," << db_path << "," << db_name << ","
        << graph_type << "," << o.method << "," << eff_var << ","
        << o.threads << "," << o.topk << "," << trial << ","
        << (structural ? "" : o.source) << ","
        << (structural ? 0 : emb_dim) << ","
        << r.pairs_scored << "," << r.true_edges << ","
        << positives << "," << r.top_k.size() << ","
        << pk << "," << rk << ","
        << time_ms << "," << load_ms << "," << total_ms;
    if (with_cache) out << "," << cache << "," << o.warmup;
    if (with_size)  out << "," << (o.cache_size.empty() ? "default" : o.cache_size);
    if (with_detail) {
        // Flags that change results or cost; blank where they do not apply.
        const bool khop  = o.candidates == "khop";
        const bool tiled = o.variant == "tiled";
        const bool urand = o.candidates == "uniformRandom";
        out << "," << (structural ? "structural" : "embedding")
            << "," << o.impl
            << "," << (o.candidates.empty() ? "exhaustive" : o.candidates)
            << "," << o.variant
            << "," << (khop  ? std::to_string(o.k_hops)  : "")
            << "," << (tiled ? std::to_string(o.tile)    : "")
            << "," << o.accum
            << "," << (urand ? std::to_string(o.samples) : "")
            << "," << (urand ? std::to_string(o.seed)    : "")
            << "," << (o.impl == "pipeline" ? o.cursor_scope : "")
            << "," << mem.peak_rss_mb << "," << mem.major_faults << "," << mem.block_reads;
    }
    if (with_detail && with_ties) {
        const linkpred::TieStats &ts = linkpred::g_last_tie_stats;
        out << "," << (lp_both_directions(o.direction) ? "both" : "out")
            << "," << linkpred::precision_at_k_tie_aware(r.top_k, (size_t)o.topk, ts)
            << "," << (ts.valid ? std::to_string(ts.at_cutoff) : std::string(""))
            << "," << (ts.valid ? std::to_string(ts.pos_at_cutoff) : std::string(""))
            << "," << o.eval << "," << (o.eval == "holdout" ? o.split_file : std::string(""));
    }
    out << "\n";
}

inline void append_topk_csv(const std::string &path,
                            const Options &o,
                            const std::string &db_path,
                            const std::string &db_name,
                            const std::string &graph_type,
                            int trial,
                            const RunResult &r)
{
    if (path.empty()) return;

    std::string eff_var = variation_label(o);

    bool exists = false;
    { std::ifstream probe(path);
      exists = probe.good() && probe.peek() != std::ifstream::traits_type::eof(); }

    std::ofstream out(path, std::ios::app);
    if (!exists) {
        out << "timestamp,db_path,db_name,graph_type,method,variation,"
            << "threads,topk,trial,rank,u,v,score,label\n";
    }
    std::string ts = now_str();
    int rank = 1;
    for (const auto &sp : r.top_k) {
        out << ts << "," << db_path << "," << db_name << ","
            << graph_type << "," << o.method << "," << eff_var << ","
            << o.threads << "," << o.topk << "," << trial << ","
            << rank++ << "," << sp.u << "," << sp.v << ","
            << std::fixed << std::setprecision(6) << sp.score << ","
            << sp.label << "\n";
    }
}
