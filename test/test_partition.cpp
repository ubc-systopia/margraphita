#include <cassert>

#include "common_util.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"
#include "thread_utils.h"

#define INFO() fprintf(stderr, "Now running: %s\n", __FUNCTION__);

void tearDown(GraphBase *graph) { graph->close(); }

void test_get_nodes(GraphBase *graph) { INFO(); }

int main()
{
    graph_opts opts;
    opts.create_new = false;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_name = "ekey_rd_cit-Patents/";
    opts.db_dir = "/home/puneet/db/cit-Patents";
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
    opts.type = GraphType::EKey;

    const int THREAD_NUM = 8;

    GraphEngine myEngine(THREAD_NUM, opts);
    myEngine.calculate_thread_offsets();
    myEngine.close_graph();
    return 0;
}
