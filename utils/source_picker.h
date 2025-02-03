#ifndef _SOURCE_PICKER_H_
#define _SOURCE_PICKER_H_

#include <algorithm>
#include <cinttypes>
#include <functional>
#include <random>
#include <utility>
#include <vector>

#include "common_util.h"
#include "graph.h"

static const int64_t kRandSeed = 27491095;

class SourcePicker
{
 public:
  explicit SourcePicker(const GraphBase &g, node_id_t given_source = -1)
      : given_source(given_source),
        rng(kRandSeed),
        udist(0, g.get_num_nodes() - 1),
        g_(g)
  {
  }

  node_id_t PickNext()
  {
    if (given_source != -1) return given_source;
    node_id_t source;
    do
    {
      source = udist(rng);
    } while (g_.get_out_degree(source) == 0);
    return source;
  }

 private:
  node_id_t given_source;
  std::mt19937 rng;
  std::uniform_int_distribution<node_id_t> udist;
  const GraphBase &g_;
};

#endif