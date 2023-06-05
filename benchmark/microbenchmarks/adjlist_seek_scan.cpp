#include <times.h>
#include <deque>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>
#include "adj_list.h"
#include "common.h"
#include "graph_engine.h"

class GraphEngineTest : public GraphEngine
{
   public:
    explicit GraphEngineTest(graph_engine_opts engine_opts) : GraphEngine(std::move(engine_opts)) {}
    WT_CONNECTION *public_get_connection() { return get_connection(); }
};

bool has_node(int vertex, AdjList &graph) { return graph.has_node(vertex); }

degree_t get_vertex_degree(int vertex, AdjList &graph)
{
    return graph.get_out_degree(vertex);
}

node get_random_node(AdjList &graph) { return graph.get_random_node(); }

void print_nodes(AdjList &graph)
{
    std::vector<node> nodes = graph.get_nodes();
    for (node n : nodes)
    {
        std::cout << n.id << "\t" << n.out_degree << std::endl;
    }
}

std::vector<long double> seek_and_scan(int vertex,
                                       AdjList &graph,
                                       WT_SESSION *session)
{
    std::vector<long double> results;
    WT_CURSOR *adj_cursor = graph.get_out_adjlist_cursor();

    // warm the WT_Cache
    while (adj_cursor->next(adj_cursor) == 0)
    {
        int key;
        CommonUtil::get_key(adj_cursor, &key);
    }

    // seek
    Times timer;
    timer.start();
    CommonUtil::set_key(adj_cursor, vertex);
    int ret = adj_cursor->search(adj_cursor);
    if (ret != 0)
    {
        std::cout << "Vertex " << vertex << " not found" << std::endl;
        exit(1);
    }
    timer.stop();
    results.push_back(timer.t_nanos());

    // scan
    adjlist adj_list;
    timer.start();
    CommonUtil::record_to_adjlist(session, adj_cursor, &adj_list);
    for ([[maybe_unused]] int node : adj_list.edgelist)
    {
        // do nothing
    }
    timer.stop();
    results.push_back(timer.t_nanos());
    return results;
}

void test_get_nodes(AdjList graph)
{
    std::cout << "Testing get_nodes" << std::endl;
    WT_CURSOR *node_cursor = graph.get_node_cursor();
    node found;
    uint32_t a = __builtin_bswap32(0);
    WT_ITEM k1 = {.data = &a, .size = sizeof(a)};
    node_cursor->set_key(node_cursor, k1);
    int ret = node_cursor->search(node_cursor);
    assert(ret == 0);
    found.in_degree = 0;
    found.out_degree = 0;
    node_cursor->get_value(node_cursor, &found.in_degree, &found.out_degree);

    WT_ITEM k2 = {0};
    ret = node_cursor->get_key(node_cursor, &k2);
    assert(ret == 0);
    uint32_t b = *(uint32_t *)k2.data;
    found.id = __builtin_bswap32(b);

    CommonUtil::dump_node(found);
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        std::cout << "Usage: ./ubenchmark <num_vertices> <num_edges> "
                     "<graphfile> <wt_db_dir> <wt_db_name>"
                  << std::endl;
        return 0;
    }
    int num_vertices = atoi(argv[1]);
    int num_edges = atoi(argv[2]);
    std::filesystem::path graphfile(argv[3]);
    string wt_db_dir(argv[4]);
    std::string wt_db_name = argv[5];
    [[maybe_unused]] int num_random_samples = 1000;

    const int THREAD_NUM = 1;
    graph_opts opts;
    opts.create_new = false;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.type = GraphType::Adj;
    opts.db_dir = wt_db_dir;
    opts.db_name = wt_db_name;
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

    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};
    GraphEngineTest myEngine(engine_opts);
    WT_CONNECTION *conn = myEngine.public_get_connection();
    AdjList graph(opts, conn);
    graph.init_cursors();
    test_get_nodes(graph);

    // profile_wt_adjlist(
    //     num_vertices, num_edges, graphfile, num_random_samples, graph);
}