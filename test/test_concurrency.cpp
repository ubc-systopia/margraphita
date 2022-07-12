#include <cassert>
#include <thread>
#include <vector>

#include "common.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define THREAD_NUM 8
using namespace std;

int main()
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
        GraphBase* graph = myEngine.create_graph_handle();
        if (i % 2 == 0)
        {
            graph->add_node(node{.id = i + 100});
        }
        else
        {
            graph->add_edge(edge{.src_id = 1, .dst_id = i - 1 + 100}, false);
        }
        graph->close();
    }

    GraphBase* report = myEngine.create_graph_handle();
    cout << "No. of nodes: " << report->get_num_nodes() << '\n';
    cout << "No. of edges: " << report->get_num_edges() << '\n';
    cout << "No. of outdeg from 1: " << report->get_out_degree(1) << '\n';
    myEngine.close_graph();
}