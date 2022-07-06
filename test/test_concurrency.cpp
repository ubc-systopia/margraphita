#include <cassert>
#include <thread>
#include <vector>

#include "common.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define THREAD_NUM 2
#define TEST_NUM 100
using namespace std;

int main()
{
    for (int i = 0; i < TEST_NUM; i++)
    {
        runTest();
    }
}

void runTest()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_dir = "./db";
    opts.db_name = "test_adj";
    opts.type = GraphType::Adj;
    opts.conn_config = "cache_size=10GB";
    opts.stat_log = std::getenv("GRAPH_PROJECT_DIR");

    GraphEngine::graph_engine_opts engine_opts{.num_threads = 8, .opts = opts};
    GraphEngine myEngine(engine_opts);
#pragma omp parallel for
    for (int i = 0; i < THREAD_NUM; i++)
    {
        GraphBase* graph = myEngine.createGraphHandle();
        if (i = 1)
        {
            // node 1 exists?
            // node 2 exists?
            // insert node 1
            // node 1 exists?
            // node 2 exists?
        }
        else if (i = 2)
        {
            // node 1 exists?
            // node 2 exists?
            // insert node 2
            // node 1 exists?
            // node 2 exists?
        }
        else
        {
            // pass
        }
    }
    myEngine.close_graph();
}