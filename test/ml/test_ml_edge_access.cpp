// ML edge access correctness tests for AdjList. Creates independent synthetic
// EMBEDDED float32 edge blobs and COLUMNAR SNB Knows attributes. Checks all
// retrieval strategies, directed edge identity, ordering, duplicates, empty
// batches and invalid requests. No external dataset required.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
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

static void add_edge(GraphBase &g, node_id_t src, node_id_t dst)
{
    edge e{}; e.src_id = src; e.dst_id = dst; g.add_edge(e, false);
}

void test_edge_vectors(GraphBase &g)
{
    INFO();
    for (node_id_t id : {1, 2, 3}) add_node(g, id);
    const std::vector<EdgeId> edges = {{1,2}, {1,3}, {2,1}};
    const std::vector<std::vector<float>> features = {{1,2,3}, {4,5,6}, {7,8,9}};
    for (std::size_t i=0; i<edges.size(); ++i) {
        auto e = edges[i]; add_edge(g, e.src, e.dst);
        g.set_edge_properties(e.src, e.dst,
            reinterpret_cast<const uint8_t*>(features[i].data()), 3*sizeof(float));
    }
    const std::vector<EdgeId> ids = {{2,1},{1,3},{1,2},{2,1}};
    const std::vector<float> expected = {7,8,9,4,5,6,1,2,3,7,8,9};
    for (auto strategy : strategies) {
        auto b = get_edge_vectors(g, ids, {3,strategy});
        assert(b.rows == ids.size() && b.dimension == 3 && b.values == expected);
    }
    PASS();
}

void test_empty_edges(GraphBase &g)
{
    INFO();
    for (auto strategy : strategies) {
        auto b = get_edge_vectors(g, {}, {3,strategy});
        assert(b.rows == 0 && b.dimension == 3 && b.values.empty());
    }
    PASS();
}

template<class Exception, class F> void expect_error(F f)
{
    bool rejected = false;
    try { f(); } catch (const Exception&) { rejected = true; }
    assert(rejected);
}

void test_invalid_edges(GraphBase &g)
{
    INFO();
    for (auto strategy : strategies) {
        expect_error<std::invalid_argument>([&]{ get_edge_vectors(g, {{1,2}}, {0,strategy}); });
        expect_error<std::runtime_error>([&]{ get_edge_vectors(g, {{1,2}}, {4,strategy}); });
        expect_error<std::runtime_error>([&]{ get_edge_vectors(g, {{3,1}}, {3,strategy}); });
    }
    PASS();
}

void test_edge_attributes(GraphBase &g)
{
    INFO();
    const auto a = MAKE_TYPED_ID(VT_PERSON, 1);
    const auto b = MAKE_TYPED_ID(VT_PERSON, 2);
    const auto c = MAKE_TYPED_ID(VT_PERSON, 3);
    add_node(g,a); add_node(g,b); add_node(g,c);
    add_edge(g,a,b); add_edge(g,a,c);
    uint8_t buf[SNBKnowsSchema::TOTAL_SIZE] = {};
    SNBKnowsSchema::set_creation_date(buf,1000);
    g.set_edge_properties(a,b,buf,sizeof(buf));
    SNBKnowsSchema::set_creation_date(buf,2000);
    g.set_edge_properties(a,c,buf,sizeof(buf));
    const std::vector<EdgeId> ids = {{a,c},{a,b},{a,c}};
    const std::vector<AttributeValue> expected = {uint64_t(2000),uint64_t(1000),uint64_t(2000)};
    for (auto strategy : strategies) {
        AttributeRequest req{KNOWS_PROPS_TABLE,CG_TEMPORAL,{{0,AttributeType::UInt64}},strategy};
        auto batch = get_edge_attributes(g,ids,req);
        assert(batch.rows == ids.size() && batch.columns == 1 && batch.values == expected);
        auto empty = get_edge_attributes(g,{},req);
        assert(empty.rows == 0 && empty.columns == 1 && empty.values.empty());
        expect_error<std::runtime_error>([&]{ get_edge_attributes(g,{{b,c}},req); });
        req.columns[0].type = AttributeType::Int32;
        expect_error<std::invalid_argument>([&]{ get_edge_attributes(g,ids,req); });
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
    opts.db_dir = "./db_test/db_ml_edge_access";
    opts.conn_config = "cache_size=256MB";
    opts.stat_log = "./";

    {
        opts.prop_mode = EMBEDDED;
        opts.db_name = "test_ml_edge_vectors";
        GraphEngine engine(1, opts);
        GraphBase *g = engine.create_graph_handle();
        test_edge_vectors(*g);
        test_empty_edges(*g);
        test_invalid_edges(*g);
        g->close(false);
        engine.close_graph();
    }
    {
        opts.prop_mode = COLUMNAR;
        opts.db_name = "test_ml_edge_attributes";
        GraphEngine engine(1, opts);
        GraphBase *g = engine.create_graph_handle();
        test_edge_attributes(*g);
        g->close(false);
        engine.close_graph();
    }
    fprintf(stderr, "\nAll ML Edge Access tests passed.\n");
    return 0;
}
