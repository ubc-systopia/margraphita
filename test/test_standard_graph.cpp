#include "test_standard_graph.h"

#include <cassert>

#include "common.h"
#include "graph_exception.h"
#include "sample_graph.h"
#include "standard_graph.h"

#define INFO() fprintf(stderr, "Now running: %s\n", __FUNCTION__);

void create_init_nodes(StandardGraph graph, bool is_directed)
{
    INFO()
    for (node x : SampleGraph::test_nodes)
    {
        graph.add_node(x);
    }

    if (!is_directed)
    {
        SampleGraph::create_undirected_edges();
        std::cout << SampleGraph::test_edges.size() << "\n";
        assert(SampleGraph::test_edges.size() ==
               10);  // checking if directed edges got created and stored in
                     // test_edges
    }
    int edge_cnt = 1;
    for (edge x : SampleGraph::test_edges)
    {
        graph.add_edge(x, false);
        edge_cnt++;
    }
}

void tearDown(StandardGraph graph) { graph.close(); }

void test_node_add(StandardGraph graph, bool read_optimize)
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

void test_node_delete(StandardGraph graph, bool is_directed)
{
    INFO();
    node to_delete = SampleGraph::node2;
    graph.delete_node(to_delete.id);

    // check that the node has been deleted
    assert(graph.has_node(to_delete.id) == false);

    // check if the edges have been deleted
    if (is_directed)
    {
        assert(graph.has_edge(SampleGraph::edge1.src_id,
                              SampleGraph::edge1.dst_id) == false);
        assert(graph.has_edge(SampleGraph::edge3.src_id,
                              SampleGraph::edge3.dst_id) == false);
    }
    else
    {
        // edge from 1->2
        assert(graph.has_edge(SampleGraph::edge1.src_id,
                              SampleGraph::edge1.dst_id) == false);
        // edge from 2->1
        assert(graph.has_edge(SampleGraph::edge1.dst_id,
                              SampleGraph::edge1.src_id) == false);
        // edge from 2->3
        assert(graph.has_edge(SampleGraph::edge3.src_id,
                              SampleGraph::edge3.dst_id) == false);
        // edge from 3->2
        assert(graph.has_edge(SampleGraph::edge3.dst_id,
                              SampleGraph::edge3.src_id) == false);
    }

    int isolatedNodeID = 11;
    graph.delete_node(isolatedNodeID);
    assert(graph.has_node(isolatedNodeID) == false);
}

void test_get_edge_id(StandardGraph graph)
{
    INFO();
    assert(graph.has_edge(1, 2) == true);  // Edge ID 1
    assert(graph.has_edge(1, 3) == true);  // Edge ID 2
    assert(graph.has_edge(2, 3) == true);  // Edge ID 3

    assert(graph.has_edge(10, 11) == false);  // non-existent edge
}

void test_get_num_nodes_and_edges(StandardGraph graph, bool is_directed)
{
    INFO();
    if (is_directed)
    {
        assert(graph.get_num_nodes() == 6);
        assert(graph.get_num_edges() == 6);
    }
    else
    {
        assert(graph.get_num_nodes() == 6);
        assert(graph.get_num_edges() == 10);
    }
}

void test_add_edge(StandardGraph graph, bool is_directed)
{
    INFO();
    edge to_insert = {
        .src_id = 5,
        .dst_id = 6,
        .edge_weight = 333};  // node 5 and 6 dont exist yet so we must also
                              // check if the nodes get created
    int test_id1 = 5, test_id2 = 6;
    graph.add_edge(to_insert, false);
    edge found = graph.get_edge(test_id1, test_id2);
    CommonUtil::dump_edge(found);
    assert(found.edge_weight == 333);
    if (!is_directed)
    {
        found = graph.get_edge(test_id2, test_id1);
        assert(found.edge_weight == 333);
    }
}

void test_get_node(StandardGraph graph)
{
    INFO();
    node found = graph.get_node(1);
    assert(found.id == 1);
}

void test_get_in_degree(StandardGraph graph, bool is_directed)
{
    INFO();
    if (is_directed)
    {
        assert(graph.get_in_degree(1) == 0);
        assert(graph.get_in_degree(2) == 1);
        assert(graph.get_in_degree(3) == 2);
        assert(graph.get_in_degree(7) == 2);
        assert(graph.get_in_degree(8) == 1);
    }
    else
    {
        assert(graph.get_in_degree(1) == 3);
        assert(graph.get_in_degree(2) == 2);
        assert(graph.get_in_degree(3) == 2);
        assert(graph.get_in_degree(7) == 2);
        assert(graph.get_in_degree(8) == 1);
    }
}

