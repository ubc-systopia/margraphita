
#ifndef SAMPLE_STD
#define SAMPLE_STD
#include "common_util.h"
namespace SampleGraphAdjList
{
// edge edge1 = {.id = 0, .src_id = 1, .dst_id = 2, .edge_weight = 0};

// edge edge2 = {.id = 1, .src_id = 1, .dst_id = 3, .edge_weight = 0};
edge edge1 = {.id = 1, .src_id = 1, .dst_id = 4, .edge_weight = 0};

// edge edge3 = {.id = 2, .src_id = 2, .dst_id = 3, .edge_weight = 0};
edge edge2 = {.id = 2, .src_id = 1, .dst_id = 5, .edge_weight = 0};

// edge edge4 = {.id = 3, .src_id = 1, .dst_id = 7, .edge_weight = 0};
edge edge3 = {.id = 3, .src_id = 1, .dst_id = 8, .edge_weight = 0};

// edge edge5 = {.id = 4, .src_id = 7, .dst_id = 8, .edge_weight = 0};
edge edge4 = {.id = 4, .src_id = 1, .dst_id = 9, .edge_weight = 0};

edge edge5 = {.id = 0, .src_id = 1, .dst_id = 13, .edge_weight = 0};

// edge edge6 = {.id = 6, .src_id = 8, .dst_id = 7, .edge_weight = 0};
edge edge6 = {.id = 6, .src_id = 2, .dst_id = 4, .edge_weight = 0};

std::vector<edge> parallel_insert_edges = {
    edge{.id = 7,  .src_id = 2,  .dst_id = 4,  .edge_weight = 0},
    edge{.id = 8,  .src_id = 3,  .dst_id = 4,  .edge_weight = 0},
    edge{.id = 9,  .src_id = 2,  .dst_id = 7,  .edge_weight = 0},
    edge{.id = 10, .src_id = 3,  .dst_id = 7,  .edge_weight = 0},
    edge{.id = 11, .src_id = 4,  .dst_id = 7,  .edge_weight = 0},
    edge{.id = 12, .src_id = 2,  .dst_id = 8,  .edge_weight = 0},
    edge{.id = 13, .src_id = 3,  .dst_id = 8,  .edge_weight = 0},
    edge{.id = 14, .src_id = 4,  .dst_id = 8,  .edge_weight = 0},
    edge{.id = 15, .src_id = 1,  .dst_id = 4,  .edge_weight = 0},
    edge{.id = 16, .src_id = 1,  .dst_id = 8,  .edge_weight = 0},
    edge{.id = 17, .src_id = 5,  .dst_id = 6,  .edge_weight = 0},
    edge{.id = 18, .src_id = 5,  .dst_id = 7,  .edge_weight = 0},
    edge{.id = 19, .src_id = 5,  .dst_id = 8,  .edge_weight = 0},
    edge{.id = 20, .src_id = 6,  .dst_id = 7,  .edge_weight = 0},
    edge{.id = 21, .src_id = 6,  .dst_id = 8,  .edge_weight = 0},
    edge{.id = 22, .src_id = 4,  .dst_id = 5,  .edge_weight = 0},
    edge{.id = 23, .src_id = 3,  .dst_id = 5,  .edge_weight = 0},
    edge{.id = 24, .src_id = 2,  .dst_id = 5,  .edge_weight = 0},
    edge{.id = 25, .src_id = 1,  .dst_id = 5,  .edge_weight = 0},
    edge{.id = 26, .src_id = 9,  .dst_id = 10, .edge_weight = 0},
    edge{.id = 27, .src_id = 9,  .dst_id = 11, .edge_weight = 0},
    edge{.id = 28, .src_id = 9,  .dst_id = 12, .edge_weight = 0},
    edge{.id = 29, .src_id = 10, .dst_id = 11, .edge_weight = 0},
    edge{.id = 30, .src_id = 10, .dst_id = 12, .edge_weight = 0},
    edge{.id = 31, .src_id = 11, .dst_id = 12, .edge_weight = 0},
    edge{.id = 32, .src_id = 5,  .dst_id = 9,  .edge_weight = 0},
    edge{.id = 33, .src_id = 6,  .dst_id = 9,  .edge_weight = 0},
    edge{.id = 34, .src_id = 7,  .dst_id = 9,  .edge_weight = 0},
    edge{.id = 35, .src_id = 8,  .dst_id = 9,  .edge_weight = 0},
    edge{.id = 36, .src_id = 4,  .dst_id = 9,  .edge_weight = 0},
    edge{.id = 37, .src_id = 3,  .dst_id = 9,  .edge_weight = 0},
    edge{.id = 38, .src_id = 2,  .dst_id = 9,  .edge_weight = 0},
    edge{.id = 39, .src_id = 1,  .dst_id = 9,  .edge_weight = 0},
    edge{.id = 40, .src_id = 10, .dst_id = 13, .edge_weight = 0},
    edge{.id = 41, .src_id = 11, .dst_id = 13, .edge_weight = 0},
    edge{.id = 42, .src_id = 12, .dst_id = 13, .edge_weight = 0},
    edge{.id = 43, .src_id = 9,  .dst_id = 13, .edge_weight = 0},
    edge{.id = 44, .src_id = 8,  .dst_id = 13, .edge_weight = 0},
    edge{.id = 45, .src_id = 7,  .dst_id = 13, .edge_weight = 0},
    edge{.id = 46, .src_id = 6,  .dst_id = 13, .edge_weight = 0},
    edge{.id = 47, .src_id = 5,  .dst_id = 13, .edge_weight = 0},
    edge{.id = 48, .src_id = 4,  .dst_id = 13, .edge_weight = 0},
    edge{.id = 49, .src_id = 3,  .dst_id = 13, .edge_weight = 0},
    edge{.id = 50, .src_id = 2,  .dst_id = 13, .edge_weight = 0},
    edge{.id = 51, .src_id = 1,  .dst_id = 13, .edge_weight = 0},
    edge{.id = 52, .src_id = 14, .dst_id = 15, .edge_weight = 0},
    edge{.id = 53, .src_id = 14, .dst_id = 16, .edge_weight = 0},
    edge{.id = 54, .src_id = 15, .dst_id = 16, .edge_weight = 0},
    edge{.id = 55, .src_id = 13, .dst_id = 14, .edge_weight = 0},
    edge{.id = 56, .src_id = 12, .dst_id = 14, .edge_weight = 0}

  }; // 16 unique nodes. 


/**
 * @brief count, node_id
  5 1
  6 2
  6 3
  8 4
  9 5
  5 6
  7 7
  8 8
  12 9
  4 10
  4 11
  5 12
  13 13
  4 14
  2 15
  2 16
 */
node node1 = {.id = 1};

node node2 = {.id = 2};

node node3 = {.id = 3};

node node4 = {.id = 4};

node node7 = {.id = 7};

node node8 = {.id = 8};

node isolated_node = {.id = 111};  // This node has no edges.

std::vector<edge> test_edges = {edge1, edge2, edge3, edge4, edge5, edge6};
std::vector<node> test_nodes = {node1, node2, node3, node4, node7, node8};

void create_undirected_edges()
{
  int size = test_edges.size();
  for (int i = 0; i < size; i++)
  {
    edge x = test_edges.at(i);
    edge temp;
    temp.src_id = x.dst_id;
    temp.dst_id = x.src_id;
    bool matched = false;
    for (auto find_edge : test_edges)
    {
      if (find_edge.dst_id == temp.dst_id && find_edge.src_id == temp.src_id)
      {
        matched = true;
      }
    }
    if (!matched)
    {
      test_edges.push_back(temp);
    }
  }
}
}  // namespace SampleGraphAdjList

#endif