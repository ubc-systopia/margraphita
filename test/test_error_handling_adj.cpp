// test_error_handling_adj.cpp
//
// Error-handling and driver-compliance tests for the AdjList backend.
//
// Covers:
//   T1.1–T1.9  basic single-threaded correctness
//   T1.7–T1.8  undirected-graph edge symmetry
//   T2.2–T2.3  count and adjacency-list integrity after rollback
//   T3.1–T3.2  AdjList-specific adjacency-list structural correctness
//   Driver sim  add_vertex / add_edge_v2 retry-loop contract
//
// Run:  ./test_error_handling_adj
// Pass: exit code 0; all lines end with ": PASSED"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "adj_list.h"
#include "common_defs.h"
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
    opts.type            = GraphType::Adj;
    opts.db_name         = "test_eh_adj_dir";
    opts.db_dir          = "./db_test/db_eh_adj_dir";
    opts.conn_config     = "cache_size=256MB";
    opts.stat_log        = "./";
    return opts;
}

static graph_opts make_undirected_opts()
{
    graph_opts opts        = make_directed_opts();
    opts.is_directed       = false;
    opts.db_name           = "test_eh_adj_undir";
    opts.db_dir            = "./db_test/db_eh_adj_undir";
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
        GraphBase *g = engine.create_graph_handle();

        fprintf(stderr, "=== AdjList directed tests ===\n");
        run_directed_tests(*g);

        g->close(false);
        engine.close_graph();
    }

    // ---- undirected tests (separate DB) ----
    {
        graph_opts opts = make_undirected_opts();
        GraphEngine engine(1, opts);
        GraphBase *g = engine.create_graph_handle();

        fprintf(stderr, "=== AdjList undirected tests ===\n");
        run_undirected_tests(*g);

        g->close(false);
        engine.close_graph();
    }

    fprintf(stderr, "\nAll AdjList error-handling tests PASSED.\n");
    return 0;
}
