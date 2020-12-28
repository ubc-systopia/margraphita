#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include <cassert>

WT_CONNECTION *conn;
WT_CURSOR *cursor;
WT_SESSION *session;
const char *home;

StandardGraph setup(opt_args opts, bool read_optimize, bool is_directed)
{

  StandardGraph graph = StandardGraph(opts.create_new, read_optimize,
                                      is_directed, "test_std", opts);
  return graph;
}

void tearDown(StandardGraph graph)
{
  graph.close();
}

void test_stringvec_packing()
{
  string testname = "String Vector Packing";
  int ret = 0;
  wiredtiger_open(home, NULL, "create", &conn);
  conn->open_session(conn, NULL, NULL, &session);

  vector<string> test = {"hello", "world", "this", "is", "test", "string", "packing"};

  size_t size;
  string format;
  char *buffer = CommonUtil::pack_string_vector(test, session, &size, &format);

  vector<string> result = CommonUtil::unpack_string_vector(buffer, session);
  assert(result == test);
  cout << "TEST:\t" << testname << ":\tpass" << endl;
  conn->close(conn, NULL);
}

void test_intvec_packing()
{
  string testname = "Integer Vector Packing";
  int ret = 0;
  wiredtiger_open(home, NULL, "create", &conn);
  conn->open_session(conn, NULL, NULL, &session);

  vector<int> test = {1, 2, 3, 4, 55, 66, 77, 888, 999};
  size_t size;
  string format;
  char *buffer = CommonUtil::pack_int_vector(test, session, &size, &format);
  vector<int> result = CommonUtil::unpack_int_vector(buffer, session);

  assert(result == test);
  cout << "TEST:\t" << testname << ":\tpass" << endl;
  ;
  conn->close(conn, NULL);
}

void test_node_add(StandardGraph graph){
  node new_node = {
    .id = 11,
    .attr = {"22"}
  };

}

int main()
{

  opt_args opts;
  opts.create_new = true;
  opts.node_value_columns = {"attr"};
  opts.node_value_format = "S"; //<- This should be S because we only save string converted ints. This shall remain so until I learn generics
  opts.edge_value_columns = {"w1", "w2"};
  opts.edge_value_format = "SS"; //<- Again, we save only strings
  opts.optimize_create = false;

  //Test std_graph setup
  StandardGraph graph = setup(opts, true, true);

  //Test std_graph teardown
  tearDown(graph);

  //Test adding a node
  test_node_add(graph);
  //Test deleting a node
  test_node_delete();
  //Test delete node without edges <-- WHY?
  test_delete_node_without_edges();
  //Test get_node
  test_get_node()
  //Test get_node for node that doesn't exist
  test_get_node() // node_id 9
  //Test update node
  test_update_node()

  test_det_node_data_int_index0();
  test_set_node_data_float_index0();
  test_set_node_data_int_index1();
  test_set_node_data_float_index1();


  check_node_data();
  //test common util packing and unpacking features.
  test_stringvec_packing();
  test_intvec_packing();

  return 0;
}