#ifndef TEST_ADJ
#define TEST_ADJ

#include <cassert>

#include "common.h"
#include "edgekey.h"
#include "graph_exception.h"
#include "sample_graph.h"

WT_CONNECTION *conn;
WT_CURSOR *cursor;
WT_SESSION *session;
const char *home;

class EdgeKeyTester : public EdgeKey
{
   public:
    EdgeKeyTester(graph_opts create_opts) : EdgeKey(create_opts) {}
};

void tearDown(EdgeKey graph);
void create_init_nodes(EdgeKey graph);
void test_node_add(EdgeKey graph);

#endif