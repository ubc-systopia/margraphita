#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "sample_graph.h"
#include <cassert>
#include "test_standard_graph.h"

void create_init_nodes(StandardGraph graph, bool is_directed)
{
  for (node x : SampleGraph::test_nodes){
    graph.add_node(x);
  }

  if(is_directed){
    SampleGraph::create_directed_edges();
    assert(SampleGraph::test_edges.size() == 6); //checking if directed edges got created and stored in test_edges
    
  }

  for (edge x : SampleGraph::test_edges){
    graph.add_edge(x);
  }

}


void tearDown(StandardGraph graph)
{
  graph.close();
}



void dump_node(node to_print){
  cout <<"Node ID is: \t"<< to_print.id<<endl;
  cout<<"Node in_degree is:\t"<<to_print.in_degree<<endl;
  cout<<"Node out_degree is:\t"<<to_print.out_degree<<endl;
}

void test_node_add(StandardGraph graph){
  node new_node = {
    .id = 11,
    .in_degree = 0,
    .out_degree = 0
  };
  graph.add_node(new_node);
  node found = graph.get_node(new_node.id);
  assert(new_node.id == found.id);
  assert(found.in_degree == 0);
  assert(found.out_degree == 0);
}

int main()
{

  opt_args opts;
  opts.create_new = true;
  opts.optimize_create = false;
  opts.is_directed = true;
  opts.read_optimize = true;
  opts.is_weighted = false;
  opts.db_name = "test_std";


  //Test std_graph setup
  StandardGraph graph = StandardGraph(opts);
  
  create_init_nodes(graph, true);

  //Test adding a node
  test_node_add(graph);

  //Test deleting a node
 //test_node_delete();
  
  //Test std_graph teardown
  tearDown(graph);
  
 // //Test delete node without edges <-- WHY?
  // test_delete_node_without_edges();
  // //Test get_node
  // test_get_node()
  // //Test get_node for node that doesn't exist
  // test_get_node() // node_id 9
  // //Test update node
  // test_update_node()

  // test_set_node_data_int_index0();
  // test_set_node_data_float_index0();
  // test_set_node_data_int_index1();
  // test_set_node_data_float_index1();


  // check_node_data();
  //test common util packing and unpacking features.
  // test_stringvec_packing();
  // test_intvec_packing();

  return 0;
}
