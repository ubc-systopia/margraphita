#include <cassert>

#include "common.h"
#include "graph_exception.h"
#include "sample_graph.h"
#include "standard_graph.h"

WT_CONNECTION *conn;
WT_CURSOR *cursor;
WT_SESSION *session;
const char *home;

class StandardGraphTester : public StandardGraph
{
   public:
    StandardGraphTester(graph_opts opts) : StandardGraph(opts) {}
};

void multithreaded_add_node(Graph graph, int range_start, int range_end)
{
    int i = range_start;
    while (int i < range_end)
    {
        node toInsert = {.id = i, .in_degree = 0, .out_degree = 0};
        graph.add_node(toInsert);
    }
}

int main()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    // opts.is_directed = false;
    // opts.read_optimize = false;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_name = "test_std";
    opts.db_dir = "./db";
    opts.conn_config = "cache_size=10GB";
    opts.stat_log = std::getenv("GRAPH_PROJECT_DIR");

    StandardGraphTester graph = StandardGraphTester(opts);

    return 0;
}