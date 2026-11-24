#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdio>
#include <vector>

#include "common_defs.h"
#include "graph_engine.h"
#include "ml/ml_access.h"
#include "ml/algorithms/fastrp.h"

using namespace ml;
using namespace ml_access;

int main()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = false;
    opts.has_node_props = true;
    opts.has_edge_props = false;
    opts.type = GraphType::Adj;
    opts.prop_mode = EMBEDDED;
    opts.db_dir = "./db_test/db_fastrp";
    opts.db_name = "test_fastrp";
    opts.conn_config = "cache_size=256MB";
    opts.stat_log = "./";

    GraphEngine engine(2, opts);
    GraphBase* g = engine.create_graph_handle();

    const std::vector<node_id_t> vertices = {1, 2, 3};
    const std::vector<EdgeId> edges = {
        {1, 2}, {2, 3}, {3, 1}
    };

    // Create a small graph.
    for (auto id : vertices) {
        node n{};
        n.id = id;
        g->add_node(n, false);
    }

    for (const auto& id : edges) {
        edge e{};
        e.src_id = id.src;
        e.dst_id = id.dst;
        g->add_edge(e, false);
    }

    GraphBase* worker = engine.create_graph_handle();
    std::vector<GraphBase*> workers = {worker};

    // Generate and store FastRP embeddings.
    FastRPOptions config;
    config.dim = 16;
    config.iters = 4;
    config.threads = 1;
    config.write_db = "after";

    auto result = fastrp(*g, workers, config, true);

    // Verify embeddings were generated.
    assert(result.ids == vertices);
    assert(result.embeddings.rows == 3);
    assert(result.embeddings.dimension == 16);
    assert(result.embeddings.values.size() == 48);

    fprintf(stderr, "FastRP generation: PASSED\n");

    // Verify embeddings were added to the database. Read through a handle opened
    // after fastrp(): the main handle g can keep reading from the snapshot it
    // took during fastrp() and miss the worker's writes.
    GraphBase* reader = engine.create_graph_handle();
    auto stored = get_vertex_vectors(*reader, vertices, {16});

    assert(stored.rows == 3);
    assert(stored.dimension == 16);
    assert(stored.values == result.embeddings.values);

    fprintf(stderr, "FastRP storage: PASSED\n");

    reader->close(false);
    worker->close(false);
    g->close(true);
    engine.close_graph();

    fprintf(stderr, "All FastRP tests passed.\n");
    return 0;
}