#pragma once

#include <cstddef>
#include <vector>

#include "linkpred_topk.h"

// Result of one link-prediction run — populated by the workers, consumed by
// print_result() and the two CSV writers.
struct RunResult {
    size_t pairs_scored = 0;
    size_t true_edges   = 0;
    std::vector<linkpred::ScoredPair> top_k;
};