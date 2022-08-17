#include "graph_engine.h"

#include <filesystem>
using namespace std;

GraphEngine::GraphEngine(graph_engine_opts engine_opts)
{
    num_threads = engine_opts.num_threads;
    opts = engine_opts.opts;
    check_opts_valid();
    locks = new LockSet();
    if (opts.create_new)
    {
        create_new_graph();
    }
    else
    {
        open_connection();
    }
    graph_stats = this->create_graph_handle();
    calculate_thread_offsets(num_threads,
                             graph_stats->get_num_nodes(),
                             graph_stats->get_num_edges(),
                             node_ranges,
                             edge_ranges,
                             opts.type,
                             graph_stats->get_new_edge_cursor());
}

GraphEngine::~GraphEngine()
{
    delete locks;
    if (conn != NULL)
    {
        close_connection();
    }
}

GraphBase *GraphEngine::create_graph_handle()
{
    WT_SESSION *sess;
    CommonUtil::open_session(conn, &sess);

    GraphBase *ptr = nullptr;
    if (opts.type == GraphType::Std)
    {
        ptr = new StandardGraph(opts, conn);
    }
    else if (opts.type == GraphType::Adj)
    {
        ptr = new AdjList(opts, conn);
    }
    else if (opts.type == GraphType::EKey)
    {
        ptr = new EdgeKey(opts, conn);
    }
    else
    {
        throw GraphException("Failed to create graph object");
    }
    ptr->set_locks(locks);
    return ptr;
}

void GraphEngine::check_opts_valid()
{
    if (num_threads < 1)
    {
        throw GraphException("Number of threads must be at least 1");
    }

    try
    {
        CommonUtil::check_graph_params(opts);
    }
    catch (GraphException &G)
    {
        std::cout << G.what() << std::endl;
    }
    if (!CommonUtil::check_dir_exists(opts.stat_log))
    {
        std::filesystem::create_directories(opts.stat_log);
    }
}

void GraphEngine::create_new_graph()
{
    std::string dirname = opts.db_dir + "/" + opts.db_name;
    CommonUtil::create_dir(dirname);
    if (CommonUtil::open_connection(const_cast<char *>(dirname.c_str()),
                                    opts.stat_log,
                                    opts.conn_config,
                                    &conn) < 0)
    {
        throw GraphException("Cannot open connection to new DB");
    };

    if (opts.type == GraphType::Std)
    {
        StandardGraph::create_wt_tables(opts, conn);
    }
    else if (opts.type == GraphType::Adj)
    {
        AdjList::create_wt_tables(opts, conn);
    }
    else if (opts.type == GraphType::EKey)
    {
        EdgeKey::create_wt_tables(opts, conn);
    }

    GraphBase::create_metadata_table(opts, conn);
}

void GraphEngine::open_connection()
{
    std::string dirname = opts.db_dir + "/" + opts.db_name;
    if (CommonUtil::open_connection(const_cast<char *>(dirname.c_str()),
                                    opts.stat_log,
                                    opts.conn_config,
                                    &conn) < 0)
    {
        throw GraphException("Cannot open connection");
    };
}

void GraphEngine::close_connection()
{
    CommonUtil::close_connection(conn);
    conn = NULL;
}

WT_CONNECTION *GraphEngine::get_connection() { return conn; }

void GraphEngine::close_graph() { close_connection(); }

key_range GraphEngine::get_key_range(int thread_id)
{
    return key_range{-1, -1};
}

edge_range GraphEngine::get_edge_range(int thread_id)
{
    key_pair start_edge = key_pair{-1, -1};
    key_pair end_edge = key_pair{-1, -1};
    return edge_range{start_edge, end_edge};
}

void GraphEngine::calculate_thread_offsets(
    int thread_max,
    node_id_t num_nodes,
    node_id_t num_edges,
    std::vector<key_range> &node_ranges,
    std::vector<edge_range> &edge_offsets,
    GraphType type,
    WT_CURSOR *ecur)
{
    /*
    Steps:
    1. calculate ndoe offsets
    2. use node offsets to find the nearest edge
    3. assign those edges
     */
    // Calculate the node offsets.
    node_id_t node_offset = 0;

    for (int i = 0; i < thread_max; i++)
    {
        key_range temp;
        if (i == thread_max - 1)
        {
            temp = {node_offset, num_nodes};
        }
        else
        {
            temp = {node_offset, (node_offset + (num_nodes / thread_max) - 1)};
        }

        node_ranges.push_back(temp);
        node_offset += num_nodes / thread_max;
    }

    key_pair first = {0, 0}, last = {0, 0};

    for (auto x : node_ranges)
    {
        node_id_t out_of_band = -1;
        if (type != GraphType::EKey)
        {
            ecur->set_key(ecur, x.start - 1, out_of_band);
        }
        else
        {
            ecur->set_key(ecur, x.start, out_of_band);
        }

        int status;
        ecur->search_near(ecur, &status);
        if (status == 0)
        {
            ecur->get_key(ecur, &first.src_id, &first.dst_id);
        }
        else if (status < 0)
        {
            ecur->get_key(ecur, &first.src_id, &first.dst_id);
        }
        else if (status > 0)
        {
            ecur->get_key(ecur, &first.src_id, &first.dst_id);
        }
        edge_range er(first, last);
        edge_offsets.push_back(er);
    }
    // now for all positions from i+1 to thread_max, call prev to set positions
    // i
    for (int i = 1; i < thread_max; i++)
    {
        key_pair key = edge_offsets.at(i).start;
        ecur->set_key(ecur, key.src_id, key.dst_id);
        ecur->search(ecur);
        ecur->prev(ecur);
        ecur->get_key(ecur,
                      &edge_offsets.at(i - 1).end.src_id,
                      &edge_offsets.at(i - 1).end.dst_id);
    }
    // now set the last
    ecur->reset(ecur);
    ecur->prev(ecur);
    ecur->get_key(ecur,
                  &edge_offsets.at(thread_max - 1).end.src_id,
                  &edge_offsets.at(thread_max - 1).end.dst_id);
}
