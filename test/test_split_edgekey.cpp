#include <cassert>
#include <cstring>

#include "common_util.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "prop_schema.h"
#include "sample_graph_ekey.h"

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
  CommonUtil::dump_node(found);
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
      .edge_weight = 333.55};  // node 300 and 400 dont exist yet so we must
                               // also check if the nodes get created
  graph.add_edge(to_insert, false);
  edge found = graph.get_edge(5, 6);
  CommonUtil::dump_edge(found);
  assert(found.src_id == 5);
  assert(found.dst_id == 6);
  if (is_weighted) assert(found.edge_weight == 333.55);
  if (!is_directed)
  {
    found = graph.get_edge(6, 5);
    assert(found.src_id == 6);
    assert(found.dst_id == 5);
    if (is_weighted) assert(found.edge_weight == 333.55);
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
  edge found{};
  std::vector<std::pair<node_id_t, node_id_t>> expected;
  if (directed)
  {
    expected = {{1, 3}, {1, 7}, {5, 6}, {7, 8}, {8, 7}, {222, 333}};
  }
  else
  {
    expected = {{1, 3}, {1, 7}, {3, 1}, {5, 6}, {6, 5}, {7, 1}, {7, 8}, {8, 7}, {222, 333}, {333, 222}};
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
  node_id_t nodeIdList[] = {1, 3, 4, 5, 6, 7, 8, 11, 222, 333};
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

void tearDown(SplitEdgeKey &graph)
{
  INFO();
  graph.close(true);
}

// Test what happens when a node or edge that already exists is inserted again.
// SplitEdgeKey::error_check_insert_txn(ret, /*ignore_duplicate_key=*/false)
// rolls back the active transaction on WT_DUPLICATE_KEY, so the session is
// clean after each call and both tests can share the same graph instance.
void test_duplicate_additions(SplitEdgeKey &graph)
{
  INFO();

  // --- duplicate node ---
  // node7 (id=7) still exists at this point (not deleted by any prior test).
  // Check its degrees before and after to observe the corruption.
  node dup_node = {.id = SampleGraph::node7.id};
  degree_t in_before  = graph.get_in_degree(dup_node.id);
  degree_t out_before = graph.get_out_degree(dup_node.id);
  int ret = graph.add_node(dup_node);
  degree_t in_after   = graph.get_in_degree(dup_node.id);
  degree_t out_after  = graph.get_out_degree(dup_node.id);
  fprintf(stderr,
          "add_node(existing id=%lu):  ret=%d  wiredtiger says: \"%s\"\n"
          "  out_edge_cursor is opened with overwrite=true, so cursor->insert()\n"
          "  silently overwrites the existing node sentinel with (in=0, out=0).\n"
          "  Degree before: in=%u out=%u  |  Degree after: in=%u out=%u\n"
          "  BUG: non-zero degrees are corrupted to 0.\n",
          (unsigned long)dup_node.id, ret, wiredtiger_strerror(ret),
          in_before, out_before, in_after, out_after);

  // --- duplicate edge ---
  // edge4 (1->7) still exists at this point (node2 was deleted, not node1/7).
  edge dup_edge = {.src_id = SampleGraph::edge4.src_id,
                   .dst_id = SampleGraph::edge4.dst_id,
                   .edge_weight = 99.99};
  degree_t src_out_before = graph.get_out_degree(dup_edge.src_id);
  degree_t dst_in_before  = graph.get_in_degree(dup_edge.dst_id);
  ret = graph.add_edge(dup_edge, false);
  degree_t src_out_after  = graph.get_out_degree(dup_edge.src_id);
  degree_t dst_in_after   = graph.get_in_degree(dup_edge.dst_id);
  fprintf(stderr,
          "add_edge(existing %lu->%lu):  ret=%d  wiredtiger says: \"%s\"\n"
          "  Cursors opened with overwrite=true: the edge weight is silently\n"
          "  updated to %.2f, but add_node_txn still increments degrees,\n"
          "  so they are double-counted.\n"
          "  src out-degree: %u -> %u  |  dst in-degree: %u -> %u\n",
          (unsigned long)dup_edge.src_id, (unsigned long)dup_edge.dst_id,
          ret, wiredtiger_strerror(ret),
          dup_edge.edge_weight,
          src_out_before, src_out_after, dst_in_before, dst_in_after);
}

void test_ro_get_nodes(GraphBase *graph)
{
  INFO();
  int n = 10;
  for (node x : graph->get_nodes())
  {
    CommonUtil::dump_node(x);
#ifdef B64
    assert(sizeof(x.id) == 8);
#else
    assert(sizeof(x.id) == 4);
#endif
    if (--n == 0) break;
  }
}

void test_update_edge(SplitEdgeKey &graph, bool is_directed)
{
  INFO();
  edge to_update = {
      .src_id = 1,
      .dst_id = 3,
      .edge_weight = 111.11};  // edge (1,3) exists in the sample graph
  int test_id1 = 1, test_id2 = 3;
  graph.update_edge(to_update);
  edge found = graph.get_edge(test_id1, test_id2);
  CommonUtil::dump_edge(found);
  assert(found.edge_weight == 111.11);
  if (!is_directed)
  {
    found = graph.get_edge(test_id2, test_id1);
    CommonUtil::dump_edge(found);
    assert(found.src_id == test_id2);
    assert(found.dst_id == test_id1);
    assert(found.edge_weight == 111.11);
  }

  // Now try updating a non-existent edge
  to_update = {.src_id = 222,
               .dst_id = 333,
               .edge_weight = 44.44};  // edge (222,333) does not exist

  bool updated = graph.update_edge(to_update);
  assert(updated == true);  // add the edge if it does not exist.
  edge found1 = graph.get_edge(222, 333);
  CommonUtil::dump_edge(found1);
  assert(found1.src_id == 222);
  assert(found1.dst_id == 333);
  assert(found1.edge_weight == 44.44);
}

void test_embedded_node_props(SplitEdgeKey &graph)
{
  INFO();
  // add two nodes, then set_node_properties, then get_node_properties
  graph.add_node({.id = 100});
  graph.add_node({.id = 200});

  // Person at node 100
  uint8_t person_buf[SNBPersonSchema::TOTAL_SIZE] = {};
  SNBPersonSchema::set_creation_date(person_buf, 1000000LL);
  SNBPersonSchema::set_birthday(person_buf, 2000000LL);
  SNBPersonSchema::set_gender(person_buf, 1);
  graph.set_node_properties(100, person_buf, SNBPersonSchema::TOTAL_SIZE);

  // Post at node 200
  uint8_t post_buf[SNBPostSchema::TOTAL_SIZE] = {};
  SNBPostSchema::set_creation_date(post_buf, 3000000LL);
  SNBPostSchema::set_length(post_buf, 42);
  graph.set_node_properties(200, post_buf, SNBPostSchema::TOTAL_SIZE);

  // Verify person
  prop_blob pb = graph.get_node_properties(100);
  assert(pb.data != nullptr);
  assert(pb.size == SNBPersonSchema::TOTAL_SIZE);
  assert(SNBPersonSchema::get_creation_date(pb.data) == 1000000LL);
  assert(SNBPersonSchema::get_birthday(pb.data) == 2000000LL);
  assert(SNBPersonSchema::get_gender(pb.data) == 1);
  delete[] pb.data;

  // Verify post
  pb = graph.get_node_properties(200);
  assert(pb.data != nullptr);
  assert(pb.size == SNBPostSchema::TOTAL_SIZE);
  assert(SNBPostSchema::get_creation_date(pb.data) == 3000000LL);
  assert(SNBPostSchema::get_length(pb.data) == 42);
  delete[] pb.data;

  fprintf(stderr, "test_embedded_node_props: PASSED\n");
}

void test_embedded_edge_props(SplitEdgeKey &graph)
{
  INFO();
  // nodes 100 and 200 already exist from test_embedded_node_props
  // knows edge: 100 -> 200 (directed)
  graph.add_edge({.src_id = 100, .dst_id = 200, .edge_weight = 0.0}, false);

  uint8_t knows_buf[SNBKnowsSchema::TOTAL_SIZE] = {};
  SNBKnowsSchema::set_creation_date(knows_buf, 9999999LL);
  graph.set_edge_properties(100, 200, knows_buf, SNBKnowsSchema::TOTAL_SIZE);

  prop_blob pb = graph.get_edge_properties(100, 200);
  assert(pb.data != nullptr);
  assert(pb.size == SNBKnowsSchema::TOTAL_SIZE);
  assert(SNBKnowsSchema::get_creation_date(pb.data) == 9999999LL);
  delete[] pb.data;

  // hasCreator-style edge with no properties
  graph.add_edge({.src_id = 200, .dst_id = 100, .edge_weight = 0.0}, false);
  graph.set_edge_properties(200, 100, nullptr, 0);
  pb = graph.get_edge_properties(200, 100);
  assert(pb.data == nullptr && pb.size == 0);

  fprintf(stderr, "test_embedded_edge_props: PASSED\n");
}

int main()
{
  const int THREAD_NUM = 1;
  graph_opts opts;
  opts.create_new = true;
  opts.optimize_create = false;
  // opts.is_directed = false;
  opts.is_directed = true;
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
  test_update_edge(graph, opts.is_directed);
  test_delete_node(graph, opts.is_directed);

  std::cout << "Dumping tables:" << std::endl<< std::endl;  
  std::string table_name = "edge_out";
  graph.dump_table(table_name, 200);
  table_name = "edge_in";
  graph.dump_table(table_name, 200);

  test_get_edges(graph);
  test_EdgeCursor(graph, opts.is_directed);
  test_EdgeCursor_Range(graph, opts.is_directed);
  test_InCursor(graph);
  // //! TODO: test_InCursor_Range(graph);
  test_OutCursor(graph);
  test_NodeCursor(graph);
  test_NodeCursor_Range(graph);
  test_delete_edge(graph, opts.is_directed);
  test_get_edges(graph);

  std::cout << "Dumping tables:" << std::endl<< std::endl;  
  table_name = "edge_out";
  graph.dump_table(table_name, 200);
  table_name = "edge_in";
  graph.dump_table(table_name, 200);

  std::cout << "Number of nodes in graph: " << graph.get_num_nodes() << std::endl;

  test_duplicate_additions(graph);

  tearDown(graph);
  myEngine.close_graph();
  ////////////
  // Now test for read_only mode
  opts.create_new = false;
  opts.read_only = true;
  GraphEngine roEngine(THREAD_NUM, opts);
  std::string chkpt_name;
  GraphBase *rograph = roEngine.create_ro_graph_handle(chkpt_name);
  test_ro_get_nodes(rograph);

  std::cout << "Number of nodes in RO graph: " << rograph->get_num_nodes() << std::endl;

  std::cout << "Dumping (RO)tables:" << std::endl<< std::endl;
  table_name = "edge_out";
  rograph->dump_table(table_name, 200);
  table_name = "edge_in";
  rograph->dump_table(table_name, 200);

  // try to insert a node — should fail on a read-only cursor
  int ret = rograph->add_node({.id = 1000, .in_degree = 0, .out_degree = 0}, false);
  std::cout << "return value is " << ret << " wt error string: " << wiredtiger_strerror(ret) << std::endl;
  assert(ret == ENOTSUP); // insertion on a read-only cursor is not supported

  rograph->close(false);
  roEngine.close_graph();

  // ---- Property storage tests (separate graph instance) ----
  graph_opts prop_opts;
  prop_opts.create_new = true;
  prop_opts.optimize_create = false;
  prop_opts.is_directed = true;
  prop_opts.read_optimize = true;
  prop_opts.is_weighted = false;
  prop_opts.has_node_props = true;
  prop_opts.has_edge_props = true;
  prop_opts.type = GraphType::SplitEKey;
  prop_opts.db_name = "test_split_ekey_props";
  prop_opts.db_dir = "./db";
  prop_opts.conn_config = "cache_size=1GB";
  prop_opts.stat_log = "./";

  GraphEngine propEngine(1, prop_opts);
  WT_CONNECTION *prop_conn = propEngine.get_connection();
  {
    SplitEdgeKey prop_graph(prop_opts, prop_conn);
    test_embedded_node_props(prop_graph);
    test_embedded_edge_props(prop_graph);
  }
  propEngine.close_graph();
}