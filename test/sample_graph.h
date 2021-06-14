#include "common.h"

edge edge1 = {
    .src_id = 1,
    .dst_id = 2,
    .attr = {111, 222}};

edge edge2 = {
    .src_id = 1,
    .dst_id = 3,
    .attr = {111, 333}};

edge edge3 = {
    .src_id = 2,
    .dst_id = 3,
    .attr = {222, 333}};

node node1 = {
    .id = 1,
    .attr = {"111"}};

node node2 = {
    .id = 2,
    .attr = {"222"}};

node node3 = {
    .id = 3,
    .attr = {"333"}};

node node4 = {
    .id = 4,
    .attr = {"444"}};

std::vector<edge> test_edges = {edge1, edge2, edge3};
std::vector<edge> undirected_test_edges = {};
std::vector<node> test_nodes = {node1, node2, node3};

void create_directed_edges()
{
    for (edge x : test_edges)
    {
        edge temp;
        temp.src_id = x.dst_id;
        temp.dst_id = x.src_id;
        temp.attr.push_back(x.attr.at(1));
        temp.attr.push_back(x.attr.at(0));

        undirected_test_edges.push_back(temp);
    }
}

