// linkpred_khop_outcursor_nested.h -- shared driver for --candidates=khop
// --variant=outcursor_nested (k = 2 only), used by both families:
//   run_direct_khop_structural_outcursor_nested -> run_direct_khop_outcursor_nested<structural_worker_khop_outcursor_nested>
//   run_direct_khop_embedding_outcursor_nested  -> run_direct_khop_outcursor_nested<embedding_worker_khop_outcursor_nested>
//
// Direct (threaded) versions of run_reference_khop_{structural,embedding}_outcursor_nested
// (2026-11-16). Same candidates and scores as the references; the work is split
// into 64-node blocks of u with the same block-stealing loop as cursor_bfs, so
// this driver checks k = 2 and then reuses run_direct_khop_cursor_bfs<Worker>'s
// loop (linkpred_khop_cursor.h): only the worker differs.

#pragma once

#include <cstdio>

#include "linkpred_khop_cursor.h"   // block-stealing loop (run_direct_khop_cursor_bfs<Worker>)
#include "linkpred_options.h"
#include "linkpred_types.h"

// Worker: void(GraphBase *g, node_id_t start, node_id_t end, const Options &o,
//             linkpred::TopK &top, size_t &pairs_scored, size_t &true_edges)
template <auto Worker>
static RunResult run_direct_khop_outcursor_nested(GraphBase *g, const Options &o)
{
    if (o.k_hops != 2) {
        fprintf(stderr, "ERROR: khop outcursor_nested is k=2 only (got k=%d)\n", o.k_hops);
        return RunResult{};
    }
    return run_direct_khop_cursor_bfs<Worker>(g, o);   // same block loop; Worker does the nested traversal
}
