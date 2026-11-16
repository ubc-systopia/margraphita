// linkpred_thread_handles.h -- per-thread read-only graph handles.
//
// One handle per worker thread: WiredTiger sessions and cursors are NOT
// thread-safe, so each thread reads through its own handle,
// g_thread_handles[tid]. Always holds o.threads handles, including when
// threads == 1. Filled and closed by the bench (fg_linkpred_bench_db.cpp:
// engine.create_ro_graph_handle per thread); used by the direct
// (exhaustive, fof, khop) and pipeline-generator workers.
//
// Moved out of linkpred_exhaustive.h (2026-11-16) so fof, khop and the
// pipeline no longer include the exhaustive file just for this.

#pragma once

#include <vector>

#include "graph.h"

inline std::vector<GraphBase*> g_thread_handles;   // inline: defined in a header
