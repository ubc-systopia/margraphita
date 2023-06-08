#include <times.h>

#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "adj_list.h"
#include "common.h"
#include "graph_engine.h"

class GraphEngineTest : public GraphEngine
{
   public:
    explicit GraphEngineTest(graph_engine_opts engine_opts)
        : GraphEngine(std::move(engine_opts))
    {
    }
    WT_CONNECTION *public_get_connection() { return get_connection(); }
};

struct time_result
{
    long double time_seek;
    long double time_scan;
    degree_t degree;
};

struct time_result test_out_cursor(node_id_t vertex,
                                   WT_SESSION *session,
                                   AdjList &graph)
{
    edge found;
    int degree = 0;
    WT_CURSOR *edge_cursor = graph.get_edge_cursor();
    struct time_result results = {};
    Times timer;
    timer.start();
    CommonUtil::set_key(edge_cursor, vertex, 0);
    int status;
    edge_cursor->search_near(edge_cursor, &status);
    if (status < 0)
    {
        edge_cursor->next(edge_cursor);  // advance to the first position
    }
    timer.stop();
    results.time_seek = timer.t_nanos();

    timer.start();
    do
    {
        CommonUtil::get_key(edge_cursor, &found.src_id, &found.dst_id);
        //        std::cout << "edge: " << found.src_id << "\t" << found.dst_id
        //                  << std::endl;
        if (found.src_id == vertex)
        {
            ++results.degree;
        }
        else
        {
            break;
        }
        edge_cursor->next(edge_cursor);
    } while (found.src_id == vertex);

    timer.stop();
    assert(results.degree == graph.get_out_degree(vertex));
    results.time_scan = timer.t_nanos();
    return results;
}

void profile_wt_adjlist(const filesystem::path &graphfile,
                        WT_SESSION *session,
                        int samples,
                        AdjList &graph)
{
    std::vector<node_id_t> random_ids;
    WT_CURSOR *random_out_adj_cursor = graph.get_new_random_outadj_cursor();
    int num = 0;
    while (num < samples)
    {
        random_out_adj_cursor->next(random_out_adj_cursor);
        node rando;
        CommonUtil::get_key(random_out_adj_cursor, &rando.id);
        if (graph.get_out_degree(rando.id) == 0) continue;
        random_ids.push_back(rando.id);
        num++;
    }
    assert(random_ids.size() == 1000);
    random_out_adj_cursor->close(random_out_adj_cursor);

    // create file for iterator based seek and scan
    char outfile_iter_name[256];
    sprintf(outfile_iter_name,
            "%s_adjlist_iter_ubench.txt",
            graphfile.stem().c_str());
    std::ofstream adjlist_iter_seek_scan_outfile(outfile_iter_name);

    adjlist_iter_seek_scan_outfile
        << "vertex_id,degree,seek_time_ns,scan_time_per_edge_ns" << std::endl;

    for (auto sample : random_ids)
    {
        struct time_result time_cursor =
            test_out_cursor(sample, session, graph);
        adjlist_iter_seek_scan_outfile
            << sample << "," << time_cursor.degree << ","
            << time_cursor.time_seek << ","
            << (time_cursor.time_scan / time_cursor.degree) << std::endl;
        assert(time_cursor.degree == graph.get_out_degree(sample));
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cout << "Usage: ./ubenchmark <graphfile> <wt_db_dir> <wt_db_name>"
                  << std::endl;
        return 0;
    }
    std::filesystem::path graphfile(argv[1]);
    string wt_db_dir(argv[2]);
    std::string wt_db_name = argv[3];
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
    WT_SESSION *session;
    if (CommonUtil::open_session(conn, &session) != 0)
    {
        throw GraphException("Cannot open session");
    }
    AdjList graph(opts, conn);
    graph.init_cursors();

    profile_wt_adjlist(graphfile, session, num_random_samples, graph);
}
