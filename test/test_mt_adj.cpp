// test_mt_adj.cpp
//
// Multi-threaded correctness tests for the AdjList backend.
// Uses engine.create_graph_handle() per thread, matching the gtx_driver
// pattern (on_thread_init creates one handle per worker thread).
//
// Run:  ./test_mt_adj
// Pass: exit code 0

#include <cassert>
#include <cstdio>
#include <string>

#include "adj_list.h"
#include "common_defs.h"
#include "graph_engine.h"

#include "test_mt_common.h"

int main()
{
    graph_opts opts;
    opts.create_new      = true;
    opts.optimize_create = false;
    opts.is_directed     = true;
    opts.read_optimize   = true;
    opts.is_weighted     = false;
    opts.has_node_props  = false;
    opts.has_edge_props  = false;
    opts.type            = GraphType::Adj;
    opts.db_name         = "test_mt_adj";
    opts.db_dir          = "./db_test/db_mt_adj";
    // session_max must cover all concurrent graph handles
    opts.conn_config     = "cache_size=512MB,session_max=32";
    opts.stat_log        = "./";

    GraphEngine engine(MT_THREADS * 2 + 4, opts);

    fprintf(stderr, "=== AdjList multi-threaded tests ===\n");
    run_mt_tests(engine, "AdjList");

    engine.close_graph();
    fprintf(stderr, "\nAll AdjList multi-threaded tests PASSED.\n");
    return 0;
}
