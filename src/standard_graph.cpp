#include "standard_graph.h"
#include "common.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <wiredtiger.h>
using namespace std;
const std::string GRAPH_PREFIX = "std";

StandardGraph::StandardGraph(bool create_new, bool read_optimize,
                             bool is_directed, std::string db_name,
                             opt_args opt_params)

{
  int ret; // general purpose hardy error int
  this->create_new = create_new;
  this->read_optimize = read_optimize;
  this->is_directed = is_directed;
  this->db_name = db_name;

  // Check which params are missing.
  try {
    CommonUtil::check_graph_params(opt_params);
  } catch (GraphException G) {
    std::cout << G.what() << std::endl;
  }

  // Find NODE_VALUE_COLUMNS in params
  if (!opt_params.node_value_columns.empty()) {
    this->node_value_cols = opt_params.node_value_columns;
  }
  // Find NODE_VALUE_FORMAT in params
  if (!opt_params.node_value_format.empty()) {
    this->node_value_format = opt_params.node_value_format;
  }

  // Find EDGE_VALUE_COLUMNS in params
  if (!opt_params.edge_value_columns.empty()) {
    this->edge_value_cols = opt_params.edge_value_columns;
  }
  // Find EGDE_VALUE_FORMAT in params
  if (!opt_params.edge_value_format.empty()) {
    this->edge_value_format = opt_params.edge_value_format;
  }
  
  if(opt_params.create_new == false){
    throw GraphException("Missing param " + CREATE_NEW);
  }
  if(opt_params.create_new){
    create_new_graph();
    if(opt_params.optimize_create == false){
      /**
       *  Create indices with graph.
       * NOTE: If user opts to optimize graph creation, then they're
       * responsible for calling create_indices() AFTER all the
       * nodes/edges have been added to graph.
       * This is done to improve performance of bulk-loading.
       */
      create_indices();
    }
  }else{
    __restore_from_db(db_name);
  }
}

void StandardGraph::create_new_graph() {
  int ret;
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
    // I think this should also append the node_value_format string in case
    // of a non-empty list of attrs.
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
    fprintf(stderr, "Failed to create index SRC_DST_INDEX in the edge table");
  }
  // Create index on SRC column
  edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
  edge_table_index_conf_str = "columns=(" + SRC + ")";
  if ((ret = session->create(session, edge_table_index_str.c_str(),
                             edge_table_index_conf_str.c_str())) > 0) {
    fprintf(stderr, "Failed to create index SRC_INDEX in the edge table");
  }
  // Create index on DST column
  edge_table_index_str = "index:" + EDGE_TABLE + ":" + DST_INDEX;
  edge_table_index_conf_str = "columns=(" + DST + ")";
  if ((ret = session->create(session, edge_table_index_str.c_str(),
                             edge_table_index_conf_str.c_str())) > 0) {
    fprintf(stderr, "Failed to create index DST_INDEX in the edge table");
  }

  /* Now doing the metadata table creation. //TODO:
     function This table stores the graph metadata
     value_format:raw byte string (Su)
     key_format: string (S)
  */
  string metadata_table_name = "table:" + string(METADATA);
  if ((ret = session->create(session, metadata_table_name.c_str(),
                             "key_format=S,value_format=Su")) > 0) {
    fprintf(stderr, "Failed to create the metadata table ");
  }
  // NODE_COLUMNS
  string node_columns_fmt;
  size_t node_columns_packed_buf_size;
  char *node_columns_packed = CommonUtil::pack_string_vector(
      node_columns, session, &node_columns_packed_buf_size, &node_columns_fmt);

  insert_metadata(NODE_COLUMNS, node_columns_fmt, node_columns_packed);

  // NODE_ATTR_FORMAT
  string node_attr_format_fmt;
  size_t node_attr_format_size;
  char *node_attr_packed =
      CommonUtil::pack_string(node_attr_format, session, &node_attr_format_fmt);
  insert_metadata(NODE_ATTR_FORMAT, node_attr_format_fmt, node_attr_packed);

  // EDGE_COLUMNS
  string edge_columns_fmt;
  size_t edge_columns_packed_buf_size;
  char *edge_columns_packed = CommonUtil::pack_string_vector(
      edge_columns, session, &edge_columns_packed_buf_size, &edge_columns_fmt);
  insert_metadata(EDGE_COLUMNS, edge_columns_fmt, edge_columns_packed);

  // EDGE_ATTR_FORMAT
  string edge_attr_format_fmt;
  size_t edge_attr_format_size;
  char *edge_attr_packed =
      CommonUtil::pack_string(edge_attr_format, session, &edge_attr_format_fmt);
  insert_metadata(EDGE_ATTR_FORMAT, edge_attr_format_fmt, edge_attr_packed);

  // HAS_NODE_ATTR
  string has_node_attr_fmt;
  size_t has_node_attr_size;
  char *has_node_attr_packed =
      CommonUtil::pack_bool(has_node_attrs, session, &has_node_attr_fmt);
  insert_metadata(HAS_NODE_ATTR, has_node_attr_fmt, has_node_attr_packed);

  // HAS_EDGE_ATTR
  string has_edge_attr_fmt;
  size_t has_edge_attr_size;
  char *has_edge_attrs_packed =
      CommonUtil::pack_bool(has_edge_attrs, session, &has_edge_attr_fmt);
  insert_metadata(HAS_EDGE_ATTR, has_edge_attr_fmt, has_edge_attrs_packed);

  // DB_NAME
  string db_name_fmt;
  size_t db_name_packed_size;
  char *db_name_packed =
      CommonUtil::pack_string(db_name, session, &db_name_fmt);
  insert_metadata(DB_NAME, db_name_fmt, db_name_packed);

  // READ_OPTIMIZE
  string is_read_optimized_fmt;
  size_t is_read_optimized_size;
  char *is_read_optimized_packed =
      CommonUtil::pack_bool(read_optimize, session, &is_read_optimized_fmt);
  insert_metadata(READ_OPTIMIZE, is_read_optimized_fmt,
                  is_read_optimized_packed);
  // EDGE_ID
  string edge_id_fmt;
  size_t edge_id_packed_size;
  char *edge_id_packed = CommonUtil::pack_int(edge_id, session);
  insert_metadata(EDGE_ID, "I", edge_id_packed); // single int fmt is "I"

  // IS_DIRECTED
  string is_directed_fmt;
  size_t is_directed_size;
  char *is_directed_packed =
      CommonUtil::pack_bool(is_directed, session, &is_directed_fmt);
  insert_metadata(IS_DIRECTED, is_directed_fmt, is_directed_packed);

  /**
   * Create the <SRC,DST> index on the table
   */
  string table_name = "index:" + EDGE_TABLE + ":" + SRC_DST_INDEX;
  string cols = "columns=(" + SRC + "," + DST + ")";
  session->create(session, table_name.c_str(), cols.c_str());
}

