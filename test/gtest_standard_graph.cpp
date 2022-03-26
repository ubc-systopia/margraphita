#include <cassert>

#include "common.h"
#include "graph_exception.h"
#include "gtest/gtest.h"
#include "standard_graph.h"

// WT_CONNECTION *conn;
// WT_CURSOR *cursor;
// WT_SESSION *session;
// const char *home;

bool read_optimize = true;
bool is_directed = false;

// class StandardGraphTest : public ::testing::Test
// {
// protected:

//   void SetUp() override
//   {
//     opts.create_new = true;
//     opts.node_value_columns = {"attr"};
//     opts.node_value_format = "S"; //<- This should be S because we only save
//     string converted ints. This shall remain so until I learn generics
//     opts.edge_value_columns = {"w1", "w2"};
//     opts.edge_value_format = "SS"; //<- Again, we save only strings
//     opts.optimize_create = false;

//     graph = StandardGraph(opts.create_new, read_optimize,
//                           is_directed, "test_std", opts);

//   }

//   opt_args opts;
//   StandardGraph graph;
// };

// TEST_F(StandardGraphTest, StdGraphAddNode)
// {
//  node new_node = {
//     .id = 11,
//     .attr = {"22"},
//     .data = {},
//     .in_degree = 0,
//     .out_degree = 0
//   };
//   graph.add_node(new_node);
//   node found = graph.get_node(new_node.id);
//   ASSERT_EQ(new_node.id, found.id);
//   ASSERT_EQ(new_node.attr, found.attr);
//   ASSERT_EQ(found.in_degree, 0);
//   ASSERT_EQ(found.out_degree, 0);
// }

// TEST_F(StandardGraphTest, StdGraphDeleteNode)
// {

// }

// int main(int argc, char **argv)
// {
//   ::testing::InitGoogleTest(&argc, argv);
//   return RUN_ALL_TESTS();
// }