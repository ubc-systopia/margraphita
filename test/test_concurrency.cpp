#include <bitset>
#include <cassert>
#include <thread>
#include <vector>

#include "common.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define THREAD_NUM 2
#define TEST_NUM 1000
using namespace std;

int bitToInt(bool* array)
{
    int sum = 0;
    for (int i = 0; i < 8; i++)
    {
        int toAdd = (int)array[i];
        int toShift = 7 - i;
        toAdd = toAdd << toShift;
        sum += toAdd;
    }
    return sum;
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

    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};
    GraphEngine myEngine(engine_opts);
    bool results[8];
#pragma omp parallel for
    for (int i = 0; i < THREAD_NUM; i++)
    {
        GraphBase* graph = myEngine.create_graph_handle();
        if (i == 0)
        {
            WT_SESSION* session1 = graph->get_session_for_testing();
            session1->begin_transaction(session1, NULL);
            graph->add_to_nnodes(2);
            cout << "adding thread 0 \n";
            session1->commit_transaction(session1, NULL);
        }
        else if (i == 1)
        {
            WT_SESSION* session2 = graph->get_session_for_testing();
            session2->begin_transaction(session2, NULL);
            graph->add_to_nnodes(2);
            cout << "adding thread 1 \n";
            session2->commit_transaction(session2, NULL);
        }
        else
        {
            // pass
        }
    }
    GraphBase* graph = myEngine.create_graph_handle();
    myEngine.close_graph();
}

int main()
{
    for (int i = 0; i < TEST_NUM; i++)
    {
        if (i % 100 == 0)
        {
            cout << i << "/" << TEST_NUM << "\n";
        }
        runTest();
    }
}