/**
 * @brief This private function inserts metadata values into the metadata
 * table. The fields are self explanatory.
 */
void StandardGraph::insert_metadata(string key, string value_format,
                                    char *value) {

  string table_name = "table:" + METADATA;
  int ret = 0;
  if (this->metadata_cursor == NULL) {
    if ((ret = _get_table_cursor(table_name, metadata_cursor, false)) != 0) {
      fprintf(stderr, "Failed to create cursor to the metadata table.");
      exit(-1);
    }
  }

  metadata_cursor->set_key(metadata_cursor, key.c_str());
  metadata_cursor->set_value(metadata_cursor, value);
  if ((ret = metadata_cursor->insert(metadata_cursor)) != 0) {
    fprintf(stderr, "failed to insert metadata for key %s", key.c_str());
    // TODO: Maybe create a GraphException?
  }
  metadata_cursor->close(metadata_cursor); //? Should I close this?
}

/**
 * @brief Returns the metadata associated with the key param from the METADATA
 * table.
 */
string StandardGraph::get_metadata(string key) {
  string table_name = "table:" + METADATA;
  int ret = 0;
  if (this->metadata_cursor == NULL) {
    if ((ret = _get_table_cursor(table_name, metadata_cursor, false)) != 0) {
      fprintf(stderr, "Failed to create cursor to the metadata table.");
      exit(-1);
    }
  }
  metadata_cursor->set_key(metadata_cursor, key.c_str());
  ret = metadata_cursor->search(metadata_cursor);
  if (ret != 0) {
    fprintf(stderr, "Failed to retrieve metadata for the key %s", key.c_str());
    exit(-1);
  }

  const char *value;
  ret = metadata_cursor->get_value(metadata_cursor, &value);
  metadata_cursor->close(metadata_cursor); //? Should I close this?

  return string(value);
}

/**
 * @brief This is the generic function to get a cursor on the table
 *
 * @param table This is the table name for which the cursor is needed.
 * @param cursor This is the pointer that will hold the set cursor.
 * @param is_random This is a bool value to indicate if the cursor must be
 * random.
 * @return 0 if the cursor could be set
 */
int StandardGraph::_get_table_cursor(string table, WT_CURSOR *cursor,
                                     bool is_random) {
  std::string table_name = "table:" + table;
  const char *config = NULL;
  if (is_random) {
    config = "next_random=true";
  }
  if (int ret = session->open_cursor(session, table_name.c_str(), NULL, config,
                                     &cursor) != 0) {
    fprintf(stderr, "Failed to get table cursor to %s\n", table_name.c_str());
    return ret;
  }
  return 0;
}

/**
 * @brief Generic function to create the indexes on a table
 *
 * @param table_name The name of the table on which the index is to be created.
 * @param idx_name The name of the index
 * @param projection The columns that are to be included in the index. This is
 * in the format "(col1,col2,..)"
 * @param cursor This is the cursor variable that needs to be set.
 * @return 0 if the index could be set
 */
int StandardGraph::_get_index_cursor(std::string table_name,
                                     std::string idx_name,
                                     std::string projection,
                                     WT_CURSOR *cursor) {
  std::string index_name = "index:" + table_name + idx_name + projection;
  if (int ret = session->open_cursor(session, index_name.c_str(), NULL, NULL,
                                     &cursor) != 0) {
    fprintf(stderr, "Failed to open the cursor on the index %s on table %s \n",
            index_name.c_str(), table_name.c_str());
    return ret;
  }
  return 0;
}

