#ifndef TEST_ADJ
#define TEST_ADJ

#include <cassert>

#include "adj_list.h"
#include "common.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

// Need new class to access protected method
class GraphEngineTest : public GraphEngine
{
   public:
    GraphEngineTest(graph_engine_opts engine_opts) : GraphEngine(engine_opts) {}
    WT_CONNECTION* public_get_connection() { return get_connection(); }
};

#endif