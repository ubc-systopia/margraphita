#include <cassert>
#include <thread>
#include <vector>

#include "common_util.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define THREAD_NUM 2
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
    if (const char* env_p = std::getenv("GRAPH_PROJECT_DIR"))
    {
        opts.stat_log = std::string(env_p);
    }
    else
    {
        std::cout << "GRAPH_PROJECT_DIR not set. Using CWD" << std::endl;
        opts.stat_log = "./";
    }

    GraphEngine myEngine(THREAD_NUM, opts);

    GraphBase* graph = myEngine.create_graph_handle();
    graph->add_node(node{.id = 0}, false);
    graph->add_node(node{.id = 100}, false);
    graph->close(true);
#pragma omp parallel for
    for (int i = 0; i < THREAD_NUM; i++)
    {
        GraphBase* graph = myEngine.create_graph_handle();

        if (i % 2 == 0)
        {
            int ret = -1;
            while (ret != 0)
            {
                ret = graph->add_node(node{.id = 1}, false);
            }
            ret = -1;
            while (ret != 0)
            {
                ret =
                    graph->add_edge(edge{static_cast<edge_id_t>(i), 1}, false);
            }
        }
        else
        {
            int ret = -1;
            while (ret != 0)
            {
                ret = graph->delete_node(1);
            }
            ret = -1;
            while (ret != 0) ret = graph->add_edge(edge{5, 8}, false);
        }
        graph->close(true);
    }

    GraphBase* report = myEngine.create_graph_handle();
    cout << "No. of nodes: " << report->get_num_nodes() << '\n';
    cout << "No. of edges: " << report->get_num_edges() << '\n';
    // cout << "No. of outdeg from 1: " << report->get_out_degree(1) << '\n';
    std::vector<node> nodes = report->get_nodes();
    cout << "NODES: \n";
    for (auto i : nodes)
    {
        CommonUtil::dump_node(i);
    }
    std::vector<edge> edges = report->get_edges();
    cout << "EDGES: \n";
    for (auto j : edges)
    {
        CommonUtil::dump_edge(j);
    }
    myEngine.close_graph();
}