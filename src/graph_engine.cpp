#include "graph_engine.h"

using namespace std;

GraphEngine::GraphEngine(graph_engine_opts engine_opts)
{
    num_threads = engine_opts.num_threads;
    opts = engine_opts.opts;
    check_opts_valid();
    locks = new LockSet();
}

GraphEngine::~GraphEngine() { delete locks; }

GraphBase *GraphEngine::create_graph_handle()
{
    GraphBase *ptr = nullptr;
    if (opts.type == GraphType::Std)
    {
        ptr = new StandardGraph(opts);
        ptr->set_locks(locks);
        return ptr;
    }
    else if (opts.type == GraphType::Adj)
    {
        ptr = new AdjList(opts);
        ptr->set_locks(locks);
        return ptr;
    }
    else if (opts.type == GraphType::EKey)
    {
        ptr = new EdgeKey(opts);
        ptr->set_locks(locks);
        return ptr;
    }

    throw GraphException("Failed to create graph object");
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
    if (opts.create_new)
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
    }
}