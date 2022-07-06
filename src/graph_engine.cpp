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
    WT_SESSION *sess;
    CommonUtil::open_session(conn, &sess);
    wt_conn t{.connection = conn, .session = sess};

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