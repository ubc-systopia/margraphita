#include <cassert>

#include "common_util.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph.h"

#define delim "--------------"
#define INFO() fprintf(stderr, "%s\nNow running: %s\n", delim, __FUNCTION__);

void create_init_nodes(SplitEdgeKey &graph, bool is_directed)
{
  INFO()
  for (node n : SampleGraph::test_nodes)
  {
    graph.add_node(n);
  }
  for (edge x : SampleGraph::test_edges)
  {
    graph.add_edge(x, false);
  }
}

void test_get_node(SplitEdgeKey &graph)
{
  INFO()
  node found = graph.get_node(SampleGraph::node1.id);
  assert(found.id == 1);
  // now get a node that does not exist
  found = graph.get_node(-1);
  assert(found.id == 0);
  found = {0};
  found = graph.get_random_node();
  CommonUtil::dump_node(found);
}

void test_node_add(SplitEdgeKey &graph, bool read_optimize)
{
  INFO()
  node new_node = {.id = 11, .in_degree = 0, .out_degree = 0};
  graph.add_node(new_node);
  node found = graph.get_node(new_node.id);
  assert(new_node.id == found.id);
  if (read_optimize)
  {
    assert(found.in_degree == 0);
    assert(found.out_degree == 0);
  }
}

void test_get_nodes(SplitEdgeKey &graph)
{
  INFO()
  for (node x : graph.get_nodes())
  {
    CommonUtil::dump_node(x);
  }
}

void test_get_random_nodes(SplitEdgeKey &graph)
{
  INFO()
  fprintf(stderr, "Random node:\n");
  CommonUtil::dump_node(graph.get_random_node());
  CommonUtil::dump_node(graph.get_random_node());
}

void test_add_edge(SplitEdgeKey &graph, bool is_directed, bool is_weighted)
{
  INFO()
  edge to_insert = {
      .src_id = 5,
      .dst_id = 6,
      .edge_weight = 333};  // node 300 and 400 dont exist yet so we must also
                            // check if the nodes get created
  graph.add_edge(to_insert, false);
  edge found = graph.get_edge(5, 6);
  CommonUtil::dump_edge(found);
  assert(found.src_id == 5);
  assert(found.dst_id == 6);
  if (is_weighted) assert(found.edge_weight == 333);
  if (!is_directed)
  {
    found = graph.get_edge(6, 5);
    assert(found.src_id == 6);
    assert(found.dst_id == 5);
    if (is_weighted) assert(found.edge_weight == 333);
  }

  // Check if the nodes were created.
  node got = graph.get_node(5);
  assert(got.id == 5);
  got = graph.get_node(6);
  assert(got.id == 6);

  // //Now check if the adjlists were updated
  WT_CURSOR *e_cur = graph.get_out_edge_cursor();
  CommonUtil::ekey_set_key(e_cur, 5, OutOfBand_ID_MIN);
  assert(e_cur->search(e_cur) == 0);
  assert(graph.get_out_degree(5) == 1);
  assert(graph.get_in_degree(6) == 1);

  if (is_directed)
  {
    assert(graph.get_in_degree(5) == 0);
    assert(graph.get_out_degree(6) == 0);
  }
  else
  {
    assert(graph.get_in_degree(5) == 1);
    assert(graph.get_out_degree(6) == 1);
  }
}

void test_get_edge(SplitEdgeKey &graph, bool directed)
{
  INFO()
  edge found =
      graph.get_edge(SampleGraph::edge1.src_id, SampleGraph::edge1.dst_id);
  assert(found.src_id == SampleGraph::edge1.src_id);
  assert(found.dst_id == SampleGraph::edge1.dst_id);

  if (!directed)
  {
    found =
        graph.get_edge(SampleGraph::edge1.dst_id, SampleGraph::edge1.src_id);
    assert(found.src_id == SampleGraph::edge1.dst_id);
    assert(found.dst_id == SampleGraph::edge1.src_id);
  }
  // Now get a non-existent edge
  found = graph.get_edge(222, 333);
  assert(found.src_id == 0);
  assert(found.dst_id == 0);
  assert(found.edge_weight == 0);
}

