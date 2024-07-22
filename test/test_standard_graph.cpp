#include <cassert>

#include "common_util.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define delim "--------------"
#define INFO() fprintf(stderr, "%s\nNow running: %s\n", delim, __FUNCTION__);

void test_get_nodes(StandardGraph &graph)
{
    INFO()

    for (node x : graph.get_nodes())
    {
        CommonUtil::dump_node(x);
    }
}

void create_init_nodes(StandardGraph &graph, bool is_directed)
{
    INFO()
    int test_node_count = 0;
    uint64_t ret;
    for (node x : SampleGraph::test_nodes)
    {
        std::cout << "Inserting node " << x.id << "\n";
        ret = graph.add_node(x);
        assert(ret == 0);
        test_node_count++;
    }
    assert(test_node_count == 6);
    // ret = graph.get_num_nodes();
    // assert(ret == 6);
    if (!is_directed)
    {
        SampleGraph::create_undirected_edges();
        std::cout << SampleGraph::test_edges.size() << "\n";
        assert(SampleGraph::test_edges.size() ==
               10);  // checking if directed edges got created and stored in
                     // test_edges
    }

    std::cout << "Insert Edges Now\n";
    size_t edge_cnt = 0;
    for (edge x : SampleGraph::test_edges)
    {
        std::cout << "Edge (" << x.src_id << " , " << x.dst_id << ")\n";
        ret = graph.add_edge(x, false);
        assert(ret == 0);
        edge_cnt++;
    }
    assert(edge_cnt == SampleGraph::test_edges.size());
}

void tearDown(StandardGraph &graph) { graph.close(true); }

