// test_error_handling_ekey.cpp
//
// Error-handling and driver-compliance tests for the SplitEdgeKey backend.
// Runs exactly the same test suite as test_error_handling_adj.cpp so that
// parity between the two backends is verified automatically.
//
// Run:  ./test_error_handling_ekey
// Pass: exit code 0; all lines end with ": PASSED"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "common_defs.h"
#include "edgekey_split.h"
#include "graph_engine.h"

#include "test_error_handling_common.h"

// ---------------------------------------------------------------------------
// Graph-opts factory helpers
// ---------------------------------------------------------------------------

static graph_opts make_directed_opts()
{
    graph_opts opts;
    opts.create_new      = true;
    opts.optimize_create = false;
    opts.is_directed     = true;
    opts.read_optimize   = true;
    opts.is_weighted     = false;
    opts.has_node_props  = false;
    opts.has_edge_props  = false;
    opts.type            = GraphType::SplitEKey;
    opts.db_name         = "test_eh_ekey_dir";
    opts.db_dir          = "./db_test/db_eh_ekey_dir";
    opts.conn_config     = "cache_size=256MB";
    opts.stat_log        = "./";
    return opts;
}

static graph_opts make_undirected_opts()
{
    graph_opts opts  = make_directed_opts();
    opts.is_directed = false;
    opts.db_name     = "test_eh_ekey_undir";
    opts.db_dir      = "./db_test/db_eh_ekey_undir";
    return opts;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    // ---- directed tests ----
    {
        graph_opts opts = make_directed_opts();
        GraphEngine engine(1, opts);
        WT_CONNECTION *conn = engine.get_connection();
        SplitEdgeKey graph(opts, conn);

        fprintf(stderr, "=== SplitEdgeKey directed tests ===\n");
        run_directed_tests(graph);

        graph.close(false);
        engine.close_graph();
    }

    // ---- undirected tests (separate DB) ----
    {
        graph_opts opts = make_undirected_opts();
        GraphEngine engine(1, opts);
        WT_CONNECTION *conn = engine.get_connection();
        SplitEdgeKey graph(opts, conn);

        fprintf(stderr, "=== SplitEdgeKey undirected tests ===\n");
        run_undirected_tests(graph);

        graph.close(false);
        engine.close_graph();
    }

    fprintf(stderr, "\nAll SplitEdgeKey error-handling tests PASSED.\n");
    return 0;
}
