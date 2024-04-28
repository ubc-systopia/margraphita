#include <cassert>

#include "common_util.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define delim "--------------"
#define INFO() fprintf(stderr, "%s\nNow running: %s\n", delim, __FUNCTION__);

void create_init_nodes(SplitEdgeKey &graph, bool is_directed)
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
        //        edge_cnt++;
    }
}

void test_get_node(SplitEdgeKey &graph)
{
    INFO()
    node found = graph.get_node(SampleGraph::node1.id);
    assert(found.id == 1);
    // now get a node that does not exist
    found = graph.get_node(-1);
    assert(found.id == 0);
    found = {0};
    found = graph.get_random_node();
    CommonUtil::dump_node(found);
}

void test_node_add(SplitEdgeKey &graph, bool read_optimize)
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

void test_get_nodes(SplitEdgeKey &graph)
{
    INFO()
    for (node x : graph.get_nodes())
    {
        CommonUtil::dump_node(x);
    }
}

void test_get_random_nodes(SplitEdgeKey &graph)
{
    INFO()
    fprintf(stderr, "Random node:\n");
    CommonUtil::dump_node(graph.get_random_node());
    CommonUtil::dump_node(graph.get_random_node());
}

void test_add_edge(SplitEdgeKey &graph, bool is_directed, bool is_weighted)
{
    INFO()
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
    if (is_weighted) assert(found.edge_weight == 333);
    if (!is_directed)
    {
        found = graph.get_edge(6, 5);
        assert(found.src_id == 6);
        assert(found.src_id == 5);
        if (is_weighted) assert(found.edge_weight == 333);
    }

    // Check if the nodes were created.
    node got = graph.get_node(5);
    assert(got.id == 5);
    got = graph.get_node(6);
    assert(got.id == 6);

    // //Now check if the adjlists were updated
    WT_CURSOR *e_cur = graph.get_out_edge_cursor();
    CommonUtil::ekey_set_key(e_cur, 5, OutOfBand_ID);
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

int main()
{
    const int THREAD_NUM = 1;
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.type = GraphType::SplitEKey;
    opts.db_name = "test_split_edgekey";
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

    GraphEngine myEngine(THREAD_NUM, opts);
    WT_CONNECTION *conn = myEngine.get_connection();
    SplitEdgeKey graph(opts, conn);

    create_init_nodes(graph, opts.is_directed);
    test_get_node(graph);
    test_node_add(graph, opts.read_optimize);
    test_get_nodes(graph);
    test_get_random_nodes(graph);
    test_add_edge(graph, opts.is_directed, opts.is_weighted);
    //    test_get_edge(graph);
    //    test_get_edges(graph);
    return 0;
}