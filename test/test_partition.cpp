#include <cassert>

#include "common.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"
#include "standard_graph.h"
#include "test_standard_graph.h"
#include "thread_utils.h"

#define INFO() fprintf(stderr, "Now running: %s\n", __FUNCTION__);

void tearDown(GraphBase *graph) { graph->close(); }

void test_get_nodes(GraphBase *graph) { INFO(); }

int main()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_name = "ekey_rd_s10_e8";
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
    opts.type = GraphType::Std;

    const int THREAD_NUM = 8;

    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};
    GraphEngineTest myEngine(engine_opts);
    WT_CONNECTION *conn = myEngine.public_get_connection();
    EdgeKey graph(opts, conn);

    std::vector<key_range> node_offsets;
    std::vector<edge_range> edge_offsets;
    calculate_thread_offsets(
        THREAD_NUM, 989, 8192, node_offsets, edge_offsets, opts.type);

    for (auto x : node_offsets)
    {
        std::cout << "(" << x.start << "," << x.end << ")" << std::endl;
    }
    for (auto x : edge_offsets)
    {
        std::cout << "(" << x.start.src_id << "," << x.start.dst_id << ")\n";
        std::cout << "(" << x.end.src_id << "," << x.end.dst_id << ")\n";
        std::cout << "------------\n";
    }
    graph.close();
    return 0;
}
