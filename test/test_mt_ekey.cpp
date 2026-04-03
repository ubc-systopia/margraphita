// test_mt_ekey.cpp
//
// Multi-threaded correctness tests for the SplitEdgeKey backend.
// Identical test suite to test_mt_adj.cpp — run both to confirm parity.
//
// Run:  ./test_mt_ekey
// Pass: exit code 0

#include <cassert>
#include <cstdio>
#include <string>

#include "common_defs.h"
#include "edgekey_split.h"
#include "graph_engine.h"

#include "test_mt_common.h"

int main(int argc, char *argv[])
{
    graph_opts opts;
    opts.create_new      = true;
    opts.optimize_create = false;
    opts.is_directed     = true;
    // Default true; pass --no-read-optimize to disable.
    opts.read_optimize   = true;
    for (int i = 1; i < argc; i++)
        if (std::string(argv[i]) == "--no-read-optimize") opts.read_optimize = false;
    opts.is_weighted     = false;
    opts.has_node_props  = false;
    opts.has_edge_props  = false;
    opts.type            = GraphType::SplitEKey;
    opts.db_name         = "test_mt_ekey";
    opts.db_dir          = "./db_test/db_mt_ekey";
    opts.conn_config     = "cache_size=512MB,session_max=32";
    opts.stat_log        = "./";

    GraphEngine engine(MT_THREADS * 2 + 4, opts);

    fprintf(stderr, "=== SplitEdgeKey multi-threaded tests ===\n");
    run_mt_tests(engine, "SplitEdgeKey");

    engine.close_graph();
    fprintf(stderr, "\nAll SplitEdgeKey multi-threaded tests PASSED.\n");
    return 0;
}
