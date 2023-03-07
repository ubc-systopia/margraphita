#ifndef TEST_UOEDGELIST
#define TEST_UOEDGELIST

#include <cassert>

#include "common.h"
#include "edgelist.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"
#include "edgelist.h"

// Need new class to access protected method
class GraphEngineTest : public GraphEngine
{
   public:
    GraphEngineTest(graph_engine_opts engine_opts) : GraphEngine(engine_opts) {}
    WT_CONNECTION* public_get_connection() { return get_connection(); }
};

#endif