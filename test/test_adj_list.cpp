#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "sample_graph.h"
#include <cassert>
#include "test_adj_list.h"

#define INFO() \
    fprintf(stderr, "Now running: %s\n", __FUNCTION__);

void create_init_nodes(AdjList graph, bool is_directed)
{
    INFO()
    for (node x : SampleGraph::test_nodes)
    {
        graph.add_node(x);
    }

    if (!is_directed)
    {
        SampleGraph::create_undirected_edges();
        assert(SampleGraph::test_edges.size() == 6); //checking if directed edges got created and stored in test_edges
    }
    int edge_cnt = 1;
    for (edge x : SampleGraph::test_edges)
    {
        graph.add_edge(x);
        edge_cnt++;
    }
}

void tearDown(AdjList graph)
{
    graph.close();
}

void test_node_add(AdjList graph, bool read_optimize)
{
    INFO();
    node new_node = {
        .id = 11,
        .in_degree = 0,
        .out_degree = 0};
    graph.add_node(new_node);
    node found = graph.get_node(new_node.id);
    assert(new_node.id == found.id);
    if (read_optimize)
    {
        assert(found.in_degree == 0);
        assert(found.out_degree == 0);
    }
    //Check the adjlists
    WT_CURSOR *in_adj_cur = graph.get_test_in_adjlist_cursor();
    in_adj_cur->set_key(in_adj_cur, 11);
    int ret = in_adj_cur->search(in_adj_cur);
    assert(ret == 0);

    WT_CURSOR *out_adj_cur = graph.get_test_out_adjlist_cursor();
    out_adj_cur->set_key(out_adj_cur, 11);
    ret = out_adj_cur->search(out_adj_cur);
    assert(ret == 0);
}

void test_get_node(AdjList graph)
{
    INFO();
    node found = graph.get_node(11);
    assert(found.id == 11);
    //now get a node that does not exist
    found = graph.get_node(-1);
    assert(found.id == 0);
    found = graph.get_random_node();
    assert(found.id > 0);
}
void test_get_nodes(AdjList graph)
{
    INFO();
    for (node x : graph.get_nodes())
    {
        CommonUtil::dump_node(x);
    }
}

void test_get_edges(AdjList graph)
{
    INFO();
    for (edge e : graph.get_edges())
    {
        CommonUtil::dump_edge(e);
    }
}

void test_get_adjlist(AdjList graph, int node_id)
{
    INFO();
    std::cout << "Printing the in_adjlist first" << std::endl;
    WT_CURSOR *in_adj_cur = graph.get_test_in_adjlist_cursor();
    WT_CURSOR *out_adj_cur = graph.get_test_out_adjlist_cursor();
    for (int al : graph.get_adjlist(in_adj_cur, node_id))
    {
        //TODO: I don't know what would be a good testase here.
    }
}

void test_get_edge(AdjList graph)
{
    edge found = graph.get_edge(SampleGraph::edge1.src_id, SampleGraph::edge1.dst_id);
    assert(found.src_id == SampleGraph::edge1.src_id);
    assert(found.dst_id == SampleGraph::edge1.dst_id);
    assert(found.edge_weight == SampleGraph::edge1.edge_weight);

    //Now get a non-existent edge
    found = graph.get_edge(222, 333);
    assert(found.src_id == 0);
    assert(found.dst_id == 0);
    assert(found.edge_weight == 0);
}

void test_add_edge(AdjList graph, bool is_directed)
{
    edge to_insert = {.id = 0,
                      .src_id = 300,
                      .dst_id = 400,
                      .edge_weight = 333}; // node 300 and 400 dont exist yet so we must also check if the nodes get created
    graph.add_edge(to_insert);
    edge found = graph.get_edge(3, 4);
    assert(found.edge_weight == 333);
    if (!is_directed)
    {
        found = graph.get_edge(4, 3);
        assert(found.edge_weight == 333);
    }

    //Check if the nodes were created.
    node got = graph.get_node(300);
    assert(got.id = 300);
    got = graph.get_node(400);
    assert(got.id == 400);

    //Now check if the adjlists were updated
    WT_CURSOR *in_adj_cur = graph.get_test_in_adjlist_cursor();
    in_adj_cur->set_key(in_adj_cur, 4);
    assert(in_adj_cur->search(in_adj_cur) == 0);
    if (!is_directed)
    {
        in_adj_cur->set_key(in_adj_cur, 3);
        assert(in_adj_cur->search(in_adj_cur) == 0);
    }

    WT_CURSOR *out_adj_cur = graph.get_test_out_adjlist_cursor();
    out_adj_cur->set_key(out_adj_cur, 3);
    assert(out_adj_cur->search(out_adj_cur) == 0);
    if (!is_directed)
    {
        out_adj_cur->set_key(out_adj_cur, 4);
        out_adj_cur->search(out_adj_cur);
        assert(out_adj_cur->search(out_adj_cur) == 0);
    }
}

int main()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_name = "test_adj";

    AdjList graph = AdjList(opts);
    create_init_nodes(graph, opts.is_directed);
    // test_get_node(graph);
    // test_add_edge(graph, opts.is_directed);
    tearDown(graph);
}