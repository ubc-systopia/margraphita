// ML Access correctness tests for AdjList.
// Creates two small test databases with known vertex properties:
//   1. EMBEDDED: raw Float32 vectors stored as property blobs.
//   2. COLUMNAR: typed SNB Post attributes accessed through property cursors.
// Tests Direct, Sorted, and SortedDeduplicated batch retrieval, verifying
// feature values, output dimensions, original request order, and duplicates.
// Also checks empty batches, invalid vector dimensions, and missing vertices
// in the dense-vector path. No existing dataset or embeddings are required.
// These are correctness tests, not performance benchmarks.
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "common_defs.h"
#include "graph_engine.h"
#include "prop_schema.h"
#include "ml/ml_access.h"

// Keep assertions active even in Release builds.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#define PASS() fprintf(stderr, "%s: PASSED\n", __FUNCTION__)
#define INFO() fprintf(stderr, "--- %s\n", __FUNCTION__)

using namespace ml_access;

static const std::vector<RetrievalStrategy> strategies = {
    RetrievalStrategy::Direct,
    RetrievalStrategy::Sorted,
    RetrievalStrategy::SortedDeduplicated
};

static void add_node(GraphBase &g, node_id_t id)
{
    node n{};
    n.id = id;
    g.add_node(n, false);
}

static void add_vector(GraphBase &g, node_id_t id,
                       const std::vector<float> &values)
{
    add_node(g, id);
    g.set_node_properties(id,
        reinterpret_cast<const uint8_t *>(values.data()),
        values.size() * sizeof(float));
}

void test_vertex_vectors(GraphBase &g)
{
    INFO();
    add_vector(g, 1, {1, 2, 3});
    add_vector(g, 2, {4, 5, 6});
    add_vector(g, 3, {7, 8, 9});

    const std::vector<node_id_t> ids = {3, 1, 2, 1, 3};
    const std::vector<float> expected = {7, 8, 9, 1, 2, 3,
                                        4, 5, 6, 1, 2, 3, 7, 8, 9};
    for (auto strategy : strategies)
    {
        auto batch = get_vertex_vectors(g, ids, {3, strategy});
        assert(batch.rows == ids.size());
        assert(batch.dimension == 3);
        assert(batch.values == expected);
    }
    PASS();
}

void test_empty_batch(GraphBase &g)
{
    INFO();
    for (auto strategy : strategies)
    {
        auto batch = get_vertex_vectors(g, {}, {3, strategy});
        assert(batch.rows == 0);
        assert(batch.dimension == 3);
        assert(batch.values.empty());
    }
    PASS();
}

void test_invalid_vectors(GraphBase &g)
{
    INFO();
    for (auto strategy : strategies)
    {
        bool rejected = false;
        try { get_vertex_vectors(g, {1}, {0, strategy}); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);

        rejected = false;
        try { get_vertex_vectors(g, {1}, {4, strategy}); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected);

        rejected = false;
        try { get_vertex_vectors(g, {1, 99}, {3, strategy}); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected);
    }
    PASS();
}

void test_vertex_attributes(GraphBase &g)
{
    INFO();
    const node_id_t p1 = MAKE_TYPED_ID(VT_POST, 1);
    const node_id_t p2 = MAKE_TYPED_ID(VT_POST, 2);
    add_node(g, p1);
    add_node(g, p2);
    std::vector<uint8_t> buf(SNBPostSchema::TOTAL_SIZE + 5, 0);
    SNBPostSchema::set_creation_date(buf.data(), 1000);
    SNBPostSchema::set_length(buf.data(), 3);
    SNBPostSchema::set_tag(buf.data(), 0);
    SNBPostSchema::set_content(buf.data(), "abc");
    g.set_node_properties(p1, buf.data(), buf.size());
    SNBPostSchema::set_creation_date(buf.data(), 2000);
    SNBPostSchema::set_length(buf.data(), 4);
    SNBPostSchema::set_content(buf.data(), "abcd");
    g.set_node_properties(p2, buf.data(), buf.size());

    const std::vector<node_id_t> ids = {p2, p1, p2};
    const std::vector<AttributeValue> expected = {
        uint64_t(2000), int32_t(4), uint64_t(1000), int32_t(3),
        uint64_t(2000), int32_t(4)
    };
    for (auto strategy : strategies)
    {
        // Cursor indices are per type, not absolute schema positions.
        AttributeRequest req{POST_PROPS_TABLE, CG_TEMPORAL,
            {{0, AttributeType::UInt64}, {0, AttributeType::Int32}}, strategy};
        auto batch = get_vertex_attributes(g, ids, req);
        assert(batch.rows == ids.size());
        assert(batch.columns == 2);
        assert(batch.values == expected);
        auto empty = get_vertex_attributes(g, {}, req);
        assert(empty.rows == 0 && empty.columns == 2 && empty.values.empty());
    }
    PASS();
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
    opts.db_dir = "./db_test/db_ml_access";
    opts.conn_config = "cache_size=256MB";
    opts.stat_log = "./";

    {
        opts.prop_mode = EMBEDDED;
        opts.db_name = "test_ml_vectors";
        GraphEngine engine(1, opts);
        GraphBase *g = engine.create_graph_handle();
        test_vertex_vectors(*g);
        test_empty_batch(*g);
        test_invalid_vectors(*g);
        g->close(false);
        engine.close_graph();
    }
    {
        opts.prop_mode = COLUMNAR;
        opts.db_name = "test_ml_attributes";
        GraphEngine engine(1, opts);
        GraphBase *g = engine.create_graph_handle();
        test_vertex_attributes(*g);
        g->close(false);
        engine.close_graph();
    }
    fprintf(stderr, "\nAll ML Access tests passed.\n");
    return 0;
}
