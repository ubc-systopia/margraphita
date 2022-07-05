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
    opts.create_new = false;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_name = "ekey_rd_s10_e8";
    opts.db_dir = "/home/puneet/scratch/margraphita/db/s10_e8";
    opts.conn_config = "cache_size=10GB";
    opts.stat_log = "/home/puneet/scratch/margraphita/profile/test";
    opts.type = GraphType::Std;

    // Test graph setup
    EdgeKey *graph = new EdgeKey(opts);
    std::vector<key_range> node_offsets;
    std::vector<edge_range> edge_offsets;
    WT_CURSOR *ecur = graph->get_edge_cursor();
    calculate_thread_offsets(
        10, 989, 8192, node_offsets, edge_offsets, opts.type, ecur);

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
    graph->close();
    return 0;
}
