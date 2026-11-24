#pragma once
#include "graph.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ml_sampling {
enum class Direction { Incoming, Outgoing };
struct SamplingOptions {
    // Seed-outward hop order. -1 = all, 0 = none, positive = fanout.
    std::vector<int> fanouts{10, 5};
    Direction direction = Direction::Incoming;
    bool replacement = false;
    std::uint64_t seed = 42;
};
struct SampledEdge {
    std::size_t src, dst; // local indices, original graph orientation
};
struct SampledHop {
    std::vector<std::size_t> frontier;
    std::vector<SampledEdge> edges; // preserves sampling multiplicity
};
struct SampledGraph {
    std::vector<node_id_t> node_ids; // unique global IDs, seeds first
    std::vector<std::size_t> seed_indices; // original seed order incl duplicates
    std::vector<SampledHop> hops;
    std::vector<SampledEdge> edges; // concatenated hop edges, NOT induced graph
};
// Borrowed single-thread handle; stable topology; read-optimized Adj recommended.
// Incoming is the default for source-to-target GNN message passing.
SampledGraph sample_neighbors(GraphBase& graph,
    const std::vector<node_id_t>& seeds, const SamplingOptions& options = {});
}
