#include <times.h>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "common.h"
#include "edgekey.h"
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

struct time_result seek_and_scan(node_id_t vertex,
                                 StandardGraph &graph,
                                 WT_SESSION *session)
{
    struct time_result results
    {
    };
    WT_CURSOR *edge_cursor = graph.get_edge_cursor();
    edge found;
    // seek
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

    // scan
    node_id_t src, dst;
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
    results.time_scan = timer.t_nanos();
    return results;
}

void profile_wt_std(const filesystem::path &graphfile,
                    WT_SESSION *session,
                    int samples,
                    StandardGraph &graph)
{
    std::vector<node_id_t> random_ids;
    // read boost serialized vector into random_ids
    char random_file_name[256];
    sprintf(random_file_name, "%s_random_ids.bin", graphfile.stem().c_str());
    std::ifstream random_in(random_file_name, std::ios::binary);
    boost::archive::binary_iarchive random_in_archive(random_in);
    random_in_archive >> random_ids;
    random_in.close();
    assert(random_ids.size() == 1000);

    std::cout << "Random samples: " << random_ids.size() << std::endl;

    // create file for adjlist seek and scan times
    char outfile_name[256];
    sprintf(outfile_name, "%s_std_ubench.txt", graphfile.stem().c_str());
    std::cout << outfile_name << std::endl;
    std::ofstream std_seek_scan_outfile(outfile_name);

    std_seek_scan_outfile
        << "vertex_id,degree,seek_time_ns,scan_time_per_edge_ns" << std::endl;
    for (node_id_t sample : random_ids)
    {
        struct time_result time = seek_and_scan(sample, graph, session);
        std_seek_scan_outfile << sample << "," << time.degree << ","
                              << time.time_seek << ","
                              << (time.time_scan / time.degree) << std::endl;
        assert(time.degree == graph.get_out_degree(sample));
    }
    std_seek_scan_outfile.close();
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
    opts.type = GraphType::Std;
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
    StandardGraph graph(opts, conn);
    graph.init_cursors();

    profile_wt_std(graphfile, session, num_random_samples, graph);
}