void test_get_edges(SplitEdgeKey &graph)
{
  INFO()
  std::vector<edge> edges = graph.get_edges();
  for (auto e : edges)
  {
    CommonUtil::dump_edge(e);
  }
}

void test_get_out_edges(SplitEdgeKey &graph)
{
  INFO()
  std::vector<edge> edges = graph.get_out_edges(1);
  for (auto e : edges)
  {
    CommonUtil::dump_edge(e);
  }
  assert(edges.size() == 3);
  // compare edge0
  assert(edges.at(0).src_id == SampleGraph::edge1.src_id);
  assert(edges.at(0).dst_id == SampleGraph::edge1.dst_id);
  // compare edge1
  assert(edges.at(1).src_id == SampleGraph::edge2.src_id);
  assert(edges.at(1).dst_id == SampleGraph::edge2.dst_id);

  assert(edges.at(2).src_id == SampleGraph::edge4.src_id);
  assert(edges.at(2).dst_id == SampleGraph::edge4.dst_id);

  // Now test for a node that has no out edge
  edges = graph.get_out_edges(4);
  assert(edges.empty());

  // Now try getting out edges for a node that does not exist
  bool assert_fail = false;
  try
  {
    edges = graph.get_out_edges(1500);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << endl;
    assert_fail = true;
  }
  assert(assert_fail);
}

void test_get_out_nodes(SplitEdgeKey &graph)
{
  INFO()
  std::vector<node> nodes = graph.get_out_nodes(1);
  std::vector<node_id_t> nodes_id = graph.get_out_nodes_id(1);
  assert(nodes.size() == 3);
  assert(nodes_id.size() == 3);
  assert(nodes.at(0).id == SampleGraph::node2.id);  // edge(1->2)
  assert(nodes.at(0).id == nodes_id.at(0));
  assert(nodes.at(1).id == SampleGraph::node3.id);  // edge(1->3)
  assert(nodes.at(1).id == nodes_id.at(1));
  assert(nodes.at(2).id == SampleGraph::node7.id);  // edge(1->7)
  assert(nodes.at(2).id == nodes_id.at(2));
  // test for a node that has no out-edge
  nodes = graph.get_out_nodes(4);
  nodes_id = graph.get_out_nodes_id(4);
  assert(nodes.empty());
  assert(nodes_id.empty());

  // test for a node that does not exist
  bool assert_fail = false;
  try
  {
    nodes = graph.get_out_nodes(1500);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << endl;
    assert_fail = true;
  }
  assert(assert_fail);
}

void test_get_in_edges(SplitEdgeKey &graph)
{
  INFO()
  std::vector<edge> edges = graph.get_in_edges(3);
  for (auto e : edges)
  {
    CommonUtil::dump_edge(e);
  }
  assert(edges.size() == 2);
  // Check edge0
  assert(edges.at(0).src_id == SampleGraph::edge2.src_id);
  assert(edges.at(0).dst_id == SampleGraph::edge2.dst_id);
  // Check edge1
  assert(edges.at(1).src_id == SampleGraph::edge3.src_id);
  assert(edges.at(1).dst_id == SampleGraph::edge3.dst_id);

  // now test for a node that has no in-edge
  edges = graph.get_in_edges(4);
  assert(edges.empty());

  // Now try getting in edges for a node that does not exist.
  bool assert_fail = false;
  try
  {
    edges = graph.get_out_edges(1500);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << endl;
    assert_fail = true;
  }
  assert(assert_fail);
}

