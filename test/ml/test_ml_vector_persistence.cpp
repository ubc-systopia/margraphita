// ML vector persistence tests using a synthetic AdjList database.
// Checks vertex and edge vectors after closing and reopening the database,
// including overwritten vectors and unchanged graph topology.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <vector>

#include "common_defs.h"
#include "graph_engine.h"
#include "ml/ml_access.h"

using namespace ml_access;

static const std::vector<node_id_t> vertices = {1, 2, 3};
static const std::vector<EdgeId> edges = {{1, 2}, {2, 3}};
static const VectorBatch vertex_values{3, 2, {1, 2, 3, 4, 5, 6}};
static const VectorBatch edge_values{2, 2, {7, 8, 9, 10}};
static const VectorBatch updated_vertex{1, 2, {11, 12}};
static const VectorBatch updated_edge{1, 2, {13, 14}};

static void check_topology(GraphBase& g)
{
    assert(g.get_num_nodes() == vertices.size());
    assert(g.get_num_edges() == edges.size());
    for (auto id : vertices) assert(g.has_node(id));
    for (const auto& e : edges) assert(g.has_edge(e.src, e.dst));
    assert(!g.has_edge(2, 1));
}

static void test_vertex_vector_persistence(GraphBase& g)
{
    const auto batch = get_vertex_vectors(g, vertices, {2});
    assert(batch.rows == vertex_values.rows);
    assert(batch.dimension == vertex_values.dimension);
    assert(batch.values == vertex_values.values);
    fprintf(stderr, "%s: PASSED\n", __FUNCTION__);
}

static void test_edge_vector_persistence(GraphBase& g)
{
    const auto batch = get_edge_vectors(g, edges, {2});
    assert(batch.rows == edge_values.rows);
    assert(batch.dimension == edge_values.dimension);
    assert(batch.values == edge_values.values);
    fprintf(stderr, "%s: PASSED\n", __FUNCTION__);
}

static void test_vector_overwrite_persistence(GraphBase& g)
{
    const auto v = get_vertex_vectors(g, vertices, {2});
    const auto e = get_edge_vectors(g, edges, {2});
    const std::vector<float> expected_v = {1, 2, 11, 12, 5, 6};
    const std::vector<float> expected_e = {13, 14, 9, 10};
    assert(v.rows == 3 && v.dimension == 2 && v.values == expected_v);
    assert(e.rows == 2 && e.dimension == 2 && e.values == expected_e);
    check_topology(g);
    fprintf(stderr, "%s: PASSED\n", __FUNCTION__);
}

int main()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = false;
    opts.has_node_props = true;
    opts.has_edge_props = true;
    opts.type = GraphType::Adj;
    opts.prop_mode = EMBEDDED;
    opts.db_dir = "./db_test/db_ml_vector_persistence";
    opts.db_name = "test_ml_vector_persistence";
    opts.conn_config = "cache_size=256MB";
    opts.stat_log = "./";

    // Create and write vectors.
    {
        GraphEngine engine(1, opts);
        GraphBase* g = engine.create_graph_handle();
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
        set_vertex_vectors(*g, vertices, vertex_values);
        set_edge_vectors(*g, edges, edge_values);
        check_topology(*g);
        g->close(true);   // sync metadata so node/edge counts persist across reopen
        engine.close_graph();
    }

    // Reopen, verify, and overwrite selected vectors.
    opts.create_new = false;
    {
        GraphEngine engine(1, opts);
        GraphBase* g = engine.create_graph_handle();
        test_vertex_vector_persistence(*g);
        test_edge_vector_persistence(*g);
        check_topology(*g);
        set_vertex_vectors(*g, {2}, updated_vertex);
        set_edge_vectors(*g, {{1, 2}}, updated_edge);
        g->close(true);   // sync metadata so node/edge counts persist across reopen
        engine.close_graph();
    }

    // Reopen again to verify the updates persisted.
    {
        GraphEngine engine(1, opts);
        GraphBase* g = engine.create_graph_handle();
        test_vector_overwrite_persistence(*g);
        g->close(true);   // sync metadata so node/edge counts persist across reopen
        engine.close_graph();
    }

    fprintf(stderr, "\nAll ML vector persistence tests passed.\n");
    return 0;
}
