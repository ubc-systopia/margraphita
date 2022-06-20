#include <cassert>
#include <thread>
#include <vector>

#include "adj_list.h"
#include "common.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define THREAD_NUM 8
using namespace std;

void f(graph_opts opts, wt_conn connection)
{
    GraphBase *graph = new AdjList(opts, connection);
    id_t id = rand() % 99 + 20;
    graph->add_node({.id = id});
    cout << "added " << id << '\n';
    graph->close();
}

int main()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_dir = "./db";
    opts.db_name = "test_adj";
    opts.conn_config = "cache_size=10GB";
    opts.stat_log = std::getenv("GRAPH_PROJECT_DIR");

    vector<wt_conn> connections;
    vector<thread *> threads;
    WT_CONNECTION *conn;
    string db_name = opts.db_dir + "/" + opts.db_name;
    CommonUtil::open_connection(const_cast<char *>(db_name.c_str()),
                                opts.stat_log,
                                opts.conn_config,
                                &conn);
    for (int i = 0; i < THREAD_NUM; i++)
    {
        wt_conn t;
        t.conn = conn;
        CommonUtil::open_session(t.conn, &t.session);
        connections.push_back(t);
    }

    for (int i = 0; i < THREAD_NUM; i++)
    {
        threads.push_back(new std::thread(&f, opts, connections[i]));
    }

    for (int i = 0; i < THREAD_NUM; i++)
    {
        threads[i]->join();
    }
    return 0;
}