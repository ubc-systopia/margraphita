#ifndef TEST_ADJ
#define TEST_ADJ

#include "common.h"
#include "graph_exception.h"
#include "edgekey.h"
#include "sample_graph.h"
#include <cassert>

WT_CONNECTION *conn;
WT_CURSOR *cursor;
WT_SESSION *session;
const char *home;

void tearDown(EdgeKey graph);
void create_init_nodes(EdgeKey graph);
void test_node_add(EdgeKey graph);

#endif