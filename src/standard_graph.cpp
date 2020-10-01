#include "common.h"
#include "graph_exception.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;
const std::string GRAPH_PREFIX = "std";

class StandardGraph {
public:
  bool create_new = true;
  bool read_optimize = true;
  bool is_directed = true;
  vector<string> node_value_cols;
  int node_attr_size = 0; // set on checking the list len
  string node_value_format;

  vector<string> edge_table_columns;
  vector<string> edge_columns;
  vector<string> edge_value_cols;
  bool has_edge_attrs;
  string edge_value_format;
  string node_key_format;
  string db_name;

  StandardGraph(bool create_new, bool read_optimize, bool is_directed,
                std::string db_name,
                std::unordered_map<string, vector<string>>);

  // WT privates
private:
  WT_CONNECTION *conn;
  WT_CURSOR *cursor;
  WT_SESSION *session;
  int edge_id;
  vector<string> node_columns;

  // Basic WT session management funcs
private:
  // Now to create and delete the graph
  void create_new_graph();
  void insert_metadata(string col_name, string fmt, vector<string>);

  //Internal cursor methods
  int _get_table_cursor(WT_SESSION *session, WT_CURSOR **cursor,
                                    const char *table);

};

StandardGraph::StandardGraph(
    bool create_new, bool read_optimize, bool is_directed, std::string db_name,
    std::unordered_map<std::string, vector<string>> params)

{
  int ret; // general purpose hardy error int
  this->create_new = create_new;
  this->read_optimize = read_optimize;
  this->is_directed = is_directed;
  this->db_name = db_name;

  // Check which params are missing.
  try {
    CommonUtil::check_graph_params(params);
  } catch (GraphException G) {
    std::cout << G.what() << std::endl;
  }

  // Find NODE_VALUE_COLUMNS in params
  auto find = params.find(NODE_VALUE_COLUMNS);
  {
    if (find != params.end()) {
      node_value_cols = find->second;
    }
  }

  // Find NODE_VALUE_FORMAT in params
  find = params.find(NODE_VALUE_FORMAT);
  {
    if (find != params.end()) {
      node_value_format = find->second.front();
    }
  }

  // Find EDGE_VALUE_COLUMNS in params
  find = params.find(EDGE_VALUE_COLUMNS);
  {
    if (find != params.end()) {
      edge_value_cols = find->second;
    }
  }

  // Find EGDE_VALUE_FORMAT in params
  find = params.find(EDGE_VALUE_FORMAT);
  {
    if (find != params.end()) {
      edge_value_format = find->second.front();
    }
  }

  // Create new directory for WT DB
  std::filesystem::path dirname = "./db/" + db_name;
  if (std::filesystem::exists(dirname)) {
    filesystem::remove_all(dirname); // remove if exists;
  }
  std::filesystem::create_directory(dirname);

  // open connection to WT
  if (CommonUtil::open_connection((char *)dirname.c_str(), &conn) < 0) {
    exit(-1);
  };

  // Open a session handle for the database
  if (CommonUtil::open_session(conn, &session) != 0) {
    exit(-1);
  }

  // Initialize edge ID for the edge table
  edge_id = 1;

  // Set up the node table
  node_columns = {ID};
  node_key_format = 'I';

  // Node Column Format: <attributes><data0><data1><in_degree><out_degree>
  if (!node_value_cols.empty()) {
    node_attr_size = node_value_format.length(); // 0 if the list len=0
    node_columns.insert(node_columns.end(), node_value_cols.begin(),
                        node_value_cols.end());
  }
  // Add Data Column (packign fmt='I', int)
  node_columns.push_back(NODE_DATA + string("0"));
  node_columns.push_back(NODE_DATA + string("1"));
  node_value_format += "SS";

  // If the graph is read_optimized, add columns and format for in/out degrees
  if (read_optimize) {
    vector<string> READ_OPTIMIZE_COLS = {IN_DEGREE, OUT_DEGREE};
    node_columns.insert(node_columns.end(), READ_OPTIMIZE_COLS.begin(),
                        node_value_cols.end());
    node_value_format += "II";
  }
  // Now Create the Node Table
  CommonUtil::set_table(session, NODE_TABLE, node_columns, node_key_format,
                        node_value_format);

  // ******** Now set up the Edge Table     **************
  // Edge Column Format : <src><dst><attrs>

  edge_columns = {SRC, DST};
  string edge_key_format = "I";
  if (edge_value_cols.size() == 0) {
    edge_value_cols.push_back("n/a");
    edge_value_format += "I";
    has_edge_attrs = false;
  } else {
    edge_columns.insert(edge_columns.end(), edge_value_cols.begin(),
                        edge_value_cols.end());
    edge_value_format += "I";
    has_edge_attrs = true;
  }
  edge_value_format += "II";
  edge_table_columns = {ID};
  edge_table_columns.insert(edge_table_columns.end(), edge_columns.begin(),
                            edge_columns.end());

  // Create edge table
  CommonUtil::set_table(session, EDGE_TABLE, edge_table_columns,
                        edge_key_format, edge_value_format);

  // Create table index on src, dst
  string edge_table_index_str, edge_table_index_conf_str;
  edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_DST_INDEX;
  edge_table_index_conf_str = "columns=(" + SRC + "," + DST + ")";
  if ((ret = session->create(session, edge_table_index_str.c_str(),
                             edge_table_index_conf_str.c_str())) > 0) {
    cerr << "Failed to create index SRC_DST_INDEX in the edge table"
         << cursor->session->strerror(cursor->session, ret) << endl;
  }
  // Create index on SRC column
  edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
  edge_table_index_conf_str = "columns=(" + SRC + ")";
  if ((ret = session->create(session, edge_table_index_str.c_str(),
                             edge_table_index_conf_str.c_str())) > 0) {
    cerr << "Failed to create index SRC_INDEX in the edge table"
         << cursor->session->strerror(cursor->session, ret) << endl;
  }
  // Create index on DST column
  edge_table_index_str = "index:" + EDGE_TABLE + ":" + DST_INDEX;
  edge_table_index_conf_str = "columns=(" + DST + ")";
  if ((ret = session->create(session, edge_table_index_str.c_str(),
                             edge_table_index_conf_str.c_str())) > 0) {
    cerr << "Failed to create index DST_INDEX in the edge table"
         << cursor->session->strerror(cursor->session, ret) << endl;
  }

  /* Now doing the metadata table creation. //TODO: move to a base class
     function This table stores the graph metadata
     value_format:raw byte string (Su)
     key_format: string (S)
  */
  string metadata_table_name = "table:" + string(METADATA);
  if ((ret = session->create(session, metadata_table_name.c_str(),
                            "key_format=S,value_format=Su") ) > 0) {
    cerr << "Failed to create the metadata table "
         << cursor->session->strerror(cursor->session, ret) << endl;
  }
  // NODE_COLUMNS:
}