void test_node_add(StandardGraph &graph, bool read_optimize)
{
    INFO()
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

void test_node_delete(StandardGraph &graph, bool is_directed)
{
    INFO()
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

void test_get_edge_id(StandardGraph &graph)
{
    INFO()
    assert(graph.has_edge(1, 2) == true);  // Edge ID 1
    assert(graph.has_edge(1, 3) == true);  // Edge ID 2
    assert(graph.has_edge(2, 3) == true);  // Edge ID 3

    assert(graph.has_edge(10, 11) == false);  // non-existent edge
}

void test_get_num_nodes_and_edges(StandardGraph &graph, bool is_directed)
{
    INFO()
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

void test_add_edge(StandardGraph &graph, bool is_directed, bool is_weighted)
{
    INFO()
    edge to_insert = {
        .src_id = 5,
        .dst_id = 6,
        .edge_weight = 333};  // node 5 and 6 dont exist yet so we must also
                              // check if the nodes get created
    int test_id1 = 5, test_id2 = 6;
    graph.add_edge(to_insert, false);

    edge found = graph.get_edge(test_id1, test_id2);
    CommonUtil::dump_edge(found);
    if (is_weighted)
    {
        assert(found.edge_weight == 333);
    }
    if (!is_directed)
    {
        found = graph.get_edge(test_id2, test_id1);
        assert(found.edge_weight == 333);
    }
}

void test_get_node(StandardGraph &graph)
{
    INFO()
    node found = graph.get_node(1);
    assert(found.id == 1);
}

void test_get_in_degree(StandardGraph &graph, bool is_directed)
{
    INFO()
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

void test_get_out_degree(StandardGraph &graph, bool is_directed)
{
    INFO()
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

void test_get_out_edges(StandardGraph &graph)
{
    INFO()
    int test_id1 = 1, test_id2 = 4, test_id3 = 1500;
    std::vector<edge> edges = graph.get_out_edges(test_id1);
    assert(edges.size() == 3);
    // compare edge0
    assert(edges.at(0).src_id == SampleGraph::edge1.src_id);
    assert(edges.at(0).dst_id == SampleGraph::edge1.dst_id);
    // compare edge1
    assert(edges.at(1).src_id == SampleGraph::edge2.src_id);
    assert(edges.at(1).dst_id == SampleGraph::edge2.dst_id);
    // compare edge4
    assert(edges.at(2).src_id == SampleGraph::edge4.src_id);
    assert(edges.at(2).dst_id == SampleGraph::edge4.dst_id);

    // Now test for a node that has no out edge
    edges = graph.get_out_edges(test_id2);
    assert(edges.empty());

    // Now try getting out edges for a node that does not exist
    bool assert_fail = false;
    try
    {
        edges = graph.get_out_edges(test_id3);
    }
    catch (GraphException &ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_out_nodes(StandardGraph &graph)
{
    INFO()
    int test_id1 = 1, test_id2 = 4, test_id3 = 1500;
    std::vector<node> nodes = graph.get_out_nodes(test_id1);
    std::vector<node_id_t> nodes_id = graph.get_out_nodes_id(test_id1);
    assert(nodes.size() == 3);
    assert(nodes_id.size() == 3);
    assert(nodes.at(0).id == SampleGraph::node2.id);  // edge(1->2)
    assert(nodes.at(0).id == nodes_id.at(0));
    assert(nodes.at(1).id == SampleGraph::node3.id);  // edge(1->3)
    assert(nodes.at(1).id == nodes_id.at(1));
    assert(nodes.at(2).id == SampleGraph::node7.id);  // edge(1->7)
    assert(nodes.at(2).id == nodes_id.at(2));

    // test for a node that has no out-edge
    nodes = graph.get_out_nodes(test_id2);
    nodes_id = graph.get_out_nodes_id(test_id2);
    assert(nodes.empty());
    assert(nodes_id.empty());

    // test for a node that does not exist
    bool assert_fail = false;
    try
    {
        nodes = graph.get_out_nodes(test_id3);
    }
    catch (GraphException &ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);

    assert_fail = false;
    try
    {
        nodes_id = graph.get_out_nodes_id(test_id3);
    }
    catch (GraphException &ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_in_nodes(StandardGraph &graph)
{
    INFO()
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
    assert(nodes.empty());
    assert(nodes_id.empty());

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

void test_get_in_edges(StandardGraph &graph)
{
    INFO()
    std::vector<edge> edges = graph.get_in_edges(3);
    assert(edges.size() == 2);
    // Check edge0
    assert(edges.at(0).src_id == SampleGraph::edge2.src_id);
    assert(edges.at(0).dst_id == SampleGraph::edge2.dst_id);
    // Check edge1
    assert(edges.at(1).src_id == SampleGraph::edge3.src_id);
    assert(edges.at(1).dst_id == SampleGraph::edge3.dst_id);

    test_get_nodes(graph);
    // now test for a node that has no in-edge
    edges = graph.get_in_edges(4);
    assert(edges.empty());

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

void test_cursor(StandardGraph &graph)
{
    INFO()
    for (edge x : graph.test_cursor_iter(2))
    {
        CommonUtil::dump_edge(x);
    }
}

void test_get_edges(StandardGraph &graph, bool is_directed)
{
    INFO()
    vector<edge> edges = graph.get_edges();
    for (auto j : edges)
    {
        CommonUtil::dump_edge(j);
    }

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
    assert(found.src_id == UINT32_MAX);
    assert(found.dst_id == UINT32_MAX);
    assert(found.edge_weight == 0);
}

void test_InCursor(StandardGraph &graph)
{
    INFO()
    InCursor *in_cursor = graph.get_innbd_iter();
    adjlist found;
    in_cursor->next(&found);
    while (found.node_id != UINT32_MAX)
    {
        CommonUtil::dump_adjlist(found);
        found.clear();
        in_cursor->next(&found);
    }

    std::cout << "-------------------\n"
              << "now testing with a set range\n"
              << "-------------------\n";
    
    in_cursor->reset();
    in_cursor->set_key_range(key_range(3, 5));
    in_cursor->next(&found);
    while (found.node_id != UINT32_MAX)
    {
        CommonUtil::dump_adjlist(found);
        found.clear();
        in_cursor->next(&found);
    }
    in_cursor->close();
    delete in_cursor;
}

void test_OutCursor(StandardGraph &graph)
{
    INFO()
    OutCursor *out_cursor = graph.get_outnbd_iter();
    adjlist found;
    out_cursor->next(&found);
    while (found.node_id != UINT32_MAX)
    {
        CommonUtil::dump_adjlist(found);
        found.clear();
        out_cursor->next(&found);
    }
    std::cout << "-------------------\n"
              << "now testing with a set range\n"
              << "-------------------\n";
    out_cursor->reset();
    out_cursor->set_key_range(key_range(3, 5));
    out_cursor->next(&found);
    while (found.node_id != UINT32_MAX)
    {
        CommonUtil::dump_adjlist(found);
        found.clear();
        out_cursor->next(&found);
    }
    out_cursor->close();
    delete out_cursor;
}

void test_index_cursor(StandardGraph &graph)
{
    INFO()
    WT_CURSOR *srccur = graph.get_src_idx_cursor();
    WT_CURSOR *dstcur = graph.get_dst_idx_cursor();

    srccur->reset(srccur);
    dstcur->reset(dstcur);
    edge found = {0};
    while (srccur->next(srccur) == 0)
    {
        CommonUtil::read_from_edge_idx(srccur, &found);
        CommonUtil::dump_edge(found);
    }
    std::cout << "-------------------\n"
              << "dumping dst_idx\n"
              << "-------------------\n";
    while (dstcur->next(dstcur) == 0)
    {
        CommonUtil::read_from_edge_idx(dstcur, &found);
        CommonUtil::dump_edge(found);
    }
}

void test_NodeCursor(StandardGraph &graph)
{
    INFO()
    NodeCursor *node_cursor = graph.get_node_iter();
    node found;
    node_id_t nodeIdList[] = {1, 3, 4, 5, 6, 7, 8};
    int i = 0;
    node_cursor->next(&found);
    while (found.id != UINT32_MAX)
    {
        assert(found.id == nodeIdList[i]);
        CommonUtil::dump_node(found);
        node_cursor->next(&found);
        i++;
    }
    node_cursor->close();
    delete node_cursor;
}

void test_NodeCursor_Range(StandardGraph &graph)
{
    INFO()
    NodeCursor *node_cursor = graph.get_node_iter();
    node found;
    node_id_t nodeIdList[] = {3, 4, 5, 6};
    int i = 0;
    node_cursor->set_key_range(key_range(3, 6));
    node_cursor->next(&found);
    while (found.id != UINT32_MAX)
    {
        assert(found.id == nodeIdList[i]);
        CommonUtil::dump_node(found);
        node_cursor->next(&found);
        i++;
    }
    node_cursor->close();
    delete node_cursor;
}

void test_EdgeCursor(StandardGraph &graph)
{
    INFO()
    EdgeCursor *edge_cursor = graph.get_edge_iter();
    edge found;
    node_id_t srcIdList[] = {1, 1, 5, 7, 8};
    node_id_t dstIdList[] = {3, 7, 6, 8, 7};
    int i = 0;
    edge_cursor->next(&found);
    while (found.src_id != OutOfBand_ID_MAX)
    {
        assert(found.src_id == srcIdList[i]);
        assert(found.dst_id == dstIdList[i]);
        CommonUtil::dump_edge(found);
        edge_cursor->next(&found);
        i++;
    }
    edge_cursor->close();
    delete edge_cursor;
}

void test_EdgeCursor_Range(StandardGraph &graph)
{
    INFO()
    EdgeCursor *edge_cursor = graph.get_edge_iter();
    edge_cursor->set_key_range(edge_range(key_pair{1, 4}, key_pair{8, 1}));
    
    edge found;
    node_id_t srcIdList[] = {1, 5, 7};
    node_id_t dstIdList[] = {7, 6, 8};
    int i = 0;
    edge_cursor->next(&found);
    while (found.src_id != OutOfBand_ID_MAX)
    {
        assert(found.src_id == srcIdList[i]);
        assert(found.dst_id == dstIdList[i]);
        CommonUtil::dump_edge(found);
        edge_cursor->next(&found);
        i++;
    }
    edge_cursor->close();
    delete edge_cursor;
}

int main()
{
    const int THREAD_NUM = 1;
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    // opts.is_directed = false;
    // opts.read_optimize = false;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.type = GraphType::Std;
    opts.db_name = "test_std";
    opts.db_dir = "./db";
    opts.conn_config = "cache_size=10GB";
    if (const char *env_p = std::getenv("GRAPH_PROJECT_DIR"))
    {
        opts.stat_log = std::string(env_p);
    }
    else
    {
        std::cout << "GRAPH_PROJECT_DIR not set. Using CWD" << std::endl;
        opts.stat_log = "./";
    }

    // Test std_graph setup
    GraphEngine myEngine(THREAD_NUM, opts);
    WT_CONNECTION *conn = myEngine.get_connection();

    StandardGraph graph(opts, conn);
    create_init_nodes(graph, opts.is_directed);
    //Test get_node()
    test_get_node(graph);
    // Test get_in_degree()
    test_get_in_degree(graph, opts.is_directed);

    // Test get_out_degree()
    test_get_out_degree(graph, opts.is_directed);

    // test get_nodes()
    test_get_nodes(graph);

    // Test adding a node
    test_node_add(graph, opts.read_optimize);
    test_add_edge(graph, opts.is_directed, opts.is_weighted);
    // test get_edge and get_edges
    test_get_edges(graph, opts.is_directed);

    test_cursor(graph);
    test_index_cursor(graph);

    // test get_in_nodes_and_edges
    test_get_in_edges(graph);
    test_get_in_nodes(graph);

    // test get_out_nodes_and_edges
    test_get_out_edges(graph);
    test_get_out_nodes(graph);

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