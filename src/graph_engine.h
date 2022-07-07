#ifndef GRAPH_ENGINE
#define GRAPH_ENGINE

#include "adj_list.h"
#include "common.h"
#include "edgekey.h"
#include "graph.h"
#include "graph_exception.h"
#include "lock.h"
#include "standard_graph.h"

using namespace std;

class GraphEngine
{
   public:
    struct graph_engine_opts
    {
        int num_threads;
        graph_opts opts;
    };

    GraphEngine(graph_engine_opts engine_opts);
    ~GraphEngine();
    GraphBase* create_graph_handle();
    void close_graph();

   protected:
    WT_CONNECTION* conn = nullptr;
    LockSet* locks;
    int num_threads;
    graph_opts opts;

    void check_opts_valid();
    void create_new_graph();
    void open_connection();
    void close_connection();
    WT_CONNECTION* get_connection();
};

#endif