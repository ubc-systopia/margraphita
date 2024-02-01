
#ifndef SAMPLE_STD
#define SAMPLE_STD
#include "common_util.h"
namespace SampleGraph
{
edge edge1 = {.id =0, .src_id = 1, .dst_id = 2, .edge_weight = 0};

edge edge2 = {.id = 1, .src_id = 1, .dst_id = 3, .edge_weight =0};

edge edge3 = {.id =2, .src_id = 2, .dst_id = 3, .edge_weight =0};

edge edge4 = {.id =3, .src_id = 1, .dst_id = 7, .edge_weight =0};

edge edge5 = {.id=4, .src_id = 7, .dst_id = 8, .edge_weight =0};

edge edge6 = {.id =6, .src_id = 8, .dst_id = 7, .edge_weight =0};

node node1 = {.id = 1};

node node2 = {.id = 2};

node node3 = {.id = 3};

node node4 = {.id = 4};

node node7 = {.id = 7};

node node8 = {.id = 8};

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
            if (find_edge.dst_id == temp.dst_id &&
                find_edge.src_id == temp.src_id)
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
}  // namespace SampleGraph

#endif