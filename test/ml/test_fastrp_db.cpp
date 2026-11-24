#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <sys/resource.h>

#include "common_defs.h"
#include "graph_engine.h"
#include "command_line.h"
#include "ml/ml_access.h"
#include "ml/algorithms/fastrp.h"

using namespace ml;
using namespace ml_access;

using Clock = std::chrono::steady_clock;

static double elapsed(Clock::time_point start)
{
    return std::chrono::duration<double>(
        Clock::now() - start
    ).count();
}

// Bulk-loaded databases may not have a node_props table.
static void prepare_storage(GraphEngine& engine)
{
    WT_CONNECTION* conn = engine.get_connection();
    WT_SESSION* session = nullptr;

    int ret = conn->open_session(
        conn, nullptr, nullptr, &session
    );

    assert(ret == 0);

    const std::string table =
        std::string("table:") + AdjList::NODE_PROPS_TABLE;

    ret = session->create(
        session,
        table.c_str(),
        "key_format=u,value_format=u,leaf_page_max=64KB"
    );

    session->close(session, nullptr);

    assert(ret == 0);
}

int main(int argc, char** argv)
{
    CmdLineApp cli(argc, argv);

    if (!cli.parse_args())
        return 1;

    auto opts = cli.get_parsed_opts();

    opts.create_new = false;
    opts.read_only = false;
    opts.has_node_props = true;
    opts.prop_mode = EMBEDDED;

    assert(opts.type == GraphType::Adj);

    FastRPOptions config;
    config.dim = 128;
    config.iters = 4;
    config.threads = 4;
    config.seed = 42;
    config.density = 0.01;
    config.cursor_read = true;
    config.csr = false;
    config.write_db = "after";

    FastRPResult result;

    node_id_t original_nodes = 0;
    edge_id_t original_edges = 0;

    double generation_write_time = 0;
    double retrieval_time = 0;

    const auto total_start = Clock::now();

    // --------------------------------------------------
    // 1. Open existing database and generate embeddings.
    // --------------------------------------------------

    {
        GraphEngine engine(config.threads + 1, opts);

        prepare_storage(engine);

        GraphBase* g = engine.create_graph_handle();

        original_nodes = g->get_num_nodes();
        original_edges = g->get_num_edges();

        assert(original_nodes > 0);

        fprintf(stderr, "\nDatabase: %s\n",
                opts.db_name.c_str());

        fprintf(stderr, "Vertices: %llu\n",
                static_cast<unsigned long long>(original_nodes));

        fprintf(stderr, "Edges: %llu\n",
                static_cast<unsigned long long>(original_edges));

        fprintf(stderr, "Open existing database: PASSED\n");

        std::vector<GraphBase*> workers;

        for (int i = 0; i < config.threads; ++i)
            workers.push_back(engine.create_graph_handle());

        const auto start = Clock::now();

        result = fastrp(
            *g,
            workers,
            config,
            opts.is_directed
        );

        generation_write_time = elapsed(start);

        // Verify embedding dimensions.
        assert(result.ids.size() == original_nodes);
        assert(result.embeddings.rows == original_nodes);

        assert(
            result.embeddings.dimension ==
            static_cast<size_t>(config.dim)
        );

        assert(
            result.embeddings.values.size() ==
            result.ids.size() * config.dim
        );

        // Verify that all embedding values are finite.
        for (float value : result.embeddings.values)
            assert(std::isfinite(value));

        fprintf(stderr, "FastRP generation: PASSED\n");
        fprintf(stderr, "Embedding dimensions: PASSED\n");
        fprintf(stderr, "Finite embedding values: PASSED\n");

        for (auto* worker : workers)
            worker->close(false);

        g->close(false);
        engine.close_graph();

        fprintf(stderr, "Database closed after writes.\n");
    }

    // --------------------------------------------------
    // 2. Reopen database and retrieve stored embeddings.
    // --------------------------------------------------

    opts.read_only = true;

    {
        GraphEngine engine(1, opts);
        GraphBase* reader = engine.create_graph_handle();

        fprintf(stderr, "Reopen database: PASSED\n");

        const auto start = Clock::now();

        const auto stored = get_vertex_vectors(
            *reader,
            result.ids,
            {static_cast<size_t>(config.dim)}
        );

        retrieval_time = elapsed(start);

        assert(stored.rows == result.embeddings.rows);

        assert(
            stored.dimension ==
            result.embeddings.dimension
        );

        assert(
            stored.values ==
            result.embeddings.values
        );

        fprintf(stderr, "Embedding retrieval: PASSED\n");
        fprintf(stderr, "Embedding persistence: PASSED\n");
        fprintf(stderr, "Embedding equality: PASSED\n");

        // Verify graph topology counts remain unchanged.
        assert(reader->get_num_nodes() == original_nodes);
        assert(reader->get_num_edges() == original_edges);

        fprintf(stderr, "Topology counts unchanged: PASSED\n");

        reader->close(false);
        engine.close_graph();
    }

    // --------------------------------------------------
    // 3. Report results.
    // --------------------------------------------------

    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);

    const double embedding_mb =
        result.embeddings.values.size() *
        sizeof(float) / (1024.0 * 1024.0);

    fprintf(stderr, "\n--- FastRP database test ---\n");

    fprintf(stderr, "Database: %s\n",
            opts.db_name.c_str());

    fprintf(stderr, "Embedding rows: %zu\n",
            result.embeddings.rows);

    fprintf(stderr, "Embedding dimensions: %zu\n",
            result.embeddings.dimension);

    fprintf(stderr, "Embedding size: %.3f MiB\n",
            embedding_mb);

    fprintf(stderr, "Generation + write: %.6f s\n",
            generation_write_time);

    fprintf(stderr, "Retrieval: %.6f s\n",
            retrieval_time);

    fprintf(stderr, "Total time: %.6f s\n",
            elapsed(total_start));

    fprintf(stderr, "Peak RSS: %.2f MiB\n",
            usage.ru_maxrss / 1024.0);

    fprintf(stderr, "\nAll FastRP database tests passed.\n");

    return 0;
}