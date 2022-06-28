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


    private:
    LockSet* locks;
    int num_threads;
    graph_opts opts;
};

#endif