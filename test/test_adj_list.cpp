#include <cassert>

#include "common_util.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define delim "--------------"
#define INFO() fprintf(stdout, "%s\nNow running: %s\n", delim, __FUNCTION__);

void create_init_nodes(AdjList &graph, bool is_directed)
{
    INFO()
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

void test_node_add(AdjList graph, bool read_optimize)
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
    // Check the adjlists
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
    int test_id1 = 1, test_id2 = -1;
    node found = graph.get_node(test_id1);
    assert(found.id == 1);
    // now get a node that does not exist
    found = graph.get_node(test_id2);
    assert(found.id == OutOfBand_ID_MAX);
    found = graph.get_random_node();
    CommonUtil::dump_node(found);
}
void test_get_nodes(AdjList graph)
{
    INFO();
    int n = 10;
    for (node x : graph.get_nodes())
    {
        CommonUtil::dump_node(x);
        if (--n == 0) break;
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
    std::cout << "Printing the in_adjlist for node " << node_id << std::endl;
    WT_CURSOR *in_adj_cur = graph.get_in_adjlist_cursor();
    WT_CURSOR *out_adj_cur = graph.get_out_adjlist_cursor();
    for (auto al : graph.get_adjlist(in_adj_cur, node_id))
    {
        std::cout << al << " ";
    }
    std::cout << "Printing the out_adjlist for node " << node_id << std::endl;
    for (auto al : graph.get_adjlist(out_adj_cur, node_id))
    {
        std::cout << al << " ";
    }
}

void test_get_edge(AdjList graph)
{
    INFO();
    edge found =
        graph.get_edge(SampleGraph::edge1.src_id, SampleGraph::edge1.dst_id);
    assert(found.src_id == SampleGraph::edge1.src_id);
    assert(found.dst_id == SampleGraph::edge1.dst_id);
    // not testing weight since not set

    // Now get a non-existent edge
    int test_id1 = 222, test_id2 = 333;
    found = graph.get_edge(test_id1, test_id2);
    assert(found.src_id == OutOfBand_ID_MAX);
    assert(found.dst_id == OutOfBand_ID_MAX);
    assert(found.edge_weight == 0);
}

void test_add_edge(AdjList graph, bool is_directed)
{
    INFO();
    edge to_insert = {
        .src_id = 5,
        .dst_id = 6,
        .edge_weight = 333};  // node 300 and 400 dont exist yet so we must also
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
    // Check if the nodes were created.
    node got = graph.get_node(test_id1);
    assert(got.id == 5);
    got = graph.get_node(test_id2);
    assert(got.id == 6);

    // Now check if the adjlists were updated
    //  graph.dump_tables();
    WT_CURSOR *in_adj_cur = graph.get_in_adjlist_cursor();
    CommonUtil::set_key(in_adj_cur, test_id2);
    assert(in_adj_cur->search(in_adj_cur) == 0);
    in_adj_cur->reset(in_adj_cur);
    std::vector<node_id_t> adjlist = graph.get_adjlist(in_adj_cur, test_id2);
    assert(adjlist.size() == 1);
    if (!is_directed)
    {
        CommonUtil::set_key(in_adj_cur, test_id1);
        assert(in_adj_cur->search(in_adj_cur) == 0);
        in_adj_cur->reset(in_adj_cur);
        adjlist = graph.get_adjlist(in_adj_cur, test_id2);
        assert(adjlist.size() == 1);
    }

    WT_CURSOR *out_adj_cur = graph.get_out_adjlist_cursor();
    CommonUtil::set_key(out_adj_cur, test_id1);
    assert(out_adj_cur->search(out_adj_cur) == 0);
    out_adj_cur->reset(out_adj_cur);
    adjlist = graph.get_adjlist(out_adj_cur, test_id1);
    assert(adjlist.size() == 1);

    if (!is_directed)
    {
        CommonUtil::set_key(out_adj_cur, test_id2);
        out_adj_cur->search(out_adj_cur);
        assert(out_adj_cur->search(out_adj_cur) == 0);
        out_adj_cur->reset(out_adj_cur);
        adjlist = graph.get_adjlist(out_adj_cur, test_id2);
        assert(adjlist.size() == 1);
    }
}

void test_get_out_edges(AdjList graph)
{
    INFO();
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

void test_get_in_edges(AdjList graph)
{
    INFO();
    int test_id1 = 3, test_id2 = 4, test_id3 = 1500;
    std::vector<edge> edges = graph.get_in_edges(test_id1);
    assert(edges.size() == 2);
    // Check edge0
    assert(edges.at(0).src_id == SampleGraph::edge2.src_id);
    assert(edges.at(0).dst_id == SampleGraph::edge2.dst_id);
    // Check edge1
    assert(edges.at(1).src_id == SampleGraph::edge3.src_id);
    assert(edges.at(1).dst_id == SampleGraph::edge3.dst_id);

    // now test for a node that has no in-edge
    edges = graph.get_in_edges(test_id2);
    assert(edges.empty());

    // Now try getting in edges for a node that does not exist.
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

void test_get_out_nodes(AdjList graph)
{
    INFO();
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

void test_get_in_nodes(AdjList graph)
{
    INFO();
    int test_id1 = 3, test_id2 = 4, test_id3 = 1500;
    std::vector<node> nodes = graph.get_in_nodes(test_id1);
    std::vector<node_id_t> nodes_id = graph.get_in_nodes_id(test_id1);
    assert(nodes.size() == 2);
    assert(nodes_id.size() == 2);
    assert(nodes.at(0).id == SampleGraph::node1.id);
    assert(nodes.at(0).id == nodes_id.at(0));
    assert(nodes.at(1).id == SampleGraph::node2.id);
    assert(nodes.at(1).id == nodes_id.at(1));

    // test for a node that has no in_edge
    nodes = graph.get_in_nodes(test_id2);
    nodes_id = graph.get_in_nodes_id(test_id2);
    assert(nodes.empty());
    assert(nodes_id.empty());

    // test for a node that does not exist
    bool assert_fail = false;
    try
    {
        nodes = graph.get_in_nodes(test_id3);
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
        nodes_id = graph.get_in_nodes_id(test_id3);
    }
    catch (GraphException &ex)
    {
        cout << ex.what() << endl;
        assert_fail = true;
    }
    assert(assert_fail);
}

void test_get_in_degree(AdjList graph)
{
    INFO();
    // check in_degree for node3
    int test_id = 3;
    auto deg = graph.get_in_degree(test_id);
    assert(deg == 2);
}

void test_delete_node(AdjList graph, bool is_directed)
{
    INFO();
    WT_CURSOR *n_cursor = graph.get_node_cursor();
    WT_CURSOR *e_cursor = graph.get_edge_cursor();
    WT_CURSOR *adj_out_cur = graph.get_out_adjlist_cursor();
    WT_CURSOR *adj_in_cur = graph.get_in_adjlist_cursor();

    // Verify node2 exists
    CommonUtil::set_key(n_cursor, SampleGraph::node2.id);
    int ret = n_cursor->search(n_cursor);
    assert(ret == 0);
    n_cursor->reset(n_cursor);

    // Delete node2 and verify it was actually deleted
    graph.delete_node(SampleGraph::node2.id);
    CommonUtil::set_key(n_cursor, SampleGraph::node2.id);
    ret = n_cursor->search(n_cursor);
    assert(ret != 0);

    // Verify node2's adjacency lists are deleted
    CommonUtil::set_key(adj_out_cur, SampleGraph::node2.id);
    ret = adj_out_cur->search(adj_out_cur);
    assert(ret != 0);
    CommonUtil::set_key(adj_in_cur, SampleGraph::node2.id);
    ret = adj_in_cur->search(adj_in_cur);
    assert(ret != 0);

    // check that edge(2,3) is deleted
    CommonUtil::set_key(e_cursor, 2, 3);
    assert(e_cursor->search(e_cursor) != 0);
    // check that edge(1,2) is deleted
    CommonUtil::set_key(e_cursor, 1, 2);
    assert(e_cursor->search(e_cursor) != 0);
    // Now delete the reverse edges for undirected graph
    if (is_directed)
    {
        CommonUtil::set_key(e_cursor, 3, 2);
        assert(e_cursor->search(e_cursor) != 0);
        CommonUtil::set_key(e_cursor, 2, 1);
        assert(e_cursor->search(e_cursor) != 0);
    }
    // Verify that node 2 is deleted from adjlist of node1 and node3
    adj_out_cur->reset(adj_out_cur);
    for (auto dst : graph.get_adjlist(adj_out_cur, SampleGraph::node1.id))
    {
        assert(dst != SampleGraph::node2.id);  // node2 should have been deleted
    }

    adj_in_cur->reset(adj_in_cur);
    for (auto src : graph.get_adjlist(adj_in_cur, SampleGraph::node3.id))
    {
        assert(src != SampleGraph::node2.id);  // node 2 should have been
                                               //  deleted from this too
    }
}

void test_delete_isolated_node(AdjList graph, bool is_directed)
{
    INFO();
    WT_CURSOR *n_cursor = graph.get_node_cursor();
    WT_CURSOR *adj_out_cur = graph.get_out_adjlist_cursor();
    WT_CURSOR *adj_in_cur = graph.get_in_adjlist_cursor();

    // Verify node4 exists
    CommonUtil::set_key(n_cursor, SampleGraph::node4.id);
    int ret = n_cursor->search(n_cursor);
    assert(ret == 0);
    n_cursor->reset(n_cursor);

    // Delete node4 and verify it was actually deleted
    graph.delete_node(SampleGraph::node4.id);
    CommonUtil::set_key(n_cursor, SampleGraph::node4.id);
    ret = n_cursor->search(n_cursor);
    assert(ret != 0);

    // Verify node4's adjacency lists are deleted
    CommonUtil::set_key(adj_out_cur, SampleGraph::node4.id);
    ret = adj_out_cur->search(adj_out_cur);
    assert(ret != 0);
    CommonUtil::set_key(adj_in_cur, SampleGraph::node4.id);
    ret = adj_in_cur->search(adj_in_cur);
    assert(ret != 0);

    // Check no edge has node4 in source or dst
    std::vector<edge> edges = graph.get_edges();
    for (edge e : edges)
    {
        assert(e.src_id != SampleGraph::node4.id);
        assert(e.dst_id != SampleGraph::node4.id);
    }

    // Now check if node4 is present in in/out_adj_list of any of the remaining
    // nodes;
    std::vector<int> remaining_nodes = {1, 3};

    for (int n : remaining_nodes)
    {
        adj_out_cur->reset(adj_out_cur);
        for (auto dst : graph.get_adjlist(adj_out_cur, n))
        {
            assert(dst !=
                   SampleGraph::node4.id);  // node4 should not exist here
        }
        adj_in_cur->reset(adj_in_cur);
        for (auto src : graph.get_adjlist(adj_in_cur, n))
        {
            assert(src !=
                   SampleGraph::node4.id);  // node 4 should not exist here
        }
    }
}

void test_InCursor(AdjList graph)
{
    INFO();
    auto *in_cursor = (AdjInCursor *)graph.get_innbd_iter();
    in_cursor->setAllNodes(true);
    std::cout << "Printing in-adjlists for all nodes (AllNodes=true)\n"
              << std::endl;
    adjlist found;
    in_cursor->next(&found);
    while (found.node_id != OutOfBand_ID_MAX)
    {
        CommonUtil::dump_adjlist(found);
        found.clear();
        in_cursor->next(&found);
    }
    std::cout << "the found.node id is " << found.node_id << std::endl;
    assert(found.node_id == OutOfBand_ID_MAX);
    found.clear();
    //    std::cout
    //        << "Printing in-adjlists for nodes with non-null nbd
    //        (AllNodes=false)\n"
    //        << std::endl;
    //    in_cursor->setAllNodes(false);
    //    adjlist found;
    //    in_cursor->next(&found);
    //    while (found.node_id != OutOfBand_ID_MAX)
    //    {
    //        CommonUtil::dump_adjlist(found);
    //        found.clear();
    //        in_cursor->next(&found);
    //    }
    in_cursor->close();
    delete in_cursor;
}

void test_OutCursor(AdjList graph)
{
    INFO();
    adjlist found;

    auto *out_cursor = (AdjOutCursor *)graph.get_outnbd_iter();
    out_cursor->setAllNodes(true);
    std::cout << "Printing in-adjlists for all nodes (AllNodes=true)\n";
    out_cursor->next(&found);
    while (found.node_id != OutOfBand_ID_MAX)
    {
        CommonUtil::dump_adjlist(found);
        found.clear();
        out_cursor->next(&found);
    }
    delete out_cursor;

    out_cursor = (AdjOutCursor *)graph.get_outnbd_iter();
    // out_cursor->setAllNodes(false);
    std::cout << "Printing in-adjlists for nodes with non-null nbd "
                 "(AllNodes=false)\n";
    out_cursor->next(&found);
    while (found.node_id != OutOfBand_ID_MAX)
    {
        CommonUtil::dump_adjlist(found);
        found.clear();
        out_cursor->next(&found);
    }
    out_cursor->close();
    delete out_cursor;
}

void test_NodeCursor(AdjList &graph)
{
    INFO();
    NodeCursor *node_cursor = graph.get_node_iter();
    node found;
    node_id_t nodeIdList[] = {
        1, 3, 5, 6, 7, 8};  // the list should be {1, 3, 4, 5, 6, 7, 8} if
                            // delete_isolated_node is not called.
    int i = 0;
    node_cursor->next(&found);
    while (found.id != OutOfBand_ID_MAX)
    {
        std::cout << "Found node " << found.id << "\tExpected node "
                  << nodeIdList[i] << std::endl;
        assert(found.id == nodeIdList[i]);
        CommonUtil::dump_node(found);
        node_cursor->next(&found);
        i++;
    }
    node_cursor->close();
    delete node_cursor;
}

void test_NodeCursor_Range(AdjList graph)
{
    INFO();
    NodeCursor *node_cursor = graph.get_node_iter();
    node found;
    node_id_t nodeIdList[] = {3, 5, 6};  // The list should be {3,5,6} if
                                         // delete_isolated_node is not called.
    int i = 0;
    node_cursor->set_key_range(key_range{3, 6});
    node_cursor->next(&found);
    while (found.id != OutOfBand_ID_MAX)
    {
        assert(found.id == nodeIdList[i]);
        CommonUtil::dump_node(found);
        node_cursor->next(&found);
        i++;
    }
    node_cursor->close();
    delete node_cursor;
}

void test_EdgeCursor(AdjList graph)
{
    INFO();
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

void test_EdgeCursor_Range(AdjList graph)
{
    INFO();
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
void tearDown(AdjList &graph) { graph.close(true); }

int main()
{
    const int THREAD_NUM = 1;
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.type = GraphType::Adj;
    opts.db_dir = "./db";
    opts.db_name = "test_adj";
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

    // GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
    //                                            .opts = opts};
    GraphEngine myEngine(THREAD_NUM, opts);
    WT_CONNECTION *conn = myEngine.get_connection();
    AdjList graph(opts, conn);
    create_init_nodes(graph, opts.is_directed);
    test_get_nodes(graph);
    test_get_node(graph);
    test_add_edge(graph, opts.is_directed);
    // /*
    //  test_add_fail should fail if create_new is false
    //  add_edge->add_to_adjlists assumes no duplicate edges.
    //  */
    test_get_edge(graph);
    test_get_out_edges(graph);
    test_get_in_edges(graph);
    test_get_out_nodes(graph);
    test_get_in_nodes(graph);
    test_get_in_degree(graph);
    test_delete_node(graph, opts.is_directed);
    test_delete_isolated_node(graph, opts.is_directed);

    test_InCursor(graph);
    test_OutCursor(graph);
    test_NodeCursor(graph);
    test_NodeCursor_Range(graph);
    test_EdgeCursor(graph);
    test_EdgeCursor_Range(graph);
    tearDown(graph);
}