void test_get_out_degree(StandardGraph graph, bool is_directed)
{
    INFO();
    if (is_directed)
    {
        assert(graph.get_out_degree(1) == 3);
        assert(graph.get_out_degree(2) == 1);
        assert(graph.get_out_degree(3) == 0);
    }
    else
    {
        assert(graph.get_out_degree(1) == 3);
        assert(graph.get_out_degree(2) == 2);
        assert(graph.get_out_degree(3) == 2);
    }
}

void test_get_nodes(StandardGraph graph)
{
    INFO();
    for (node x : graph.get_nodes())
    {
        CommonUtil::dump_node(x);
    }
}

void test_get_in_nodes_and_edges(StandardGraph graph, bool is_directed)
{
    INFO();
    vector<edge> got_e = graph.get_in_edges(3);
    vector<node> got_n = graph.get_in_nodes(3);
    if (is_directed)
    {
        assert(got_e.size() == 2);
        assert(got_n.size() == 2);
    }
    else
    {
        assert(got_e.size() == 2);
        assert(got_n.size() == 2);
    }
    assert(graph.get_in_degree(3) == got_n.size());
    assert(graph.get_in_degree(3) == got_e.size());
}

void test_get_out_nodes_and_edges(StandardGraph graph, bool is_directed)
{
    INFO();
    vector<edge> got_e = graph.get_out_edges(3);
    vector<node> got_n = graph.get_out_nodes(3);
    if (!is_directed)
    {
        assert(got_e.size() == 2);
        assert(got_n.size() == 2);
    }
    else
    {
        assert(got_e.size() == 0);
        assert(got_n.size() == 0);
    }

    for (edge x : got_e)
    {
        CommonUtil::dump_edge(x);
    }

    for (node x : got_n)
    {
        CommonUtil::dump_node(x);
    }
}

void test_cursor(StandardGraph graph)
{
    INFO();
    for (edge x : graph.test_cursor_iter(2))
    {
        CommonUtil::dump_edge(x);
    }
}

void test_get_edges(StandardGraph graph, bool is_directed)
{
    INFO();
    vector<edge> edges = graph.get_edges();

    if (!is_directed)
    {
        assert(edges.size() == 12);
    }
    else
    {
        assert(edges.size() == 7);
    }
    edge found =
        graph.get_edge(SampleGraph::edge1.src_id, SampleGraph::edge1.dst_id);
    assert(found.src_id == SampleGraph::edge1.src_id);
    assert(found.dst_id == SampleGraph::edge1.dst_id);
    // not testing weight since not set

    // Now get a non-existent edge
    int test_id1 = 222, test_id2 = 333;
    found = graph.get_edge(test_id1, test_id2);
    assert(found.src_id == -1);
    assert(found.dst_id == -1);
    assert(found.edge_weight == -1);
}

void print_delim() { cout << endl << "--------------------" << endl; }

void test_InCursor(StandardGraph graph)
{
    INFO();
    StdIterator::InCursor in_cursor = graph.get_innbd_iter();
    adjlist found = {0};
    in_cursor.next(&found);
    while (found.node_id != -1)
    {
        CommonUtil::dump_adjlist(found);
        found = {0};
        in_cursor.next(&found);
    }

    in_cursor.reset();

    in_cursor.next(&found, 3);
    assert(found.node_id == 3);
    assert(found.edgelist.size() ==
           1);  // size is 1 after deleting node 2; 2 if not deleted
}

void test_OutCursor(StandardGraph graph)
{
    INFO();
    auto out_cursor = graph.get_outnbd_iter();
    adjlist found = {0};
    while (out_cursor.has_more())
    {
        out_cursor.next(&found);
        if (found.node_id != -1)
        {
            CommonUtil::dump_adjlist(found);
            found = {0};
        }
        else
        {
            break;
        }
    }

    out_cursor.reset();
    out_cursor.next(&found, 1);
    assert(found.node_id == 1);
    assert(found.edgelist.size() == 2);
}

void test_index_cursor(StandardGraph graph)
{
    WT_CURSOR *srccur = graph.get_src_idx_cursor();
    WT_CURSOR *dstcur = graph.get_dst_idx_cursor();

    srccur->reset(srccur);
    dstcur->reset(dstcur);
    edge found = {0};

    while (srccur->next(srccur) == 0)
    {
        CommonUtil::__read_from_edge_idx(srccur, &found);
        CommonUtil::dump_edge(found);
    }
    print_delim();
    while (dstcur->next(dstcur) == 0)
    {
        CommonUtil::__read_from_edge_idx(dstcur, &found);
        CommonUtil::dump_edge(found);
    }
}

