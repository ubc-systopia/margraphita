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

int runTest()
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
    bool results[8];
#pragma omp parallel for
    for (int i = 0; i < THREAD_NUM; i++)
    {
        GraphBase* graph = myEngine.create_graph_handle();
        if (i == 0)
        {
            node node1 = {.id = 1};
            results[0] = graph->has_node(1);
            results[1] = graph->has_node(2);
            graph->add_node(node1);
            results[2] = graph->has_node(1);
            results[3] = graph->has_node(2);
        }
        else if (i == 1)
        {
            node node2 = {.id = 2};
            results[4] = graph->has_node(1);
            results[5] = graph->has_node(2);
            graph->add_node(node2);
            results[6] = graph->has_node(1);
            results[7] = graph->has_node(2);
        }
        else
        {
            // pass
        }
    }
    myEngine.close_graph();
    return bitToInt(results);
}

int main()
{
    int toCSV[256];

    for (int j = 0; j < 256; j++)
    {
        toCSV[j] = 0;
    }

    for (int i = 0; i < TEST_NUM; i++)
    {
        int retVal = runTest();
        toCSV[retVal] += 1;
    }

    for (int j = 0; j < 256; j++)
    {
        if (toCSV[j] != 0)
        {
            std::bitset<8> binString(j);
            cout << binString << ": " << toCSV[j] << "\n";
        }
    }
}