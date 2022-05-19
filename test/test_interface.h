#ifndef TEST_INTERFACE
#define TEST_INTERFACE

#include <cassert>

#include "adj_list.h"
#include "common.h"
#include "edgekey.h"
#include "graph.h"
#include "graph_exception.h"
#include "sample_graph.h"
#include "standard_graph.h"

WT_CONNECTION *conn;
WT_CURSOR *cursor;
WT_SESSION *session;
const char *home;

class AdjListTester : public AdjList
{
   public:
    AdjListTester(graph_opts create_opts) : AdjList(create_opts) {}
};

void tearDown(AdjList graph);
void create_init_nodes(AdjList graph);
void test_node_add(adjlist graph);

#endif