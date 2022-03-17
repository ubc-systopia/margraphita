#ifndef TEST_STD
#define TEST_STD

#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "sample_graph.h"
#include <cassert>

WT_CONNECTION *conn;
WT_CURSOR *cursor;
WT_SESSION *session;
const char *home;

class StandardGraphTester : public StandardGraph
{
public:
    StandardGraphTester(graph_opts opts) : StandardGraph(opts)
    {
    }
};

void tearDown(StandardGraph graph);
void create_init_nodes(StandardGraph graph);
void dump_node(node to_print);
void test_node_add(StandardGraph graph);

#endif