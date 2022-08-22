#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <vector>

#include "adj_list.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"

int main(int argc, char *argv[])
{
    cout << "Initializing Database" << endl;
    CmdLineBase init_cli(argc, argv);
    if (!init_cli.parse_args())
    {
        return -1;
    }

    graph_opts opts;
    opts.create_new = init_cli.is_create_new();
    opts.is_directed = init_cli.is_directed();
    opts.read_optimize = init_cli.is_read_optimize();
    opts.is_weighted = init_cli.is_weighted();
    opts.optimize_create = init_cli.is_create_optimized();
    opts.db_name = init_cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = init_cli.get_db_path();
    std::string init_log = init_cli.get_logdir();  //$RESULT/$bmark
    if (init_log.empty())
    {
        init_log = ".";
    }
    opts.stat_log = init_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";  // init_cli.get_conn_config();
    opts.type = init_cli.get_graph_type();
    const int THREAD_NUM = 1;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    t.stop();
    cout << "Graph created in " << t.t_micros() << endl;

    // must use derived class object here
    if (init_cli.is_exit_on_create())  // Exit after creating the db
    {
        graphEngine.close_graph();
        exit(0);
    }

    // create_indices does not apply to adjacency lists
    if (init_cli.is_index_create() && opts.type != GraphType::Adj)
    {
        t.start();
        graphEngine.create_indices();
        t.stop();
        cout << "Indices created in " << t.t_micros() << endl;
    }
    graphEngine.close_graph();
    exit(0);
}