void test_get_in_nodes(SplitEdgeKey &graph)
{
  INFO()
  int test_id1 = 3, test_id2 = 4, test_id3 = 1500;
  std::vector<node> nodes = graph.get_in_nodes(test_id1);
  for (auto n : nodes) CommonUtil::dump_node(n);
  std::vector<node_id_t> nodes_id = graph.get_in_nodes_id(test_id1);
  assert(nodes.size() == 2);
  assert(nodes_id.size() == 2);
  assert(nodes.at(0).id == SampleGraph::node1.id);
  assert(nodes.at(0).id == nodes_id.at(0));
  assert(nodes.at(1).id == SampleGraph::node2.id);
  assert(nodes.at(1).id == nodes_id.at(1));

  // test for a node that has no in_edge
  nodes = graph.get_in_nodes(test_id2);
  nodes_id = graph.get_in_nodes_id(test_id2);
  assert(nodes.empty());
  assert(nodes_id.empty());

  // test for a node that does not exist
  bool assert_fail = false;
  try
  {
    nodes = graph.get_in_nodes(test_id3);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << endl;
    assert_fail = true;
  }
  assert(assert_fail);
}

void test_get_in_and_out_degree(SplitEdgeKey &graph, bool directed)
{
  INFO()
  degree_t indeg, outdeg;
  indeg = graph.get_in_degree(3);
  outdeg = graph.get_out_degree(3);
  assert(indeg == 2);
  if (!directed)
  {
    assert(outdeg == 2);
  }
  else
  {
    assert(outdeg == 0);
  }
  indeg = graph.get_in_degree(1);
  outdeg = graph.get_out_degree(1);
  assert(outdeg == 3);
  if (!directed)
  {
    assert(indeg == 3);
  }
  else
  {
    assert(indeg == 0);
  }
}

void test_delete_node(SplitEdgeKey &graph, bool is_directed)
{
  INFO()
  WT_CURSOR *e_cur = graph.get_out_edge_cursor();
  // Verify node2 exists
  assert(graph.has_node(SampleGraph::node2.id) == true);

  // Delete node2 and verify it was actually deleted
  graph.delete_node(SampleGraph::node2.id);
  CommonUtil::ekey_set_key(e_cur, SampleGraph::node2.id, OutOfBand_ID_MIN);
  int ret = e_cur->search(e_cur);
  assert(ret != 0);

  // check that edge(2,3) is deleted
  CommonUtil::ekey_set_key(e_cur, 2, 3);
  assert(e_cur->search(e_cur) != 0);
  // check that edge(1,2) is deleted
  CommonUtil::ekey_set_key(e_cur, 1, 2);
  assert(e_cur->search(e_cur) != 0);

  // Now delete the reverse edges for undirected graph
  if (is_directed)
  {
    CommonUtil::ekey_set_key(e_cur, 3, 2);
    assert(e_cur->search(e_cur) != 0);
    CommonUtil::ekey_set_key(e_cur, 2, 1);
    assert(e_cur->search(e_cur) != 0);
  }
  // Verify that the in and out degrees of node 1 and 3 got updated
  if (is_directed)
  {
    // std::cout << graph.get_out_degree(SampleGraph::node1.id)<< std::endl;
    assert(graph.get_out_degree(SampleGraph::node1.id) == 2);
    // std::cout << graph.get_in_degree(SampleGraph::node3.id) <<std::endl;
    assert(graph.get_in_degree(SampleGraph::node3.id) == 1);
  }
  else
  {
    CommonUtil::dump_node(graph.get_node(SampleGraph::node1.id));
    CommonUtil::dump_node(graph.get_node(SampleGraph::node3.id));

    assert(graph.get_in_degree(SampleGraph::node1.id) == 2);
    std::cout << graph.get_out_degree(SampleGraph::node3.id) << std::endl;
    assert(graph.get_out_degree(SampleGraph::node3.id) == 1);
  }
}

