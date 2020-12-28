#ifndef STD_GRAPH
#define STD_GRAPH

#include "common.h"
#include "graph_exception.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;

class StandardGraph {
public:
  bool create_new = true;
  bool read_optimize = true;
  bool is_directed = true;

  string db_name;

  StandardGraph(bool create_new, bool read_optimize, bool is_directed,
                std::string db_name, opt_args opt_params);
  StandardGraph();

  void create_new_graph();
  void add_node(node to_insert);
  bool has_node(int node_id);
  bool has_edge(int src_id, int dst_id);
  int get_edge_id(int src_id, int dst_id);
  int get_num_nodes();
  int get_num_edges();
  node get_node(int node_id);
  node get_random_node();
  void delete_node(int node_id);
  void delete_related_edges(WT_CURSOR *index_cursor, int node_id);
  int get_in_degree(int node_id);
  int get_out_degree(int node_id);
  std::vector<node> get_nodes();
  void add_edge(edge to_insert);
  void delete_edge(int src_id, int dst_id);
  void update_node_degree(WT_CURSOR *cursor, int node_id, int in_degree,
                          int out_degree);
  edge get_edge(int src_id, int dst_id);
  // void update_edge(int src_id, int dst_id, char *new_attrs); <-not needed.
  // skip.
  std::vector<edge> get_edges();
  std::vector<edge> get_out_edges(int node_id);
  std::vector<node> get_out_nodes(int node_id);
  std::vector<edge> get_in_edges(int node_id);
  std::vector<node> get_in_nodes(int node_id);
  void set_node_data(int node_id, int idx, string data);
  std::string get_node_data(int node_id, int idx);
  void get_node_iter();
  void get_edge_iter();
  // Metadata operations:
  void insert_metadata(string key, string value_format, char *value);
  string get_metadata(string key);
    void close();


  // WT privates
private:
  WT_CONNECTION *conn;
  WT_SESSION *session;
  int edge_id;
  int node_attr_size = 0; // set on checking the list len
  string node_value_format;
  string node_attr_format;
  string edge_value_format;
  string edge_attr_format;
  vector<string> edge_table_columns;
  vector<string> edge_columns;
  vector<string> edge_value_cols;
  vector<string> node_columns;
  vector<string> node_value_cols;
  bool has_edge_attrs;
  bool has_node_attrs;
  string node_key_format;
  WT_CURSOR *node_cursor = NULL;
  WT_CURSOR *random_node_cursor = NULL;
  WT_CURSOR *edge_cursor = NULL;
  WT_CURSOR *src_dst_index_cursor = NULL;
  WT_CURSOR *src_index_cursor = NULL;
  WT_CURSOR *dst_index_cursor = NULL;
  WT_CURSOR *metadata_cursor = NULL;

  void __node_to_record(WT_CURSOR *cursor, node to_insert);
  node __record_to_node(WT_CURSOR *cursor);
  edge __record_to_edge(WT_CURSOR *cursor);
  void __read_from_edge_idx(WT_CURSOR *cursor, edge_index *e_idx);

  // Internal cursor methods
  int _get_table_cursor(string table, WT_CURSOR **cursor, bool is_random);
  int _get_index_cursor(std::string table_name, std::string idx_name,
                        std::string projection, WT_CURSOR **cursor);
  void __restore_from_db(string db_name);
  void create_indices();
  void drop_indices();
};

#endif