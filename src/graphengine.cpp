#include "graphengine.h"

#include <filesystem>

GraphEngine::GraphEngine(graph_opts opts)
{
    this->opts = opts;
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