void test_delete_edge(SplitEdgeKey &graph, bool is_directed)
{
  INFO()
  WT_CURSOR *e_cur = graph.get_out_edge_cursor();
  // Verify edge(1,3) exists
  assert(graph.has_edge(SampleGraph::edge2.src_id, SampleGraph::edge2.dst_id) ==
         true);

  // Delete edge(1,3) and verify it was actually deleted
  graph.delete_edge(SampleGraph::edge2.src_id, SampleGraph::edge2.dst_id);
  CommonUtil::ekey_set_key(
      e_cur, SampleGraph::edge2.src_id, SampleGraph::edge2.dst_id);
  int ret = e_cur->search(e_cur);
  assert(ret != 0);

  // Verify that the in and out degrees of node 1 and 3 got updated
  if (is_directed)
  {
    assert(graph.get_out_degree(SampleGraph::edge2.src_id) == 1);
    assert(graph.get_in_degree(SampleGraph::edge2.src_id) == 0);
    assert(graph.get_out_degree(SampleGraph::edge2.dst_id) == 0);
    assert(graph.get_in_degree(SampleGraph::edge2.dst_id) == 0);
  }
  else
  {
    assert(graph.get_out_degree(SampleGraph::edge2.src_id) == 1);
    assert(graph.get_in_degree(SampleGraph::edge2.dst_id) == 0);
  }
}

void test_EdgeCursor(SplitEdgeKey &graph, bool directed)
{
  INFO()
  EdgeCursor *edge_cursor = graph.get_edge_iter();
  edge found;
  std::vector<std::pair<node_id_t, node_id_t>> expected;
  if (directed)
  {
    expected = {{1, 3}, {1, 7}, {5, 6}, {7, 8}, {8, 7}};
  }
  else
  {
    expected = {{1, 3}, {1, 7}, {3, 1}, {5, 6}, {6, 5}, {7, 1}, {7, 8}, {8, 7}};
  }

  int i = 0;
  edge_cursor->next(&found);
  while (found.src_id != OutOfBand_ID_MAX)
  {
    assert(found.src_id == expected[i].first);
    assert(found.dst_id == expected[i].second);
    CommonUtil::dump_edge(found);
    edge_cursor->next(&found);
    i++;
  }
  edge_cursor->close();
  delete edge_cursor;
}

void test_EdgeCursor_Range(SplitEdgeKey &graph, bool directed)
{
  INFO()
  EdgeCursor *edge_cursor = graph.get_edge_iter();
  edge_cursor->set_key_range(edge_range(key_pair{1, 4}, key_pair{8, 1}));
  edge found;
  std::vector<std::pair<node_id_t, node_id_t>> expected;
  if (directed)
  {
    expected = {{1, 7}, {5, 6}, {7, 8}};
  }
  else
  {
    expected = {{1, 7}, {3, 1}, {5, 6}, {6, 5}, {7, 1}, {7, 8}};
  }
  int i = 0;
  edge_cursor->next(&found);
  while (found.src_id != OutOfBand_ID_MAX)
  {
    assert(found.src_id == expected[i].first);
    assert(found.dst_id == expected[i].second);
    CommonUtil::dump_edge(found);
    edge_cursor->next(&found);
    i++;
  }
  delete edge_cursor;
}

void test_OutCursor(SplitEdgeKey &graph)
{
  INFO()
  OutCursor *out_cursor = graph.get_outnbd_iter();
  out_cursor->set_key_range(key_range{0, 0});
  adjlist found;
  out_cursor->next(&found);
  while (found.node_id != OutOfBand_ID_MAX)
  {
    CommonUtil::dump_adjlist(found);
    found.clear();
    out_cursor->next(&found);
  }
  out_cursor->reset();

  // testing next() with a key range set
  fprintf(stderr, "\n ---- now testing with a range (3,6) set:\n");

  out_cursor->set_key_range({3, 6});
  found.clear();
  out_cursor->next(&found);
  CommonUtil::dump_adjlist(found);
  out_cursor->next(&found);
  while (found.node_id != OutOfBand_ID_MAX)
  {
    CommonUtil::dump_adjlist(found);
    found.clear();
    out_cursor->next(&found);
  }
  out_cursor->reset();
  delete out_cursor;
}

