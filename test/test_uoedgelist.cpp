#include "test_uoedgelist.h"

#include "common.h"
#include "edgelist.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define delim "--------------"
#define INFO() fprintf(stderr, "%s\nNow running: %s\n", delim, __FUNCTION__);

void create_init_nodes(UnOrderedEdgeList graph, bool is_directed)
{
    INFO()
    int node_count = 0;
    for (node x : SampleGraph::test_nodes)
    {
        assert(graph.add_node(x) == 0);
        node_count++;
    }
    assert(node_count == 6);

    if (!is_directed)
    {
        SampleGraph::create_undirected_edges();
        std::cout << SampleGraph::test_edges.size() << "\n";
        assert(SampleGraph::test_edges.size() ==
               10);  // checking if directed edges got created and stored in
                     // test_edges
    }

    std::cout << "Insert Edges Now\n";
    int edge_cnt = 0;
    for (edge x : SampleGraph::test_edges)
    {
        std::cout << "Edge (" << x.src_id << " , " << x.dst_id << ")\n";
        graph.add_edge(x, false);
        edge_cnt++;
    }
    assert(edge_cnt == SampleGraph::test_edges.size());
}

void test_get_num_nodes_and_edges(UnOrderedEdgeList graph, bool is_directed)
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

void test_get_node(UnOrderedEdgeList graph)
{
    INFO()
    node found = graph.get_node(1);
    assert(found.id == 1);
}

void test_get_in_degree(UnOrderedEdgeList graph, bool is_directed)
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

void test_get_out_degree(UnOrderedEdgeList graph, bool is_directed)
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

void test_get_nodes(UnOrderedEdgeList graph)
{
    INFO()
    for (node x : graph.get_nodes())
    {
        CommonUtil::dump_node(x);
    }
}

void test_has_edge(UnOrderedEdgeList graph)
{
    INFO()
    assert(graph.has_edge(1, 2));
    assert(graph.has_edge(1, 3));
    assert(graph.has_edge(2, 3));
    assert(graph.has_edge(1, 7));
    assert(graph.has_edge(7, 8));
    assert(graph.has_edge(8, 7));
}

void test_get_edge(UnOrderedEdgeList graph, bool is_directed)
{
    INFO()
    edge found =
        graph.get_edge(SampleGraph::edge1.src_id, SampleGraph::edge1.dst_id);
    assert(found.src_id == SampleGraph::edge1.src_id);
    assert(found.dst_id == SampleGraph::edge1.dst_id);
}

void test_node_add(UnOrderedEdgeList graph, bool read_optimize)
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

void test_add_edge(UnOrderedEdgeList graph, bool is_directed, bool is_weighted)
{
    INFO()
    edge to_insert = {
        .id = 7,
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

void test_get_edges(UnOrderedEdgeList graph, bool is_directed)
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
    assert(found.src_id == -1);
    assert(found.dst_id == -1);
    assert(found.edge_weight == -1);
}

void tearDown(UnOrderedEdgeList graph) { graph.close(); }

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
    opts.type = GraphType::EList;
    opts.db_name = "test_uoelist";
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
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};
    GraphEngineTest myEngine(engine_opts);
    WT_CONNECTION *conn = myEngine.public_get_connection();

    UnOrderedEdgeList graph(opts, conn);
    create_init_nodes(graph, opts.is_directed);
    // Test num_get_nodes and num_get_edges
    test_get_num_nodes_and_edges(graph, opts.is_directed);
    // Test get_node()
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

    // test has_edge()
    test_has_edge(graph);
    // test_get_edge(graph, opts.is_directed);

    // Test std_graph teardown
    tearDown(graph);

    return 0;
}