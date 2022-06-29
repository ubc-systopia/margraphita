#ifndef GRAPH_ENGINE
#define GRAPH_ENGINE

#include "graph.h"

#include "adj_list.h"
#include "edgekey.h"
#include "standard_graph.h"

#include "common.h"
#include "lock.h"
#include "graph_exception.h"

using namespace std;

class GraphEngine
{

    struct graph_engine_opts
    {
        int num_threads;
        graph_opts opts;
    };

    public:
    GraphEngine(graph_engine_opts engine_opts);
    GraphBase* create_graph_handle();
    void close_graph(); // TODO


    private:
    void check_opts_valid();
    WT_CONNECTION *conn;
    LockSet* locks;
    int num_threads;
    graph_opts opts;
};

#endif