/**
 * @brief This function is used to restore the graph configs from the saved
 *  attrs in the METADATA table.
 *
 * @param db_name
 */
void StandardGraph::__restore_from_db(std::string db_name) {

  int ret = CommonUtil::open_connection(strdup(db_name.c_str()), &conn);
  WT_CURSOR *cursor;

  ret = CommonUtil::open_session(conn, &session);
  const char *key, *value;
  ret = _get_table_cursor(("table:" + METADATA).c_str(), cursor, false);

  while ((ret = cursor->next(cursor)) == 0) {
    ret = cursor->get_key(cursor, &key);
    ret = cursor->get_value(cursor, &value);

    if (strcmp(key, NODE_COLUMNS.c_str()) == 0) {
      this->node_columns =
          CommonUtil::unpack_string_vector(value, this->session);

    } else if (strcmp(key, NODE_ATTR_FORMAT.c_str()) == 0) {

      this->node_attr_format = CommonUtil::unpack_string(value, this->session);

    } else if (strcmp(key, EDGE_COLUMNS.c_str()) == 0) {

      this->edge_columns =
          CommonUtil::unpack_string_vector(value, this->session);

    } else if (strcmp(key, EDGE_ATTR_FORMAT.c_str()) == 0) {

      this->edge_attr_format = CommonUtil::unpack_string(value, this->session);

    } else if (strcmp(key, HAS_NODE_ATTR.c_str()) == 0) {

      this->has_node_attrs = CommonUtil::unpack_bool(value, this->session);

    } else if (strcmp(key, HAS_EDGE_ATTR.c_str()) == 0) {

      this->has_edge_attrs = CommonUtil::unpack_bool(value, this->session);

    } else if (strcmp(key, DB_NAME.c_str()) == 0) {

      this->db_name = CommonUtil::unpack_string(value, this->session);

    } else if (strcmp(key, READ_OPTIMIZE.c_str()) == 0) {

      this->read_optimize = CommonUtil::unpack_bool(value, this->session);

    } else if (strcmp(key, EDGE_ID.c_str()) == 0) {

      this->edge_id = CommonUtil::unpack_int(value, this->session);

    } else if (strcmp(key, IS_DIRECTED.c_str()) == 0) {

      this->is_directed = CommonUtil::unpack_bool(value, this->session);
    }
  }
  cursor->close(cursor);
}

/**
 * @brief Creates the indices not required for adding nodes/edges.
 * Used for optimized bulk loading: User should call drop_indices() before
 * bulk loading nodes/edges. Once nodes/edges are added, user must
 * call create_indices() for the graph API to work.
 *
 * This method requires exclusive access to the specified data source(s). If
 * any cursors are open with the specified name(s) or a data source is
 * otherwise in use, the call will fail and return EBUSY.
 */
void StandardGraph::create_indices() {
  CommonUtil::close_cursor(this->edge_cursor);
  CommonUtil::close_cursor(this->src_dst_index_cursor);
  CommonUtil::close_cursor(this->src_index_cursor);
  CommonUtil::close_cursor(this->dst_index_cursor);

  // Create index for (src) on the edge table
  string uri = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
  string config = "columns=(" + SRC + ")";
  this->session->create(this->session, uri.c_str(), config.c_str());

  // Create index on(dst) on Edge Table
  uri = "index:" + EDGE_TABLE + ":" + DST_INDEX;
  config = "columns=(" + DST + ")";
  this->session->create(this->session, uri.c_str(), config.c_str());
}
/**
 * @brief Drops the indices not required for adding nodes/edges.
 * Used for optimized bulk loading: User should call drop_indices() before
 * bulk loading nodes/edges. Once nodes/edges are added, user must
 * call create_indices() for the graph API to work.
 *
 * This method requires exclusive access to the specified data source(s). If
 * any cursors are open with the specified name(s) or a data source is
 * otherwise in use, the call will fail and return EBUSY.
 */
void StandardGraph::drop_indices() {
  CommonUtil::close_cursor(this->edge_cursor);
  CommonUtil::close_cursor(this->src_dst_index_cursor);
  CommonUtil::close_cursor(this->src_index_cursor);
  CommonUtil::close_cursor(this->dst_index_cursor);

  // Drop (src) index on edge table
  string uri = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
  this->session->drop(this->session, uri.c_str(), NULL);

  // Drop (dst) index on the edge table
  uri = "index:" + EDGE_TABLE + ":" + DST_INDEX;
  this->session->drop(this->session, uri.c_str(), NULL);
}

void StandardGraph::_close() {
  string edge_id_fmt;
  size_t edge_id_packed_size;
  char *edge_id_packed = CommonUtil::pack_int(edge_id, session);
  insert_metadata(EDGE_ID, "I", edge_id_packed); // single int fmt is "I"
  CommonUtil::close_connection(conn);
}

int main() { printf("Trying this out\n"); }