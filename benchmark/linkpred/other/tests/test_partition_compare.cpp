// test_partition_compare.cpp — THROWAWAY. A/B run_pipeline_structural
// (block stealing) against run_pipeline_structural_contiguous (fixed ranges)
// on structural exhaustive. Delete after reading the result.
//
// Run:
//   ./test_partition_compare -p <db_dir> -m <db_name> -g adj -r --method=cn --threads=20 --topk=10
//
// Prints pairs / true_edges / time_ms / speedup per strategy. Exits 1 if
// pairs_scored, true_edges or any top-k row disagree — that would mean the two
// partitions computed different results, which must never happen.
// Each strategy runs once untimed first (warm-up), so neither one gets an
// unfair cold or warm DB cache. Only meaningful on a graph with many 64-node
// blocks per thread (a tiny test graph fits in one block).

#include <chrono>
#include <cstdio>
#include <vector>

#include <omp.h>
#include "command_line.h"
#include "common_defs.h"
#include "graph_engine.h"

#include "linkpred_thread_handles.h"   // g_thread_handles (was linkpred_exhaustive.h)
#include "linkpred_options.h"
#include "linkpred_types.h"
#include "linkpred_candidates.h"
#include "linkpred_pipeline.h"     // run_pipeline_structural,
                                   // run_pipeline_structural_contiguous

// Duplicated from fg_linkpred_bench_db.cpp, where it's `static`.
// Throwaway file — duplication is fine here. If you keep this test, extract
// open_handles into a shared header and have both use it.
// Serial on purpose: create_ro_graph_handle is not thread-safe (see the
// comment on open_handles in fg_linkpred_bench_db.cpp).
static GraphBase *open_handles(GraphEngine &engine, const Options &o)
{
    std::string checkpoint = engine.make_checkpoint();
    GraphBase *g = engine.create_ro_graph_handle(checkpoint);

    g_thread_handles.resize(o.threads);
    for (int i = 0; i < o.threads; ++i)
        g_thread_handles[i] = engine.create_ro_graph_handle(checkpoint);
    return g;
}

static double ms_between(std::chrono::high_resolution_clock::time_point a,
                         std::chrono::high_resolution_clock::time_point b)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count() / 1e6;
}

int main(int argc, char *argv[])
{
    Options o;
    std::vector<char *> clean_argv;
    if (!parse_lp_options(argc, argv, o, clean_argv)) return 1;

    CmdLineApp cli((int)clean_argv.size(), clean_argv.data());
    if (!cli.parse_args()) return -1;
    cmdline_opts opts = cli.get_parsed_opts();
    opts.read_only  = true;
    opts.create_new = false;

    if (o.impl == "reference") {
        fprintf(stderr, "ERROR: --impl=reference (or --reference) is meaningless for this test\n");
        return 1;
    }
    if (o.method != "cn" && o.method != "jaccard") {
        fprintf(stderr, "ERROR: this test only runs structural (cn / jaccard)\n");
        return 1;
    }

    GraphEngine engine(o.threads + 1, opts);
    GraphBase *g = open_handles(engine, o);
    std::cout << "opened, threads=" << o.threads
              << ", method=" << o.method << "\n";

    auto make = [](GraphBase *h) { return linkpred::ExhaustiveCandidates(h); };

    // warm-up: one untimed run of each, so both timed runs see a warm cache
    run_pipeline_structural(g, o, make);
    run_pipeline_structural_contiguous(g, o, make);

    auto t0 = std::chrono::high_resolution_clock::now();
    RunResult steal = run_pipeline_structural(g, o, make);
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms_steal = ms_between(t0, t1);

    auto t2 = std::chrono::high_resolution_clock::now();
    RunResult cont  = run_pipeline_structural_contiguous(g, o, make);
    auto t3 = std::chrono::high_resolution_clock::now();
    double ms_cont = ms_between(t2, t3);

    printf("\n");
    printf("%-12s %12s %12s %10s\n", "strategy", "pairs", "true_edges", "time_ms");
    printf("%-12s %12zu %12zu %10.3f\n", "steal",
           steal.pairs_scored, steal.true_edges, ms_steal);
    printf("%-12s %12zu %12zu %10.3f\n", "contiguous",
           cont.pairs_scored, cont.true_edges, ms_cont);
    printf("%-12s %12s %12s %10.2fx\n", "speedup",
           "", "", ms_cont / ms_steal);

    int rc = 0;
    if (steal.pairs_scored != cont.pairs_scored) {
        fprintf(stderr, "MISMATCH pairs_scored: steal=%zu contiguous=%zu\n",
                steal.pairs_scored, cont.pairs_scored);
        rc = 1;
    }
    if (steal.true_edges != cont.true_edges) {
        fprintf(stderr, "MISMATCH true_edges: steal=%zu contiguous=%zu\n",
                steal.true_edges, cont.true_edges);
        rc = 1;
    }
    if (steal.top_k.size() != cont.top_k.size()) {
        fprintf(stderr, "MISMATCH top_k size: steal=%zu contiguous=%zu\n",
                steal.top_k.size(), cont.top_k.size());
        rc = 1;
    } else {
        for (size_t i = 0; i < steal.top_k.size(); ++i) {
            const auto &a = steal.top_k[i], &b = cont.top_k[i];
            if (a.u != b.u || a.v != b.v || a.score != b.score || a.label != b.label) {
                fprintf(stderr, "MISMATCH top_k rank %zu: steal=(%llu,%llu) contiguous=(%llu,%llu)\n",
                        i + 1, (unsigned long long)a.u, (unsigned long long)a.v,
                        (unsigned long long)b.u, (unsigned long long)b.v);
                rc = 1;
                break;
            }
        }
    }
    if (rc == 0)
        printf("\nOK — both partitions produced identical metrics and top-k.\n");
    else
        printf("\nFAIL — partitions disagree, timing number is meaningless.\n");

    for (auto *h : g_thread_handles) if (h) h->close(false);
    g->close(false);
    engine.close_graph();
    return rc;
}