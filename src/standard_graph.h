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
  //,std::unordered_map<string, vector<string>>);

  // WT privates
private:
  WT_CONNECTION *conn;
  WT_SESSION *session;
  int edge_id;

  int node_attr_size = 0;   // set on checking the list len
  string node_value_format; //? <- check if this is ever used again.
  string node_attr_format;
  string edge_attr_format;

  vector<string> edge_table_columns;
  vector<string> edge_columns;
  vector<string> edge_value_cols;
  vector<string> node_columns;
  vector<string> node_value_cols;
  bool has_edge_attrs;
  bool has_node_attrs;
  string edge_value_format;
  string node_key_format;

  WT_CURSOR *node_cursor = NULL;
  WT_CURSOR *random_node_cursor = NULL;
  WT_CURSOR *edge_cursor = NULL;
  WT_CURSOR *src_dst_index_cursor = NULL;
  WT_CURSOR *src_index_cursor = NULL;
  WT_CURSOR *dst_index_cursor = NULL;
  WT_CURSOR *metadata_cursor = NULL;
  // Basic WT session management funcs
  // Now to create and delete the graph
  void create_new_graph();
  void insert_metadata(string key, string value_format, char *value);
  string get_metadata(string key);
  // Internal cursor methods
  int _get_table_cursor(string table, WT_CURSOR *cursor, bool is_random);
  //   int _get_index_cursor(std::string table_name, std::string idx_name,
  //                         std::string projection);
  int _get_index_cursor(std::string table_name, std::string idx_name,
                        std::string projection, WT_CURSOR *cursor);
  void __restore_from_db(string db_name);
  void create_indices();
  void drop_indices();
  void _close();
};

#endif