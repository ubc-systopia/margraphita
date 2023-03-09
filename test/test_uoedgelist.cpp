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

    // Test std_graph teardown
    tearDown(graph);

    return 0;
}