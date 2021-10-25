#include "common.h"
#include "graph_exception.h"
#include "sample_graph.h"
#include <cassert>
#include "test_adj_list.h"

#define delim "--------------"
#define INFO() \
    fprintf(stderr, "%s\nNow running: %s\n", delim, __FUNCTION__);

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
    for (node n : SampleGraph::test_nodes)
    {
        graph.add_node(n);
    }

    for (edge x : SampleGraph::test_edges)
    {
        graph.add_edge(x, false);
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
    WT_CURSOR *in_adj_cur = graph.get_in_adjlist_cursor();
    in_adj_cur->set_key(in_adj_cur, 11);
    int ret = in_adj_cur->search(in_adj_cur);
    assert(ret == 0);

    WT_CURSOR *out_adj_cur = graph.get_out_adjlist_cursor();
    out_adj_cur->set_key(out_adj_cur, 11);
    ret = out_adj_cur->search(out_adj_cur);
    assert(ret == 0);
}

void test_get_node(AdjList graph)
{
    INFO();
    node found = graph.get_node(1);
    assert(found.id == 1);
    //now get a node that does not exist
    found = graph.get_node(-1);
    assert(found.id == 0);
    found = graph.get_random_node();
    CommonUtil::dump_node(found);
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
    WT_CURSOR *in_adj_cur = graph.get_in_adjlist_cursor();
    WT_CURSOR *out_adj_cur = graph.get_out_adjlist_cursor();
    for (int al : graph.get_adjlist(in_adj_cur, node_id))
    {
        //TODO: I don't know what would be a good testase here.
    }
}

void test_get_edge(AdjList graph)
{
    INFO();
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
    INFO();
    edge to_insert = {.id = 0,
                      .src_id = 5,
                      .dst_id = 6,
                      .edge_weight = 333}; // node 300 and 400 dont exist yet so we must also check if the nodes get created
    graph.add_edge(to_insert, false);
    edge found = graph.get_edge(5, 6);
    CommonUtil::dump_edge(found);
    assert(found.edge_weight == 333);
    if (!is_directed)
    {
        found = graph.get_edge(6, 5);
        assert(found.edge_weight == 333);
    }

    //Check if the nodes were created.
    node got = graph.get_node(5);
    assert(got.id == 5);
    got = graph.get_node(6);
    assert(got.id == 6);

    //Now check if the adjlists were updated
    WT_CURSOR *in_adj_cur = graph.get_in_adjlist_cursor();
    in_adj_cur->set_key(in_adj_cur, 6);
    assert(in_adj_cur->search(in_adj_cur) == 0);
    in_adj_cur->reset(in_adj_cur);
    std::vector<int> adjlist = graph.get_adjlist(in_adj_cur, 6);
    assert(adjlist.size() == 1);
    if (!is_directed)
    {
        in_adj_cur->set_key(in_adj_cur, 5);
        assert(in_adj_cur->search(in_adj_cur) == 0);
        in_adj_cur->reset(in_adj_cur);
        adjlist = graph.get_adjlist(in_adj_cur, 6);
        assert(adjlist.size() == 1);
    }

    WT_CURSOR *out_adj_cur = graph.get_out_adjlist_cursor();
    out_adj_cur->set_key(out_adj_cur, 5);
    assert(out_adj_cur->search(out_adj_cur) == 0);
    out_adj_cur->reset(out_adj_cur);
    adjlist = graph.get_adjlist(out_adj_cur, 5);
    assert(adjlist.size() == 1);

    if (!is_directed)
    {
        out_adj_cur->set_key(out_adj_cur, 6);
        out_adj_cur->search(out_adj_cur);
        assert(out_adj_cur->search(out_adj_cur) == 0);
        out_adj_cur->reset(out_adj_cur);
        adjlist = graph.get_adjlist(out_adj_cur, 6);
        assert(adjlist.size() == 1);
    }
}