void test_NodeCursor(SplitEdgeKey &graph)
{
  INFO()
  NodeCursor *node_cursor = graph.get_node_iter();
  node found = {0, 0, 0};
  node_id_t nodeIdList[] = {1, 3, 4, 5, 6, 7, 8, 11};
  int i = 0;
  node_cursor->next(&found);
  while (found.id != OutOfBand_ID_MAX)
  {
    assert(found.id == nodeIdList[i]);
    CommonUtil::dump_node(found);
    node_cursor->next(&found);
    i++;
  }
  delete node_cursor;
}

void test_NodeCursor_Range(SplitEdgeKey &graph)
{
  INFO()
  NodeCursor *node_cursor = graph.get_node_iter();
  node found;
  node_id_t nodeIdList[] = {3, 4, 5, 6};
  int i = 0;
  node_cursor->set_key_range(key_range{3, 6});
  node_cursor->next(&found);
  while (found.id != OutOfBand_ID_MAX)
  {
    assert(found.id == nodeIdList[i]);
    CommonUtil::dump_node(found);
    node_cursor->next(&found);
    i++;
  }
  delete node_cursor;
}
void test_InCursor(SplitEdgeKey &graph)
{
  INFO()
  InCursor *in_cursor = graph.get_innbd_iter();
  in_cursor->set_key_range(key_range{0, 0});
  adjlist found;
  in_cursor->next(&found);
  while (found.node_id != OutOfBand_ID_MAX)
  {
    CommonUtil::dump_adjlist(found);
    found.clear();
    in_cursor->next(&found);
  }
  in_cursor->reset();

  // testing next() with a key range set
  fprintf(stderr, "\n ---- now testing with a range (3,6) set :\n");

  in_cursor->set_key_range(key_range{3, 6});
  found.clear();
  in_cursor->next(&found);

  while (found.node_id != OutOfBand_ID_MAX)
  {
    CommonUtil::dump_adjlist(found);
    found.clear();
    in_cursor->next(&found);
  }
  delete in_cursor;
}

void tearDown(SplitEdgeKey &graph) { graph.close(true); }

int main()
{
  const int THREAD_NUM = 1;
  graph_opts opts;
  opts.create_new = true;
  opts.optimize_create = false;
  opts.is_directed = false;
  //  opts.is_directed = true;
  opts.read_optimize = true;
  opts.is_weighted = true;
  opts.type = GraphType::SplitEKey;
  opts.db_name = "test_split_edgekey";
  opts.db_dir = "./db";
  opts.conn_config = "cache_size=10GB";
  if (const char *env_p = std::getenv("GRAPH_PROJECT_DIR"))
  {
    opts.stat_log = std::string(env_p);
  }
  else
  {
    std::cout << "GRAPH_PROJECT_DIR not set. Using CWD" << std::endl;
    opts.stat_log = "./";
  }

  GraphEngine myEngine(THREAD_NUM, opts);
  WT_CONNECTION *conn = myEngine.get_connection();
  SplitEdgeKey graph(opts, conn);

  create_init_nodes(graph, opts.is_directed);
  test_get_node(graph);
  test_node_add(graph, opts.read_optimize);
  test_get_nodes(graph);
  test_get_random_nodes(graph);
  test_add_edge(graph, opts.is_directed, opts.is_weighted);
  test_get_edge(graph, opts.is_directed);
  test_get_edges(graph);
  test_get_out_edges(graph);
  test_get_out_nodes(graph);
  test_get_in_edges(graph);

  test_get_in_nodes(graph);
  test_get_in_and_out_degree(graph, opts.is_directed);
  test_get_edges(graph);
  test_get_nodes(graph);
  test_delete_node(graph, opts.is_directed);

  test_get_edges(graph);
  test_EdgeCursor(graph, opts.is_directed);
  test_EdgeCursor_Range(graph, opts.is_directed);
  test_InCursor(graph);
  //! TODO: test_InCursor_Range(graph);
  test_OutCursor(graph);
  test_NodeCursor(graph);
  test_NodeCursor_Range(graph);
  test_delete_edge(graph, opts.is_directed);

  tearDown(graph);
  return 0;
}
