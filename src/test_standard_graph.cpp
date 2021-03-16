#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "sample_graph.h"
#include <cassert>
#include "test_standard_graph.h"

void create_init_nodes(StandardGraph graph)
{
  for (node x : SampleGraph::test_nodes){
    graph.add_node(x);
  }

  if(graph.is_directed){
    SampleGraph::create_directed_edges();
    assert(SampleGraph::test_edges.size() == 6); //checking if directed edges got created and stored in test_edges
    
  }

  for (edge x : SampleGraph::test_edges){
    graph.add_edge(x);
  }

}

StandardGraph setup(opt_args opts, bool read_optimize, bool is_directed)
{

  StandardGraph graph = StandardGraph(opts.create_new, read_optimize,
                                      is_directed, "test_std", opts);
  create_init_nodes(graph);
  return graph;
}

void tearDown(StandardGraph graph)
{
  graph.close();
}



void dump_node(node to_print){
  cout <<"Node ID is: \t"<< to_print.id<<endl;
  cout <<"Node attributes are:\t"<<endl;
  cout <<"{"<<endl;
  for(string x : to_print.attr){
    cout << "\t\t\t"<<x<<endl;
  }
  cout << "}"<<endl;

  cout<<"Node data is:\t"<<endl;
  cout<<"{"<<endl;
  for(string x : to_print.data){
    cout << "\t\t\t"<<x<<endl;
  }
  cout << "}"<<endl;
  cout<<"Node in_degree is:\t"<<to_print.in_degree<<endl;
  cout<<"Node out_degree is:\t"<<to_print.out_degree<<endl;
}

void test_node_add(StandardGraph graph){
  node new_node = {
    .id = 11,
    .attr = {"22"},
    .data = {},
    .in_degree = 0,
    .out_degree = 0
  };
  graph.add_node(new_node);
  node found = graph.get_node(new_node.id);
  assert(new_node.id == found.id);
  assert(new_node.attr == found.attr);
  assert(found.in_degree == 0);
  assert(found.out_degree == 0);
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