void test_get_out_edges(AdjList graph)
{
    INFO();
    std::vector<edge> edges = graph.get_out_edges(1);
    assert(edges.size() == 2);
    //compare edge0
    assert(edges.at(0).src_id == SampleGraph::edge1.src_id);
    assert(edges.at(0).dst_id == SampleGraph::edge1.dst_id);
    //compare edge1
    assert(edges.at(1).src_id == SampleGraph::edge2.src_id);
    assert(edges.at(1).dst_id == SampleGraph::edge2.dst_id);

    //Now test for a node that has no out edge
    edges = graph.get_out_edges(4);
    assert(edges.size() == 0);

    //Now try getting out edges for a node that does not exist
    bool assert_fail = false;
    try
    {
        edges = graph.get_out_edges(1500);
    }
    catch (GraphException ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_in_edges(AdjList graph)
{
    INFO();
    std::vector<edge> edges = graph.get_in_edges(3);
    assert(edges.size() == 2);
    //Check edge0
    assert(edges.at(0).src_id == SampleGraph::edge2.src_id);
    assert(edges.at(0).dst_id == SampleGraph::edge2.dst_id);
    //Check edge1
    assert(edges.at(1).src_id == SampleGraph::edge3.src_id);
    assert(edges.at(1).dst_id == SampleGraph::edge3.dst_id);

    //now test for a node that has no in-edge
    edges = graph.get_in_edges(4);
    assert(edges.size() == 0);

    //Now try getting in edges for a node that does not exist.
    bool assert_fail = false;
    try
    {
        edges = graph.get_out_edges(1500);
    }
    catch (GraphException ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_out_nodes(AdjList graph)
{
    INFO();
    std::vector<node> nodes = graph.get_out_nodes(1);
    assert(nodes.size() == 2);
    assert(nodes.at(0).id == SampleGraph::node2.id); // edge(1->2)
    assert(nodes.at(1).id == SampleGraph::node3.id); // edge(1->3)

    //test for a node that has no out-edge
    nodes = graph.get_out_nodes(4);
    assert(nodes.size() == 0);

    //test for a node that does not exist
    bool assert_fail = false;
    try
    {
        nodes = graph.get_out_nodes(1500);
    }
    catch (GraphException ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_in_nodes(AdjList graph)
{
    INFO();
    std::vector<node> nodes = graph.get_in_nodes(3);
    assert(nodes.size() == 2);
    assert(nodes.at(0).id == SampleGraph::node1.id);
    assert(nodes.at(1).id == SampleGraph::node2.id);

    //test for a node that has no in_edge
    nodes = graph.get_in_nodes(4);
    assert(nodes.size() == 0);

    //test for a node that does not exist
    bool assert_fail = false;
    try
    {
        nodes = graph.get_in_nodes(1500);
    }
    catch (GraphException ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_in_degree(AdjList graph)
{
    INFO();
    //check in_degree for node3
    int deg = graph.get_in_degree(3);
    assert(deg == 2);
}

void test_delete_node(AdjList graph, bool is_directed)
{
    INFO();
    WT_CURSOR *n_cursor = graph.get_node_cursor();
    WT_CURSOR *e_cursor = graph.get_edge_cursor();
    WT_CURSOR *adj_out_cur = graph.get_out_adjlist_cursor();
    WT_CURSOR *adj_in_cur = graph.get_in_adjlist_cursor();

    //Verify node2 exists
    n_cursor->set_key(n_cursor, SampleGraph::node2.id);
    int ret = n_cursor->search(n_cursor);
    assert(ret == 0);
    n_cursor->reset(n_cursor);

    //Delete node2 and verify it was actually deleted
    graph.delete_node(SampleGraph::node2.id);
    n_cursor->set_key(n_cursor, SampleGraph::node2.id);
    ret = n_cursor->search(n_cursor);
    assert(ret != 0);

    //Verify node2's adjacency lists are deleted
    adj_out_cur->set_key(adj_out_cur, SampleGraph::node2.id);
    ret = adj_out_cur->search(adj_out_cur);
    assert(ret != 0);
    adj_in_cur->set_key(adj_in_cur, SampleGraph::node2.id);
    ret = adj_in_cur->search(adj_in_cur);
    assert(ret != 0);

    //check that edge(2,3) is deleted
    e_cursor->set_key(e_cursor, 2, 3);
    assert(e_cursor->search(e_cursor) != 0);
    //check that edge(1,2) is deleted
    e_cursor->set_key(e_cursor, 1, 2);
    assert(e_cursor->search(e_cursor) != 0);
    //Now delete the reverse edges for undirected graph
    if (is_directed)
    {
        e_cursor->set_key(e_cursor, 3, 2);
        assert(e_cursor->search(e_cursor) != 0);
        e_cursor->set_key(e_cursor, 2, 1);
        assert(e_cursor->search(e_cursor) != 0);
    }
    //Verify that node 2 is deleted from adjlist of node1 and node3
    adj_out_cur->reset(adj_out_cur);
    for (int dst : graph.get_adjlist(adj_out_cur, SampleGraph::node1.id))
    {
        assert(dst != SampleGraph::node2.id); //node2 should have been deleted
    }

    adj_in_cur->reset(adj_in_cur);
    for (int src : graph.get_adjlist(adj_in_cur, SampleGraph::node3.id))
    {
        assert(src != SampleGraph::node2.id); //node 2 should have been
                                              // deleted from this too
    }
}

void test_delete_isolated_node(AdjList graph, bool is_directed)
{
    INFO();
    WT_CURSOR *n_cursor = graph.get_node_cursor();
    WT_CURSOR *e_cursor = graph.get_edge_cursor();
    WT_CURSOR *adj_out_cur = graph.get_out_adjlist_cursor();
    WT_CURSOR *adj_in_cur = graph.get_in_adjlist_cursor();

    //Verify node4 exists
    n_cursor->set_key(n_cursor, SampleGraph::node4.id);
    int ret = n_cursor->search(n_cursor);
    assert(ret == 0);
    n_cursor->reset(n_cursor);

    //Delete node4 and verify it was actually deleted
    graph.delete_node(SampleGraph::node4.id);
    n_cursor->set_key(n_cursor, SampleGraph::node4.id);
    ret = n_cursor->search(n_cursor);
    assert(ret != 0);

    //Verify node4's adjacency lists are deleted
    adj_out_cur->set_key(adj_out_cur, SampleGraph::node4.id);
    ret = adj_out_cur->search(adj_out_cur);
    assert(ret != 0);
    adj_in_cur->set_key(adj_in_cur, SampleGraph::node4.id);
    ret = adj_in_cur->search(adj_in_cur);
    assert(ret != 0);

    //Check no edge has node4 in source or dst
    std::vector<edge> edges = graph.get_edges();
    for (edge e : edges)
    {
        assert(e.src_id != SampleGraph::node4.id);
        assert(e.dst_id != SampleGraph::node4.id);
    }

    //Now check if node4 is present in in/out_adj_list of any of the remaining
    //nodes;
    std::vector<int> remaining_nodes = {1, 3};

    for (int n : remaining_nodes)
    {
        adj_out_cur->reset(adj_out_cur);
        for (int dst : graph.get_adjlist(adj_out_cur, n))
        {
            assert(dst != SampleGraph::node4.id); //node4 should not exist here
        }
        adj_in_cur->reset(adj_in_cur);
        for (int src : graph.get_adjlist(adj_in_cur, n))
        {
            assert(src != SampleGraph::node4.id); //node 4 should not exist here
        }
    }
}

int main()
{
    graph_opts opts;
    opts.create_new = false;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_name = "test_adj";

    AdjList graph = AdjList(opts);
    create_init_nodes(graph, opts.is_directed);
    test_get_nodes(graph);
    test_get_node(graph);
    test_add_edge(graph, opts.is_directed);
    test_get_edge(graph);
    test_get_out_edges(graph);
    test_get_out_edges(graph);
    test_get_in_edges(graph);
    test_get_out_nodes(graph);
    test_get_in_nodes(graph);
    test_get_in_degree(graph);
    test_delete_node(graph, opts.is_directed);
    test_delete_isolated_node(graph, opts.is_directed);

    tearDown(graph);
}
