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
    //    if (!is_directed)
    //    {
    //        SampleGraph::create_undirected_edges();
    //        assert(SampleGraph::test_edges.size() ==
    //               6);  // checking if directed edges got created and stored
    //               in
    //                    // test_edges
    //    }
    //    int edge_cnt = 1;
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

    return 0;
}