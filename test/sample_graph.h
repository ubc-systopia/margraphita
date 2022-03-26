
#ifndef SAMPLE_STD
#define SAMPLE_STD
#include "common.h"
namespace SampleGraph
{
edge edge1 = {.src_id = 1, .dst_id = 2};

edge edge2 = {.src_id = 1, .dst_id = 3};

edge edge3 = {.src_id = 2, .dst_id = 3};

node node1 = {.id = 1};

node node2 = {.id = 2};

node node3 = {.id = 3};

node node4 = {.id = 4};

std::vector<edge> test_edges = {edge1, edge2, edge3};
std::vector<node> test_nodes = {node1, node2, node3, node4};

void create_undirected_edges()
{
    int size = test_edges.size();
    for (int i = 0; i < size; i++)
    {
        edge x = test_edges.at(i);
        edge temp;
        temp.src_id = x.dst_id;
        temp.dst_id = x.src_id;

        test_edges.push_back(temp);
    }
}
}  // namespace SampleGraph

#endif