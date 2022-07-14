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
    cout << "START" << '\n';
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_dir = "./db";
    opts.db_name = "test_ekey";
    opts.type = GraphType::EKey;
    opts.conn_config = "cache_size=10GB";
    opts.stat_log = std::getenv("GRAPH_PROJECT_DIR");

    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};
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
    // std::vector<node> nodes = report->get_nodes();
    // for (auto i : nodes)
    // {
    //     CommonUtil::dump_node(i);
    // }
    // std::vector<edge> edges = report->get_edges();
    // for (auto j : edges)
    // {
    //     CommonUtil::dump_edge(j);
    // }
    myEngine.close_graph();
    cout << "END"
         << "\n \n";
}