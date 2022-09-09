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

void GraphEngine::create_indices()
{
    WT_SESSION *sess;
    CommonUtil::open_session(conn, &sess);
    // Should enforce other handles are closed here (TODO?)
    if (opts.type == GraphType::Std)
    {
        StandardGraph::create_indices(sess);
    }
    else if (opts.type == GraphType::EKey)
    {
        EdgeKey::create_indices(sess);
    }
}

void GraphEngine::calculate_thread_offsets()
{
    // Create snapshot here first?
    GraphBase *graph_stats = this->create_graph_handle();
    calculate_thread_offsets(num_threads,
                             graph_stats->get_num_nodes(),
                             graph_stats->get_num_edges(),
                             node_ranges,
                             edge_ranges,
                             opts.type);
    graph_stats->close();
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
        try
        {
            std::filesystem::create_directories(opts.stat_log);
        }
        catch (exception e)
        {
        }
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
    return node_ranges[thread_id];
}

edge_range GraphEngine::get_edge_range(int thread_id)
{
    return edge_ranges[thread_id];
}

void GraphEngine::calculate_thread_offsets(
    int thread_max,
    node_id_t num_nodes,
    node_id_t num_edges,
    std::vector<key_range> &node_ranges,
    std::vector<edge_range> &edge_offsets,
    GraphType type)
{
    node_id_t node_offset = 0;
    node_ranges.clear();
    edge_offsets.clear();

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

    for (auto x : node_ranges)
    {
        key_pair first = {x.start, 0}, last = {x.end + 1, -1};
        edge_range er(first, last);
        edge_offsets.push_back(er);
    }
}