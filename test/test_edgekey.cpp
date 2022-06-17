#include "test_edgekey.h"

#include <cassert>

#include "common.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define delim "--------------"
#define INFO() fprintf(stderr, "%s\nNow running: %s\n", delim, __FUNCTION__);

void create_init_nodes(EdgeKey graph, bool is_directed)
{
    INFO()
    for (node x : SampleGraph::test_nodes)
    {
        graph.add_node(x);
    }

    if (!is_directed)
    {
        SampleGraph::create_undirected_edges();
        assert(SampleGraph::test_edges.size() ==
               6);  // checking if directed edges got created and stored in
                    // test_edges
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

void test_get_node(EdgeKey graph)
{
    INFO();
    node found = graph.get_node(SampleGraph::node1.id);
    assert(found.id == 1);
    // now get a node that does not exist
    found = graph.get_node(-1);
    assert(found.id == 0);
    found = graph.get_random_node();
    CommonUtil::dump_node(found);
}

void test_node_add(EdgeKey graph, bool read_optimize)
{
    INFO();
    node new_node = {.id = 11, .in_degree = 0, .out_degree = 0};
    graph.add_node(new_node);
    node found = graph.get_node(new_node.id);
    assert(new_node.id == found.id);
    if (read_optimize)
    {
        assert(found.in_degree == 0);
        assert(found.out_degree == 0);
    }
}

void test_get_nodes(EdgeKey graph)
{
    INFO();
    for (node x : graph.get_nodes())
    {
        CommonUtil::dump_node(x);
    }
}

void test_add_edge(EdgeKey graph, bool is_directed)
{
    INFO();
    edge to_insert = {
        .src_id = 5,
        .dst_id = 6,
        .edge_weight = 333};  // node 300 and 400 dont exist yet so we must also
                              // check if the nodes get created
    graph.add_edge(to_insert, false);
    edge found = graph.get_edge(5, 6);
    CommonUtil::dump_edge(found);
    assert(found.src_id == 5);
    assert(found.dst_id == 6);
    assert(found.edge_weight == 333);
    if (!is_directed)
    {
        found = graph.get_edge(6, 5);
        assert(found.src_id == 6);
        assert(found.src_id == 5);
        assert(found.edge_weight == 333);
    }

    // Check if the nodes were created.
    node got = graph.get_node(5);
    assert(got.id == 5);
    got = graph.get_node(6);
    assert(got.id == 6);

    // //Now check if the adjlists were updated
    WT_CURSOR *e_cur = graph.get_edge_cursor();
    e_cur->set_key(e_cur, 5, OutOfBand_ID);
    assert(e_cur->search(e_cur) == 0);
    assert(graph.get_out_degree(5) == 1);
    assert(graph.get_in_degree(6) == 1);

    if (is_directed)
    {
        assert(graph.get_in_degree(5) == 0);
        assert(graph.get_out_degree(6) == 0);
    }
    else
    {
        assert(graph.get_in_degree(5) == 1);
        assert(graph.get_out_degree(6) == 1);
    }
}

void test_get_edge(EdgeKey graph)
{
    INFO();
    edge found =
        graph.get_edge(SampleGraph::edge1.src_id, SampleGraph::edge1.dst_id);
    assert(found.src_id == SampleGraph::edge1.src_id);
    assert(found.dst_id == SampleGraph::edge1.dst_id);

    // Now get a non-existent edge
    found = graph.get_edge(222, 333);
    assert(found.src_id == -1);
    assert(found.dst_id == -1);
    assert(found.edge_weight == -1);
}

void test_get_out_edges(EdgeKey graph)
{
    INFO();
    std::vector<edge> edges = graph.get_out_edges(1);
    assert(edges.size() == 3);
    // compare edge0
    assert(edges.at(0).src_id == SampleGraph::edge1.src_id);
    assert(edges.at(0).dst_id == SampleGraph::edge1.dst_id);
    // compare edge1
    assert(edges.at(1).src_id == SampleGraph::edge2.src_id);
    assert(edges.at(1).dst_id == SampleGraph::edge2.dst_id);

    assert(edges.at(2).src_id == SampleGraph::edge4.src_id);
    assert(edges.at(2).dst_id == SampleGraph::edge4.dst_id);

    // Now test for a node that has no out edge
    edges = graph.get_out_edges(4);
    assert(edges.size() == 0);

    // Now try getting out edges for a node that does not exist
    bool assert_fail = false;
    try
    {
        edges = graph.get_out_edges(1500);
    }
    catch (GraphException &ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_out_nodes(EdgeKey graph)
{
    INFO();
    std::vector<node> nodes = graph.get_out_nodes(1);
    std::vector<node_id_t> nodes_id = graph.get_out_nodes_id(1);
    assert(nodes.size() == 3);
    assert(nodes_id.size() == 3);
    assert(nodes.at(0).id == SampleGraph::node2.id);  // edge(1->2)
    assert(nodes.at(0).id == nodes_id.at(0));
    assert(nodes.at(1).id == SampleGraph::node3.id);  // edge(1->3)
    assert(nodes.at(1).id == nodes_id.at(1));
    assert(nodes.at(2).id == SampleGraph::node7.id);  // edge(1->7)
    assert(nodes.at(2).id == nodes_id.at(2));
    // test for a node that has no out-edge
    nodes = graph.get_out_nodes(4);
    nodes_id = graph.get_out_nodes_id(4);
    assert(nodes.size() == 0);
    assert(nodes_id.size() == 0);

    // test for a node that does not exist
    bool assert_fail = false;
    try
    {
        nodes = graph.get_out_nodes(1500);
    }
    catch (GraphException &ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_in_edges(EdgeKey graph)
{
    INFO();
    std::vector<edge> edges = graph.get_in_edges(3);
    assert(edges.size() == 2);
    // Check edge0
    assert(edges.at(0).src_id == SampleGraph::edge2.src_id);
    assert(edges.at(0).dst_id == SampleGraph::edge2.dst_id);
    // Check edge1
    assert(edges.at(1).src_id == SampleGraph::edge3.src_id);
    assert(edges.at(1).dst_id == SampleGraph::edge3.dst_id);

    // now test for a node that has no in-edge
    edges = graph.get_in_edges(4);
    assert(edges.size() == 0);

    // Now try getting in edges for a node that does not exist.
    bool assert_fail = false;
    try
    {
        edges = graph.get_out_edges(1500);
    }
    catch (GraphException &ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_in_nodes(EdgeKey graph)
{
    INFO();
    std::vector<node> nodes = graph.get_in_nodes(3);
    std::vector<node_id_t> nodes_id = graph.get_in_nodes_id(3);
    assert(nodes.size() == 2);
    assert(nodes_id.size() == 2);
    assert(nodes.at(0).id == SampleGraph::node1.id);
    assert(nodes.at(0).id == nodes_id.at(0));
    assert(nodes.at(1).id == SampleGraph::node2.id);
    assert(nodes.at(1).id == nodes_id.at(1));

    // test for a node that has no in_edge
    nodes = graph.get_in_nodes(4);
    nodes_id = graph.get_in_nodes_id(4);
    assert(nodes.size() == 0);
    assert(nodes_id.size() == 0);

    // test for a node that does not exist
    bool assert_fail = false;
    try
    {
        nodes = graph.get_in_nodes(1500);
    }
    catch (GraphException &ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_delete_node(EdgeKey graph, bool is_directed)
{
    INFO();
    WT_CURSOR *e_cur = graph.get_edge_cursor();
    // Verify node2 exists
    assert(graph.has_node(SampleGraph::node2.id) == true);

    // Delete node2 and verify it was actually deleted
    graph.delete_node(SampleGraph::node2.id);
    e_cur->set_key(e_cur, SampleGraph::node2.id, -1);
    int ret = e_cur->search(e_cur);
    assert(ret != 0);

    // check that edge(2,3) is deleted
    e_cur->set_key(e_cur, 2, 3);
    assert(e_cur->search(e_cur) != 0);
    // check that edge(1,2) is deleted
    e_cur->set_key(e_cur, 1, 2);
    assert(e_cur->search(e_cur) != 0);

    // Now delete the reverse edges for undirected graph
    if (is_directed)
    {
        e_cur->set_key(e_cur, 3, 2);
        assert(e_cur->search(e_cur) != 0);
        e_cur->set_key(e_cur, 2, 1);
        assert(e_cur->search(e_cur) != 0);
    }
    // Verify that the in and out degrees of node 1 and 3 got updated
    if (is_directed)
    {
        assert(graph.get_out_degree(SampleGraph::node1.id) == 2);
        assert(graph.get_in_degree(SampleGraph::node3.id) == 1);
    }
    else
    {
        assert(graph.get_in_degree(SampleGraph::node1.id) == 2);
        assert(graph.get_out_degree(SampleGraph::node3.id) == 1);
    }
}

void test_get_in_and_out_degree(EdgeKey graph)
{
    INFO();
    int indeg, outdeg;
    indeg = graph.get_in_degree(3);
    outdeg = graph.get_out_degree(1);
    assert(indeg == 2);
    assert(outdeg == 3);
}

void tearDown(EdgeKey graph) { graph.close(); }

void test_InCursor(EdgeKey graph)
{
    INFO();
    InCursor *in_cursor = graph.get_innbd_iter();
    adjlist found = {0};
    in_cursor->next(&found);
    while (found.node_id != -1)
    {
        CommonUtil::dump_adjlist(found);
        in_cursor->next(&found);
    }
    in_cursor->reset();
    int nodeID = 0;
    in_cursor->next(&found, nodeID);
    CommonUtil::dump_adjlist(found);
}

void test_OutCursor(EdgeKey graph)
{
    INFO();
    OutCursor *out_cursor = graph.get_outnbd_iter();
    adjlist found = {0};
    out_cursor->next(&found);
    while (found.node_id != -1)
    {
        CommonUtil::dump_adjlist(found);
        found = {0};
        out_cursor->next(&found);
    }
    out_cursor->reset();
    int nodeID = 1;
    out_cursor->next(&found, nodeID);
    CommonUtil::dump_adjlist(found);
}

void test_NodeCursor(EdgeKey graph)
{
    INFO();
    NodeCursor *node_cursor = graph.get_node_iter();
    node found = {0, 0, 0};
    int nodeIdList[] = {1, 3, 4, 5, 6, 7, 8, 11};
    int i = 0;
    node_cursor->next(&found);
    while (found.id != -1)
    {
        assert(found.id == nodeIdList[i]);
        CommonUtil::dump_node(found);
        node_cursor->next(&found);
        i++;
    }
}

void test_NodeCursor_Range(EdgeKey graph)
{
    INFO();
    NodeCursor *node_cursor = graph.get_node_iter();
    node found;
    int nodeIdList[] = {3, 4, 5, 6};
    int i = 0;
    node_cursor->set_key_range(key_range{.start = 3, .end = 6});
    node_cursor->next(&found);
    while (found.id != -1)
    {
        assert(found.id == nodeIdList[i]);
        CommonUtil::dump_node(found);
        node_cursor->next(&found);
        i++;
    }
}

void test_EdgeCursor(EdgeKey graph)
{
    INFO();
    EdgeCursor *edge_cursor = graph.get_edge_iter();
    edge found;
    int srcIdList[] = {1, 1, 5, 7, 8};
    int dstIdList[] = {3, 7, 6, 8, 7};
    int i = 0;
    edge_cursor->next(&found);
    while (found.src_id != -1)
    {
        assert(found.src_id == srcIdList[i]);
        assert(found.dst_id == dstIdList[i]);
        CommonUtil::dump_edge(found);
        edge_cursor->next(&found);
        i++;
    }
}

void test_EdgeCursor_Range(EdgeKey graph)
{
    INFO();
    EdgeCursor *edge_cursor = graph.get_edge_iter();
    edge_cursor->set_key({1, 4}, {8, 1});
    edge found;
    int srcIdList[] = {1, 5, 7};
    int dstIdList[] = {7, 6, 8};
    int i = 0;
    edge_cursor->next(&found);
    while (found.src_id != -1)
    {
        assert(found.src_id == srcIdList[i]);
        assert(found.dst_id == dstIdList[i]);
        CommonUtil::dump_edge(found);
        edge_cursor->next(&found);
        i++;
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
    opts.db_name = "test_eKey";
    opts.db_dir = "./db";
    opts.conn_config = "cache_size=10GB";
    opts.stat_log = std::getenv("GRAPH_PROJECT_DIR");

    EdgeKeyTester graph = EdgeKeyTester(opts);
    create_init_nodes(graph, opts.is_directed);

    test_get_node(graph);
    test_node_add(graph, opts.read_optimize);
    test_get_nodes(graph);
    test_add_edge(graph, opts.is_directed);
    test_get_edge(graph);
    test_get_out_edges(graph);
    test_get_in_edges(graph);
    test_get_out_nodes(graph);
    test_get_in_nodes(graph);
    test_get_in_and_out_degree(graph);

    test_delete_node(graph, opts.is_directed);
    // // test_delete_isolated_node(graph, opts.is_directed);

    test_InCursor(graph);
    test_OutCursor(graph);

    test_NodeCursor(graph);
    test_NodeCursor_Range(graph);
    test_EdgeCursor(graph);
    test_EdgeCursor_Range(graph);

    tearDown(graph);
}
