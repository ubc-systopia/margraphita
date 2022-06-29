#ifndef GRAPH_ENGINE
#define GRAPH_ENGINE
#include <wiredtiger.h>

#include "adj_list.h"
#include "common.h"
#include "edgekey.h"
#include "graph.h"
#include "graph_exception.h"
#include "standard_graph.h"

class GraphEngine
{
   public:
    GraphEngine(graph_opts opts);      // TODO
    GraphBase *create_graph_handle();  // TODO
    void close_graph();                // TODO

   private:
    WT_CONNECTION *conn = nullptr;
    graph_opts opts;
    // LockSet Class?
};

#endif