void test_NodeCursor(StandardGraph graph)
{
    INFO();
    StdIterator::NodeCursor node_cursor = graph.get_node_iter();
    node found;
    int nodeIdList[] = {1, 3, 4, 5, 6, 7, 8};
    int i = 0;
    node_cursor.next(&found);
    while (found.id != -1)
    {
        assert(found.id == nodeIdList[i]);
        CommonUtil::dump_node(found);
        node_cursor.next(&found);
        i++;
    }
}

void test_NodeCursor_Range(StandardGraph graph)
{
    INFO();
    StdIterator::NodeCursor node_cursor = graph.get_node_iter();
    node found;
    int nodeIdList[] = {3, 4, 5, 6};
    int i = 0;
    node_cursor.set_key_range(key_range{.start = 3, .end = 6});
    node_cursor.next(&found);
    while (found.id != -1)
    {
        assert(found.id == nodeIdList[i]);
        CommonUtil::dump_node(found);
        node_cursor.next(&found);
        i++;
    }
}

void test_EdgeCursor(StandardGraph graph)
{
    INFO();
    StdIterator::EdgeCursor edge_cursor = graph.get_edge_iter();
    edge found;
    int srcIdList[] = {1, 1, 5, 7, 8};
    int dstIdList[] = {3, 7, 6, 8, 7};
    int i = 0;
    edge_cursor.next(&found);
    while (found.src_id != -1)
    {
        assert(found.src_id == srcIdList[i]);
        assert(found.dst_id == dstIdList[i]);
        CommonUtil::dump_edge(found);
        edge_cursor.next(&found);
        i++;
    }
}

void test_EdgeCursor_Range(StandardGraph graph)
{
    INFO();
    StdIterator::EdgeCursor edge_cursor = graph.get_edge_iter();
    edge_cursor.set_key({1, 4}, {8, 1});
    edge found;
    int srcIdList[] = {1, 5, 7};
    int dstIdList[] = {7, 6, 8};
    int i = 0;
    edge_cursor.next(&found);
    while (found.src_id != -1)
    {
        assert(found.src_id == srcIdList[i]);
        assert(found.dst_id == dstIdList[i]);
        CommonUtil::dump_edge(found);
        edge_cursor.next(&found);
        i++;
    }
}

int main()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    // opts.is_directed = false;
    // opts.read_optimize = false;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_name = "test_std";
    opts.db_dir = "./db";
    opts.conn_config = "cache_size=10GB";
    opts.stat_log = "/home/puneet/scratch/margraphita/profile/test";

    // Test std_graph setup
    StandardGraphTester graph = StandardGraphTester(opts);

    create_init_nodes(graph, opts.is_directed);
    print_delim();

    test_index_cursor(graph);
    print_delim();
    // Test num_get_nodes and num_get_edges

    test_get_num_nodes_and_edges(graph, opts.is_directed);
    print_delim();

    // Test get_node()
    test_get_node(graph);
    print_delim();

    // Test get_in_degree()
    test_get_in_degree(graph, opts.is_directed);
    print_delim();

    // Test get_out_degree()
    test_get_out_degree(graph, opts.is_directed);
    print_delim();

    // test get_nodes()
    test_get_nodes(graph);
    print_delim();

    // Test adding a node
    test_node_add(graph, opts.read_optimize);
    print_delim();

    test_add_edge(graph, opts.is_directed);

    // test get_edge_id()
    test_get_edge_id(graph);
    print_delim();

    // test get_edge and get_edges
    test_get_edges(graph, opts.is_directed);
    print_delim();

    // test_cursor(graph);

    // test get_in_nodes_and_edges
    test_get_in_nodes_and_edges(graph, opts.is_directed);
    print_delim();

    // test get_out_nodes_and_edges
    test_get_out_nodes_and_edges(graph, opts.is_directed);
    print_delim();

    // Test deleting a node
    test_node_delete(graph, opts.is_directed);

    test_InCursor(graph);
    test_OutCursor(graph);
    test_NodeCursor(graph);
    test_NodeCursor_Range(graph);
    test_EdgeCursor(graph);
    test_EdgeCursor_Range(graph);

    // Test std_graph teardown
    tearDown(graph);

    return 0;
}