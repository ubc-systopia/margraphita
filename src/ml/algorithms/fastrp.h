#pragma once
#include "ml/ml_access.h"
#include <string>
#include <vector>

namespace ml {
struct FastRPOptions {
    int dim = 128, iters = 4, threads = 16;
    double density = 0.01;
    uint64_t seed = 42;
    std::vector<float> weights;
    bool step_norm = false;
    bool fastrp_paper_variant = false;        // --variant=fastrp-paper-variant: step_norm + degree^beta scaling of R, weights [0,1,..]
    double beta = -0.5;        // --beta= (paper only)
    bool cursor_read = true;   // --adj-read=cursor (default) | api has node (not right)
    bool csr = false;          // --adj-read=csr: lists read once into an in-memory CSR
    std::string write_db;      // "" = off | fused | after
};

struct FastRPResult {
    std::vector<node_id_t> ids;
    ml_access::VectorBatch embeddings;
};

// Borrowed handles: one distinct worker handle per thread, plus the main
// handle. Caller prepares raw Float32 property storage for write_db modes.
// Topology must remain stable during computation. No handles are closed here.
FastRPResult fastrp(GraphBase& graph, const std::vector<GraphBase*>& workers,
                    FastRPOptions options, bool directed);
} // namespace ml