// void StandardGraph::insert_metadata(string col_name, string fmt,   )



int _get_table_cursor(WT_SESSION *session, WT_CURSOR **cursor,
                                    const char *table) {
  std::string table_name = "table:" + string(table);
  if (int ret = session->open_cursor(session, table_name.c_str(), NULL, NULL,
                                     cursor) != 0) {
    fprintf(stderr, "Failed to get table cursor to %s\n", table_name.c_str());
    return ret;
  }
  return 0;
}

int _get_index_cursor(WT_SESSION *session, WT_CURSOR **cursor,
                                    std::string table_name,
                                    std::string idx_name) {
  std::string index_name = "index:" + table_name + idx_name + "(id,src, dst)";
  if (int ret = session->open_cursor(session, index_name.c_str(), NULL, NULL,
                                     cursor) != 0) {
    fprintf(stderr, "Failed to open the cursor on the index %s on table %s \n",
            index_name.c_str(), table_name.c_str());
    return ret;
  }
  return 0;
}

int close(WT_CONNECTION *conn){
  /*
  Closes the graph database. This must be called by user before
  application terminates, otherwise WT will return resource busy when graph
  is accessed later.
  # STANDARD GRAPH ONLY: save current edge_id to metadata before closing.
  # The edge_id is auto_incremented for each edge inserted, so this 
  # value needs to be saved when reloading a standard graph
  */  
  //self.__insert_metadata(EDGE_ID, 'I', pack('I', self.edge_id)) TODO:
  CommonUtil::close_connection(conn);
  return 0;
}