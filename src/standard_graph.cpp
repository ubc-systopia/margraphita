#include "standard_graph.h"
#include "common.h"
#include <filesystem>
#include <cstring>
#include <cassert>
#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
#include <unordered_map>
#include <wiredtiger.h>
using namespace std;
const std::string GRAPH_PREFIX = "std";

StandardGraph::StandardGraph(){};

StandardGraph::StandardGraph(bool create_new, bool read_optimize,
                             bool is_directed, std::string db_name,
                             opt_args opt_params)

{
  this->create_new = create_new;
  this->read_optimize = read_optimize;
  this->is_directed = is_directed;
  this->db_name = db_name;

  // Check which params are missing.
  try
  {
    CommonUtil::check_graph_params(opt_params);
  }
  catch (GraphException G)
  {
    std::cout << G.what() << std::endl;
  }

  // Find NODE_VALUE_COLUMNS in params
  if (!opt_params.node_value_columns.empty())
  {
    this->node_value_cols = opt_params.node_value_columns;
  }
  // Find NODE_VALUE_FORMAT in params
  if (!opt_params.node_value_format.empty())
  {
    this->node_value_format = opt_params.node_value_format;
  }

  // Find NODE_ATTR_FORMAT in params
  if (!opt_params.node_value_format.empty())
  {
    this->node_attr_format = opt_params.node_value_format;
  }

  // Find EDGE_VALUE_COLUMNS in params
  if (!opt_params.edge_value_columns.empty())
  {
    this->edge_value_cols = opt_params.edge_value_columns;
  }
  // Find EGDE_VALUE_FORMAT in params
  if (!opt_params.edge_value_format.empty())
  {
    this->edge_value_format = opt_params.edge_value_format;
  }

  // Find EGDE_ATTR_FORMAT in params
  if (!opt_params.edge_value_format.empty())
  {
    this->edge_attr_format = opt_params.edge_value_format;
  }

  if (opt_params.create_new == false)
  {
    throw GraphException("Missing param " + CREATE_NEW);
  }
  if (opt_params.create_new)
  {
    create_new_graph();
    if (opt_params.optimize_create == false)
    {
      /**
       *  Create indices with graph.
       * NOTE: If user opts to optimize graph creation, then they're
       * responsible for calling create_indices() AFTER all the
       * nodes/edges have been added to graph.
       * This is done to improve performance of bulk-loading.
       */
      create_indices();
    }
  }
  else
  {
    __restore_from_db(db_name);
  }
}

void StandardGraph::create_new_graph()
{
  int ret;
  // Create new directory for WT DB
  std::filesystem::path dirname = "./db/" + db_name;
  if (std::filesystem::exists(dirname))
  {
    filesystem::remove_all(dirname); // remove if exists;
  }
  std::filesystem::create_directories(dirname);

  // open connection to WT
  if (CommonUtil::open_connection((char *)dirname.c_str(), &conn) < 0)
  {
    exit(-1);
  };

  // Open a session handle for the database
  if (CommonUtil::open_session(conn, &session) != 0)
  {
    exit(-1);
  }

  // Initialize edge ID for the edge table
  edge_id = 1;

  // Set up the node table
  node_columns = {ID};
  node_key_format = 'I';
  if (!node_value_cols.empty())
  {
    this->has_node_attrs = true;
  }
  // Node Column Format: <attributes><data0><data1><in_degree><out_degree>
  // Add the attributes to the column vector and the format string for that.
  if (!node_value_cols.empty())
  {
    node_columns.insert(node_columns.end(), node_value_cols.begin(),
                        node_value_cols.end());
  }

  // Add Data Column (packign fmt='I', int)
  node_columns.push_back(NODE_DATA + string("0"));
  node_columns.push_back(NODE_DATA + string("1"));
  node_value_format += "SS";

  // If the graph is read_optimized, add columns and format for in/out degrees
  if (read_optimize)
  {
    // vector<string> READ_OPTIMIZE_COLS = {IN_DEGREE, OUT_DEGREE};
    // node_columns.insert(node_columns.end(), READ_OPTIMIZE_COLS.begin(),
    //                     node_value_cols.end());
    node_columns.push_back(IN_DEGREE);
    node_columns.push_back(OUT_DEGREE);
    node_value_format += "II";
  }
  // Now Create the Node Table
  CommonUtil::set_table(session, NODE_TABLE, node_columns, node_key_format,
                        node_value_format);

  // ******** Now set up the Edge Table     **************
  // Edge Column Format : <src><dst><attrs>

  edge_columns = {SRC, DST};
  string edge_key_format = "I";
  //Now prepare the edge value format. starts with II for src,dst
  edge_value_format = "II";
  if (edge_value_cols.size() == 0)
  {
    // Add n/a because there are no columns. WT does not allow
    // empty value, so add integer placeholder when no column
    // is provided.
    edge_value_cols.push_back("n/a");
    edge_value_format += "S";
    has_edge_attrs = false;
  }
  else
  {
    edge_columns.insert(edge_columns.end(), edge_value_cols.begin(),
                        edge_value_cols.end());
    for (int i = 0; i < edge_value_cols.size(); i++)
    {
      edge_value_format += "S"; //Append format string for <attr> vec
    }
    has_edge_attrs = true;
  }

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
                             edge_table_index_conf_str.c_str())) > 0)
  {
    fprintf(stderr, "Failed to create index SRC_DST_INDEX in the edge table");
  }
  // Create index on SRC column
  edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
  edge_table_index_conf_str = "columns=(" + SRC + ")";
  if ((ret = session->create(session, edge_table_index_str.c_str(),
                             edge_table_index_conf_str.c_str())) > 0)
  {
    fprintf(stderr, "Failed to create index SRC_INDEX in the edge table");
  }
  // Create index on DST column
  edge_table_index_str = "index:" + EDGE_TABLE + ":" + DST_INDEX;
  edge_table_index_conf_str = "columns=(" + DST + ")";
  if ((ret = session->create(session, edge_table_index_str.c_str(),
                             edge_table_index_conf_str.c_str())) > 0)
  {
    fprintf(stderr, "Failed to create index DST_INDEX in the edge table");
  }

  /* Now doing the metadata table creation. //TODO:
     function This table stores the graph metadata
     value_format:raw byte string (Su)
     key_format: string (S)
  */
  string metadata_table_name = "table:" + string(METADATA);
  if ((ret = session->create(session, metadata_table_name.c_str(),
                             "key_format=S,value_format=S")) > 0)
  {
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
  char *edge_attr_packed =
      CommonUtil::pack_string(edge_attr_format, session, &edge_attr_format_fmt);
  insert_metadata(EDGE_ATTR_FORMAT, edge_attr_format_fmt, edge_attr_packed);

  // HAS_NODE_ATTR
  string has_node_attr_fmt;
  char *has_node_attr_packed =
      CommonUtil::pack_bool(has_node_attrs, session, &has_node_attr_fmt);
  insert_metadata(HAS_NODE_ATTR, has_node_attr_fmt, has_node_attr_packed);

  // HAS_EDGE_ATTR
  string has_edge_attr_fmt;
  char *has_edge_attrs_packed =
      CommonUtil::pack_bool(has_edge_attrs, session, &has_edge_attr_fmt);
  insert_metadata(HAS_EDGE_ATTR, has_edge_attr_fmt, has_edge_attrs_packed);

  // DB_NAME
  string db_name_fmt;
  char *db_name_packed =
      CommonUtil::pack_string(db_name, session, &db_name_fmt);
  insert_metadata(DB_NAME, db_name_fmt, db_name_packed);

  // READ_OPTIMIZE
  string is_read_optimized_fmt;
  char *is_read_optimized_packed =
      CommonUtil::pack_bool(read_optimize, session, &is_read_optimized_fmt);
  insert_metadata(READ_OPTIMIZE, is_read_optimized_fmt,
                  is_read_optimized_packed);
  // EDGE_ID
  string edge_id_fmt;
  char *edge_id_packed = CommonUtil::pack_int(edge_id, session);
  insert_metadata(EDGE_ID, "I", edge_id_packed); // single int fmt is "I"

  // IS_DIRECTED
  string is_directed_fmt;
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
                                    char *value)
{

  int ret = 0;

  if ((ret = _get_table_cursor(METADATA, &metadata_cursor, false)) != 0)
  {
    fprintf(stderr, "Failed to create cursor to the metadata table.");
    exit(-1);
  }

  this->metadata_cursor->set_key(metadata_cursor, key.c_str());
  this->metadata_cursor->set_value(metadata_cursor, value);
  if ((ret = this->metadata_cursor->insert(metadata_cursor)) != 0)
  {
    fprintf(stderr, "failed to insert metadata for key %s", key.c_str());
    // TODO: Maybe create a GraphException?
  }
  this->metadata_cursor->close(metadata_cursor); //? Should I close this?
}

/**
 * @brief Returns the metadata associated with the key param from the METADATA
 * table.
 */
string StandardGraph::get_metadata(string key)
{

  int ret = 0;
  if (this->metadata_cursor == NULL)
  {
    if ((ret = _get_table_cursor(METADATA, &metadata_cursor, false)) != 0)
    {
      fprintf(stderr, "Failed to create cursor to the metadata table.");
      exit(-1);
    }
  }
  metadata_cursor->set_key(metadata_cursor, key.c_str());
  ret = metadata_cursor->search(metadata_cursor);
  if (ret != 0)
  {
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
int StandardGraph::_get_table_cursor(string table, WT_CURSOR **cursor,
                                     bool is_random)
{
  std::string table_name = "table:" + table;
  const char *config = NULL;
  if (is_random)
  {
    config = "next_random=true";
  }
  if (int ret = session->open_cursor(session, table_name.c_str(), NULL, config,
                                     cursor) != 0)
  {
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
                                     WT_CURSOR **cursor)
{
  std::string index_name = "index:" + table_name + ":" + idx_name + projection;
  if (int ret = session->open_cursor(session, index_name.c_str(), NULL, NULL,
                                     cursor) != 0)
  {
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
void StandardGraph::__restore_from_db(std::string db_name)
{

  int ret = CommonUtil::open_connection(strdup(db_name.c_str()), &conn);
  WT_CURSOR *cursor = nullptr;

  ret = CommonUtil::open_session(conn, &session);
  const char *key, *value;
  ret = _get_table_cursor(METADATA, &cursor, false);

  while ((ret = cursor->next(cursor)) == 0)
  {
    ret = cursor->get_key(cursor, &key);
    ret = cursor->get_value(cursor, &value);

    if (strcmp(key, NODE_COLUMNS.c_str()) == 0)
    {
      this->node_columns =
          CommonUtil::unpack_string_vector(value, this->session);
    }
    else if (strcmp(key, NODE_ATTR_FORMAT.c_str()) == 0)
    {

      this->node_attr_format = CommonUtil::unpack_string(value, this->session);
    }
    else if (strcmp(key, EDGE_COLUMNS.c_str()) == 0)
    {

      this->edge_columns =
          CommonUtil::unpack_string_vector(value, this->session);
    }
    else if (strcmp(key, EDGE_ATTR_FORMAT.c_str()) == 0)
    {

      this->edge_attr_format = CommonUtil::unpack_string(value, this->session);
    }
    else if (strcmp(key, HAS_NODE_ATTR.c_str()) == 0)
    {

      this->has_node_attrs = CommonUtil::unpack_bool(value, this->session);
    }
    else if (strcmp(key, HAS_EDGE_ATTR.c_str()) == 0)
    {

      this->has_edge_attrs = CommonUtil::unpack_bool(value, this->session);
    }
    else if (strcmp(key, DB_NAME.c_str()) == 0)
    {

      this->db_name = CommonUtil::unpack_string(value, this->session);
    }
    else if (strcmp(key, READ_OPTIMIZE.c_str()) == 0)
    {

      this->read_optimize = CommonUtil::unpack_bool(value, this->session);
    }
    else if (strcmp(key, EDGE_ID.c_str()) == 0)
    {

      this->edge_id = CommonUtil::unpack_int(value, this->session);
    }
    else if (strcmp(key, IS_DIRECTED.c_str()) == 0)
    {

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
void StandardGraph::create_indices()
{
  if (this->edge_cursor != nullptr)
  {
    CommonUtil::close_cursor(this->edge_cursor);
  }
  if (this->src_dst_index_cursor != nullptr)
  {
    CommonUtil::close_cursor(this->src_dst_index_cursor);
  }
  if (this->src_index_cursor != nullptr)
  {
    CommonUtil::close_cursor(this->src_index_cursor);
  }
  if (this->dst_index_cursor!= nullptr)
  {
    CommonUtil::close_cursor(this->dst_index_cursor);
  }

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
void StandardGraph::drop_indices()
{
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

void StandardGraph::close()
{
  string edge_id_fmt;
  char *edge_id_packed = CommonUtil::pack_int(edge_id, session);
  insert_metadata(EDGE_ID, "I", edge_id_packed); // single int fmt is "I"
  CommonUtil::close_connection(conn);
}

/**
 * @brief The information that gets persisted to WT is of the form:
 * <node_id>,<attr1>..<attrN>,data0,data1,in_degree,out_degree
 * attrs are persisted if has_node_attrs is true
 * data0,data1 are strings. use std::to_string for others.
 * in_degree and out_degree are persisted if read_optimize is true. ints
 *
 *
 * @param to_insert
 */
void StandardGraph::add_node(node to_insert)
{
  int ret = 0;
  if (this->node_cursor == NULL)
  {
    ret = _get_table_cursor(NODE_TABLE, &(this->node_cursor), false);
  }
  this->node_cursor->set_key(this->node_cursor, to_insert.id);

  size_t pack_size = 0; // size for the attr vector

  if ((to_insert.attr.size() == 0) & (this->has_node_attrs == true))
  {

    // using this because I know attrs are represented as strings.
    to_insert.attr =
        CommonUtil::get_default_string_attrs(this->node_attr_format);
  }

  string packed_node_attr_fmt; // This should be equal to node_attr_format after
                               // packing.
  char *attr_list_packed = CommonUtil::pack_string_vector(
      to_insert.attr, session, &pack_size, &packed_node_attr_fmt);
  assert(packed_node_attr_fmt.compare(this->node_attr_format) ==
         0); // This is my understanding of this whole mess.

  if (this->has_node_attrs && this->read_optimize)
  {
    this->node_cursor->set_value(this->node_cursor, attr_list_packed, "0.0",
                                 "0.0", 0, 0);
  }
  else if (this->read_optimize)
  {
    this->node_cursor->set_value(this->node_cursor, "0.0", "0.0", 0, 0);
  }
  else
  {
    this->node_cursor->set_value(this->node_cursor, "0.0", "0.0");
  }

  ret = node_cursor->insert(node_cursor);

  if (ret != 0)
  {
    throw GraphException("Failed to add node_id" +
                         std::to_string(to_insert.id));
  }
}

/**
 * @brief This function is used to check if a node identified by node_id exists
 * in the node table.
 *
 * @param node_id the node_id to be searched.
 * @return true if the node is found; false otherwise.
 */
bool StandardGraph::has_node(int node_id)
{
  int ret = 0;
  if (this->node_cursor == NULL)
  {
    ret = _get_table_cursor(NODE_TABLE, &(this->node_cursor), false);
  }
  this->node_cursor->set_key(this->node_cursor, node_id);
  ret = this->node_cursor->search(this->node_cursor);
  if (ret == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * @brief This function is used to get the edge id for the edge identified by
 * <src,dst>
 *
 * @param src_id source node_id
 * @param dst_id dsetination node_id
 * @return edge_id -1 if the edge_id does not exist; edge_id if it does exists.
 */
int StandardGraph::get_edge_id(int src_id, int dst_id)
{
  int ret = 0;
  int edge_id;
  string projection = "(" + ID + "," + SRC + "," + DST + ")";
  if (this->src_dst_index_cursor == NULL)
  {
    ret = _get_index_cursor(EDGE_TABLE, SRC_DST_INDEX, projection,
                            &(this->src_dst_index_cursor));
  }
  this->src_dst_index_cursor->set_key(this->src_dst_index_cursor, src_id,
                                      dst_id);
  ret = this->src_dst_index_cursor->search(this->src_dst_index_cursor);
  if (ret != 0)
  {
    return -1;
  }
  else
  {

    ret = this->src_dst_index_cursor->get_value(this->src_dst_index_cursor,
                                                &edge_id);
    return edge_id;
  }
}

/**
 * @brief This function tests if the edge identified by <src_id, dst_id> exists
 *
 * @param src_id source node id
 * @param dst_id destination node_id
 * @return true/false for if the edge exists/does not exist
 */
bool StandardGraph::has_edge(int src_id, int dst_id)
{
  int edge_id = get_edge_id(src_id, dst_id);

  if (edge_id > 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

int StandardGraph::get_num_nodes()
{
  int ret = 0;
  WT_CURSOR *cursor = nullptr;
  ret = _get_table_cursor(NODE_TABLE, &cursor, false);

  int count = 0;
  while ((ret = cursor->next(cursor)) == 0)
  {
    count += 1;
  }
  cursor->close(cursor);
  return count;
}

int StandardGraph::get_num_edges()
{
  int ret = 0;
  WT_CURSOR *cursor = nullptr;
  ret = _get_table_cursor(EDGE_TABLE, &cursor, false);

  int count = 0;
  while ((ret = cursor->next(cursor)) == 0)
  {
    count += 1;
  }
  cursor->close(cursor);
  return count;
}

node StandardGraph::get_node(int node_id)
{
  node found;
  int ret = 0;

  WT_CURSOR *cursor = nullptr;

  ret = _get_table_cursor(NODE_TABLE, &cursor, false);
  cursor->set_key(cursor, node_id);

  ret = cursor->search(cursor);
  if (ret != 0)
  {
    throw GraphException("Could not find a node with nodeId " +
                         std::to_string(node_id));
  }
  found = __record_to_node(cursor);
  cursor->close(cursor);
  return found;
}

node StandardGraph::get_random_node()
{
  node found;
  int ret = 0;

  if (this->random_node_cursor == NULL)
  {
    ret = _get_table_cursor(NODE_TABLE, &(this->random_node_cursor), true);
  }

  ret = this->random_node_cursor->next(random_node_cursor);
  if (ret != 0)
  {
    throw GraphException("Could not find a random node");
  }

  found = __record_to_node(this->random_node_cursor);
  // random_node_cursor->close(random_node_cursor); <- don't close a cursor
  // prematurely
  return found;
}

/**
 * @brief This function is used to delete a node and all its edges.
 *
 * @param node_id
 */
void StandardGraph::delete_node(int node_id)
{
  int ret = 0;

  if (this->node_cursor == NULL)
  {
    ret = _get_table_cursor(NODE_TABLE, &(this->node_cursor), false);
    if (ret != 0)
    {
      throw GraphException("Could not get a cursor to the node table");
    }
  }
  // Delete the node with the matching node_id
  node_cursor->set_key(node_cursor, node_id);
  ret = node_cursor->remove(node_cursor);

  if (ret != 0)
  {
    throw GraphException("Could not delete node with ID " + to_string(node_id));
  }

  // Delete outgoing edges from the victim node
  // This check is probably not necessary
  if (this->src_dst_index_cursor == NULL)
  {
    string projection = "(" + ID + "," + SRC + "," + DST + ")";
    ret = _get_index_cursor(EDGE_TABLE, SRC_DST_INDEX, projection,
                            &(this->src_dst_index_cursor));
    if (ret != 0)
    {
      throw GraphException("Could not open a SRC_DST index on the edge table.");
    }
  }
  delete_related_edges(src_dst_index_cursor, node_id);

  // Delete incoming edges to the victim node
  if (this->dst_index_cursor == NULL)
  {
    string projection = "(" + DST + ")";
    ret = _get_index_cursor(EDGE_TABLE, DST_INDEX, projection,
                            &(this->dst_index_cursor));
    if (ret != 0)
    {
      throw GraphException(
          "Could not get a DST index cursor on the edge table");
    }
  }
  delete_related_edges(dst_index_cursor, node_id);
}

void StandardGraph::delete_related_edges(WT_CURSOR *index_cursor, int node_id)
{
  WT_CURSOR *edge_cursor = nullptr;
  int ret = 0;

  ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, false);
  if (ret != 0)
  {
    throw GraphException("Unable to get a cursor to the edge table.");
  }

  int edge_id, src, dst = {-1};
  index_cursor->set_key(index_cursor, node_id);
  if (index_cursor->search(index_cursor) == 0)
  {
    index_cursor->get_value(index_cursor, &edge_id, &src, &dst); //! check
    edge_cursor->set_key(edge_cursor, edge_id);
    ret = edge_cursor->remove(edge_cursor);
    if (ret != 0)
    {
      throw GraphException("Failed to delete edge (" + SRC + "," + DST + ")");
    }
    int tmp_key;
    index_cursor->get_key(index_cursor, &tmp_key);
    while (index_cursor->next(index_cursor) == 0 && tmp_key == node_id)
    {
      ret = index_cursor->get_value(index_cursor, &edge_id, &src, &dst);
      edge_cursor->set_key(edge_cursor, edge_id);
      ret = edge_cursor->remove(edge_cursor);

      if (ret != 0)
      {
        throw GraphException("Failed to delete edge (" + SRC + "," + DST + ")");
      }
    }
  }
}

/**
 * @brief This fucntion updates a node with the given attribute vector.
 * This function assumes that only the node attributes are updated here.
 * Node data is modified using get/set_node_data() and degrees are modified
 * automatically on edge insertions/deletions.
 * @param node_id The Node ID to be updated
 * @param new_attrs The new node attribute vector.
 */
void StandardGraph::__node_to_record(WT_CURSOR *cursor, node to_insert)
{

  cursor->set_key(cursor, to_insert.id);
  int search_ret = cursor->search(cursor);

  if (search_ret != 0)
  {
    throw GraphException("Failed to find a node with node_id = " +
                         to_string(to_insert.id));
  }

  // first pack the attr vector
  char *attr_list_packed = (char *)malloc(1000 * sizeof(char)); // arbitrary :(

  // Pack the new attrs;
  size_t pack_size;
  string packed_node_attr_fmt;
  char *new_attr_list_packed = CommonUtil::pack_string_vector(
      to_insert.attr, session, &pack_size, &packed_node_attr_fmt);

  // Now get the cursor value and update the attr
  if (this->has_node_attrs && this->read_optimize)
  {
    string data0 = to_insert.data.at(0);
    string data1 = to_insert.data.at(1);
    cursor->set_value(cursor, new_attr_list_packed,
                      to_insert.data.at(0).c_str(),
                      to_insert.data.at(1).c_str(), to_insert.in_degree,
                      to_insert.out_degree);
  }
  else
  {
    // cursor->get_value(cursor, attr_list_packed, to_insert.data[0],
    // to_insert.data[1]); <-- Not needed as override.
    cursor->set_value(cursor, attr_list_packed, to_insert.data.at(0).c_str(),
                      to_insert.data.at(1).c_str());
  }

  int ret = cursor->update(cursor);
  if (ret != 0)
  {
    throw GraphException("Failed to update node_id = " +
                         to_string(to_insert.id));
  }

  free(attr_list_packed);
}

int StandardGraph::get_in_degree(int node_id)
{
  if (this->read_optimize)
  {
    node found = get_node(node_id);
    return found.in_degree;
  }
  else
  {
    if (!has_node(node_id))
    {
      throw GraphException("The node with ID " + std::to_string(node_id) +
                           " does not exist.");
    }
    WT_CURSOR *cursor = nullptr;

    int ret = _get_index_cursor(EDGE_TABLE, DST_INDEX,
                                "(" + ID + "," + SRC + "," + DST + ")", &cursor);
    CommonUtil::check_return(
        ret, "Failed to get DST_INDEX cursor on the edge table");
    cursor->reset(cursor);
    cursor->set_key(cursor, node_id);
    int count = 0;

    if (cursor->search(cursor) == 0)
    {
      count++;
      int id, src, dst = 0;
      cursor->get_value(cursor, &id, &src, &dst);

      while (cursor->next(cursor) == 0 && dst == node_id)
      {
        count++;
        cursor->get_value(cursor, &id, &src, &dst);
      }
    }
    return count;
  }
}

int StandardGraph::get_out_degree(int node_id)
{
  if (read_optimize)
  {
    node found = get_node(node_id);
    return found.out_degree;
  }
  else
  {
    if (!has_node(node_id))
    {
      throw GraphException("The node with ID " + std::to_string(node_id) +
                           " does not exist");
    }
    WT_CURSOR *cursor = nullptr;

    int ret = _get_index_cursor(EDGE_TABLE, SRC_INDEX,
                                "(" + ID + "," + SRC + "," + DST + ")", &cursor);
    CommonUtil::check_return(ret, "Failed to get SRC_INDEX on the Edge table");
    cursor->reset(cursor);
    cursor->set_key(cursor, node_id);
    int count = 0;
    if (cursor->search(cursor) == 0)
    {
      count++;
      int id, src, dst = 0;
      cursor->get_value(cursor, &id, &src, &dst);
      while (cursor->next(cursor) == 0 && src == node_id)
      {
        count++;
        cursor->get_value(cursor, &id, &src, &dst);
      }
    }
    return count;
  }
}

std::vector<node> StandardGraph::get_nodes()
{
  vector<node> nodes;

  WT_CURSOR *cursor = nullptr;
  int ret = _get_table_cursor(NODE_TABLE, &cursor, false);
  while ((ret = cursor->next(cursor) == 0))
  {
    node item = __record_to_node(cursor);
    nodes.push_back(item);
  }
  return nodes;
}

node StandardGraph::__record_to_node(WT_CURSOR *cursor)
{
  node found;
  char *data0 = (char *)malloc(50 * sizeof(char));
  char *data1 = (char *)malloc(50 * sizeof(char));
  char *attr_list_packed = (char *)malloc(1000 * sizeof(char)); // arbitrary :(

  if (this->has_node_attrs && this->read_optimize)
  {
    cursor->get_value(cursor, attr_list_packed, data0, data1, &found.in_degree,
                      &found.out_degree);
    found.attr = CommonUtil::unpack_string_vector(attr_list_packed, session);
    found.data.push_back(data0);
    found.data.push_back(data1);
  }
  else if (this->read_optimize)
  {
    cursor->get_value(cursor, data0, data1, &found.in_degree,
                      &found.out_degree);
    found.data.push_back(data0);
    found.data.push_back(data1);
  }
  else
  {
    cursor->get_value(cursor, data0, data1);
    found.data.push_back(data0);
    found.data.push_back(data1);
  }
  free(data0);
  free(data1);
  free(attr_list_packed);
  return found;
}

edge StandardGraph::__record_to_edge(WT_CURSOR *cursor)
{
  edge found;
  char *attr_list_packed = (char *)malloc(1000 * sizeof(char));
  cursor->get_value(cursor, &found.src_id, &found.dst_id, attr_list_packed);

  found.attr = CommonUtil::unpack_int_vector(attr_list_packed, session);

  free(attr_list_packed);

  return found;
}

void StandardGraph::__read_from_edge_idx(WT_CURSOR *cursor, edge_index *e_idx)
{

  cursor->get_value(cursor, e_idx->edge_id, e_idx->src_id, e_idx->dst_id);
}

void StandardGraph::add_edge(edge to_insert)
{
  // Add dst and src nodes if they don't exist.
  if (!has_node(to_insert.src_id))
  {
    node src = {0};
    src.id = to_insert.src_id;
    add_node(src);
  }
  if (!has_node(to_insert.dst_id))
  {
    node dst = {0};
    dst.id = to_insert.dst_id;
    add_node(dst);
  }

  WT_CURSOR *cursor = nullptr;
  int ret = _get_table_cursor(EDGE_TABLE, &cursor, false);

  // check if the edge exists already, if so, get edge_id
  int edge_id = get_edge_id(to_insert.src_id, to_insert.dst_id);
  if (edge_id > 0)
  {
    // The edge exists, set the cursor to point to that edge_id
    cursor->set_key(cursor, edge_id);
  }
  else
  {
    // The edge does not exist, use the global edge-id and update it by 1
    cursor->set_key(cursor, this->edge_id);
    this->edge_id++;
  }
  // If has_edge_attr is true, but none are provided
  if (this->has_edge_attrs)
  {
    if (to_insert.attr.size() == 0)
    {
      to_insert.attr = CommonUtil::get_default_nonstring_attrs("I");
    }
    size_t packed_attr_buf_size;
    string fmt;
    char *packed = CommonUtil::pack_int_vector(to_insert.attr, session,
                                               &packed_attr_buf_size, &fmt);
    cursor->set_value(cursor, to_insert.src_id, to_insert.dst_id, packed);
  }
  else
  {
    cursor->set_value(cursor, to_insert.src_id, to_insert.dst_id, 0);
  }
  ret = cursor->insert(cursor);
  if (ret != 0)
  {
    throw GraphException("Failed to insert edge (" +
                         to_string(to_insert.src_id) + "," +
                         to_string(to_insert.dst_id));
  }
  cursor->close(cursor);
  // If read_optimized is true, we update in/out degreees in the node table.
  if (this->read_optimize)
  {
    // update in/out degrees for src node in NODE_TABLE
    ret = _get_table_cursor(NODE_TABLE, &cursor, false);
    CommonUtil::check_return(ret, "Failed to open a cursor to the node table");

    cursor->set_key(cursor, to_insert.src_id);
    cursor->search(cursor);
    node found = __record_to_node(cursor);
    found.out_degree = found.out_degree + 1;
    if (is_directed)
    {
      found.in_degree = found.in_degree + 1;
    }
    update_node_degree(cursor, found.id, found.in_degree,
                       found.out_degree); //! pass the cursor to this function

    // update in/out degrees for the dst node in the NODE_TABLE
    cursor->set_key(cursor, to_insert.dst_id);
    cursor->search(cursor);
    found = __record_to_node(cursor);
    found.in_degree = found.in_degree + 1;
    if (is_directed)
    {
      found.out_degree = found.out_degree + 1;
    }
    update_node_degree(cursor, found.id, found.in_degree, found.out_degree);
    cursor->close(cursor);
  }
}

void StandardGraph::delete_edge(int src_id, int dst_id)
{
  int edge_id = get_edge_id(src_id, dst_id);
  if (edge_id <= 0)
  {
    return;
  }
  WT_CURSOR *cursor = nullptr;
  int ret = _get_table_cursor(EDGE_TABLE, &cursor, false);
  CommonUtil::check_return(ret, "Could not get a cursor to the edge table");

  cursor->set_key(cursor, edge_id);
  ret = cursor->remove(cursor);
  CommonUtil::check_return(ret, "Failed to delete edge (" + to_string(src_id) +
                                    "," + to_string(dst_id));
  // Delete reverse edge if the graph is undiected.
  if (!is_directed)
  {
    edge_id = get_edge_id(dst_id, src_id);
    if (edge_id <= 0)
    {
      return;
    }
    cursor->set_key(cursor, edge_id);
    ret = cursor->remove(cursor);
    CommonUtil::check_return(ret, "Failed to delete edge (" +
                                      to_string(src_id) + "," +
                                      to_string(dst_id));
  }
  cursor->close(cursor);
  // Update in/out degrees for the src and dst nodes if the graph is read
  // optimized
  ret = _get_table_cursor(NODE_TABLE, &cursor, false);
  CommonUtil::check_return(ret, "Could not get a cursor to the node table");
  // Update src node's in/out degrees
  cursor->set_key(cursor, src_id);
  cursor->search(cursor);
  node found = __record_to_node(cursor);
  found.out_degree = found.out_degree - 1;
  if (!is_directed)
  {
    found.in_degree = found.in_degree - 1;
  }
  update_node_degree(cursor, found.id, found.in_degree, found.out_degree);

  // Update dst node's in/out degrees
  cursor->set_key(cursor, dst_id);
  cursor->search(cursor);
  found = __record_to_node(cursor);
  found.in_degree = found.in_degree - 1;
  if (!is_directed)
  {
    found.out_degree = found.out_degree - 1;
  }
  update_node_degree(cursor, found.id, found.in_degree, found.out_degree);
  cursor->close(cursor);
}

void StandardGraph::update_node_degree(WT_CURSOR *cursor, int node_id,
                                       int in_degree, int out_degree)
{
  cursor->set_key(cursor, node_id);
  int ret = cursor->search(cursor);
  if (ret != 0)
  {
    CommonUtil::check_return(ret, "Could not find node with id = " +
                                      to_string(node_id));
  }
  node found = __record_to_node(cursor);

  if (found.in_degree != 0)
  {
    found.in_degree = in_degree;
  }
  if (found.out_degree != 0)
  {
    found.out_degree = out_degree;
  }
  __node_to_record(cursor, found);
}

edge StandardGraph::get_edge(int src_id, int dst_id)
{
  edge found = {0};
  int edge_id = get_edge_id(src_id, dst_id);
  if (edge_id < 0)
  {
    return found;
  }

  WT_CURSOR *cursor = nullptr;
  int ret = _get_table_cursor(EDGE_TABLE, &cursor, false);
  cursor->set_key(cursor, edge_id);
  ret = cursor->search(cursor);
  if (ret != 0)
  {
    return found;
  }
  else
  {
    return __record_to_edge(cursor);
  }
}

std::vector<edge> StandardGraph::get_edges()
{
  WT_CURSOR *cursor = nullptr;
  vector<edge> edges;
  int ret = _get_table_cursor(EDGE_TABLE, &cursor, false);
  CommonUtil::check_return(ret, "Failed to get a cursor to the edge table");

  while ((ret = cursor->next(cursor) == 0))
  {
    edge item = __record_to_edge(cursor);
    edges.push_back(item);
  }
  return edges;
}

std::vector<edge> StandardGraph::get_out_edges(int node_id)
{
  vector<edge> edges;
  if (!has_node(node_id))
  {
    throw GraphException("Could not find node with id" + to_string(node_id));
  }

  WT_CURSOR *cursor = nullptr, *edge_cursor = nullptr;
  int ret = _get_index_cursor(EDGE_TABLE, SRC_INDEX,
                              "(" + ID + "," + SRC + "," + DST + ")", &cursor);
  cursor->reset(cursor);
  cursor->set_key(cursor, node_id);
  ret = cursor->search(cursor);
  vector<int> edge_ids;
  if (ret == 0)
  {
    edge_index e_idx;

    do
    {
      __read_from_edge_idx(cursor, &e_idx);
      edge_ids.push_back(edge_id);
    } while ((ret = cursor->next(cursor) == 0) && (e_idx.src_id == node_id));
  }
  cursor->close(cursor);
  // Now get edges from the edge_ids

  if (edge_ids.size() > 0)
  {
    ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, false);
    CommonUtil::check_return(ret, "Could not get a cursor to the edge table");

    for (int found_edge : edge_ids)
    {
      edge_cursor->set_key(edge_cursor, found_edge);
      ret = edge_cursor->search(edge_cursor);
      if (ret == 0)
      {
        edge from_edge_table = __record_to_edge(edge_cursor);
        edges.push_back(from_edge_table);
      }
    }
    edge_cursor->close(edge_cursor);
  }
  return edges;
}

vector<node> StandardGraph::get_out_nodes(int node_id)
{
  vector<int> dst_ids;
  vector<node> nodes;
  if (!has_node(node_id))
  {
    throw GraphException("Could not find node with id" + to_string(node_id));
  }
  WT_CURSOR *cursor = nullptr;
  int ret = _get_index_cursor(EDGE_TABLE, SRC_INDEX,
                              "(" + ID + "," + SRC + "," + DST + ")", &cursor);
  cursor->reset(cursor);
  cursor->set_key(cursor, node_id);
  cursor->search(cursor);
  if (ret == 0)
  {
    edge_index e_idx;
    do
    {
      __read_from_edge_idx(cursor, &e_idx);
      dst_ids.push_back(e_idx.dst_id);
    } while ((ret = cursor->next(cursor) == 0) && (e_idx.src_id == node_id));
  }
  cursor->close(cursor);
  // Now get all adjacent nodes using dst_ids
  if (dst_ids.size() > 0)
  {
    ret = _get_table_cursor(NODE_TABLE, &cursor, false);
    CommonUtil::check_return(ret, "Could not get a cursor to the node table");

    for (int dst_id : dst_ids)
    {
      cursor->set_key(cursor, dst_id);
      ret = cursor->search(cursor);
      if (ret == 0)
      {
        node found_dst = __record_to_node(cursor);
        nodes.push_back(found_dst);
      }
    }
  }
  cursor->close(cursor);
  return nodes;
}

vector<edge> StandardGraph::get_in_edges(int node_id)
{
  vector<int> edge_ids;
  vector<edge> in_edges;

  if (!has_node(node_id))
  {
    throw GraphException("Could not find node with ID " + to_string(node_id));
  }

  // Get all edge_ids for edges with src = node_id
  WT_CURSOR *cursor = nullptr;
  int ret = _get_index_cursor(EDGE_TABLE, DST_INDEX,
                              "(" + ID + "," + SRC + "," + DST + ")", &cursor);
  cursor->reset(cursor);
  cursor->set_key(cursor, node_id);
  ret = cursor->search(cursor);
  if (ret == 0)
  {
    edge_index e_idx;
    do
    {
      __read_from_edge_idx(cursor, &e_idx);
      edge_ids.push_back(e_idx.edge_id);
    } while ((ret = cursor->next(cursor) == 0) && (e_idx.dst_id == node_id));
  }
  cursor->close(cursor);
  if (edge_ids.size() > 0)
  {
    _get_table_cursor(EDGE_TABLE, &cursor, false);
    for (int edge_id : edge_ids)
    {
      cursor->set_key(cursor, edge_id);
      ret = cursor->search(cursor);
      if (ret == 0)
      {
        edge found = __record_to_edge(cursor);
        in_edges.push_back(found);
      }
    }
  }
  cursor->close(cursor);
  return in_edges;
}

vector<node> StandardGraph::get_in_nodes(int node_id)
{
  vector<int> src_ids;
  vector<node> in_nodes;

  if (!has_node(node_id))
  {
    throw GraphException("Could not find node with ID " + to_string(node_id));
  }

  // Get all node ids for nodes adjacent to node_id
  WT_CURSOR *cursor = nullptr;
  int ret = _get_index_cursor(EDGE_TABLE, DST_INDEX,
                              "(" + ID + "," + SRC + "," + DST + ")", &cursor);
  cursor->reset(cursor);
  cursor->set_key(cursor, node_id);
  ret = cursor->search(cursor);
  if (ret == 0)
  {
    edge_index e_idx;
    do
    {
      __read_from_edge_idx(cursor, &e_idx);
      src_ids.push_back(e_idx.src_id);
    } while ((ret = cursor->next(cursor) == 0) && (e_idx.dst_id == node_id));
  }
  cursor->close(cursor);
  if (src_ids.size() > 0)
  {
    ret = _get_table_cursor(NODE_TABLE, &cursor, false);
    for (int src_id : src_ids)
    {
      cursor->set_key(cursor, src_id);
      if (cursor->search(cursor) == 0)
      {
        node found = __record_to_node(cursor);
        in_nodes.push_back(found);
      }
    }
  }
  cursor->close(cursor);
  return in_nodes;
}
/**
 * @brief Set the node's data field specified by the index. The data can either
 * be an int or a float converted to a string. The API does not make any
 * assumptions about the converted-to-string datatype being sent to it; we
 * assume the caller handles this.
 *
 * @param node_id
 * @param idx
 * @param data
 */
void StandardGraph::set_node_data(int node_id, int idx, string data)
{
  if (idx < 0 || idx > 1)
  {
    throw GraphException("Index out of bounds when setting node data");
  }

  WT_CURSOR *cursor = nullptr;
  int ret = _get_table_cursor(NODE_TABLE, &cursor, false);
  CommonUtil::check_return(ret, "Could not get cursor for the Node table");

  cursor->set_key(cursor, node_id);
  ret = cursor->search(cursor);
  CommonUtil::check_return(ret, "Failed to find a node with ID = " +
                                    to_string(node_id));
  node found = __record_to_node(cursor);
  found.data[idx] = data;

  __node_to_record(cursor, found);
  cursor->close(cursor);
}

/**
 * @brief Get node data for the node identified by node_id at the given index.
 * Returns the string form of the data
 *
 * @param node_id
 * @param idx
 * @return string
 */
string StandardGraph::get_node_data(int node_id, int idx)
{
  if (idx < 0 || idx > 1)
  {
    throw GraphException("Index out of bounds when setting node data");
  }

  WT_CURSOR *cursor = nullptr;
  int ret = _get_table_cursor(NODE_TABLE, &cursor, false);
  CommonUtil::check_return(ret, "Could not get cursor for the Node table");

  cursor->set_key(cursor, node_id);
  ret = cursor->search(cursor);
  CommonUtil::check_return(ret,
                           "Failed to find node with ID " + to_string(node_id));
  node found = __record_to_node(cursor);
  return found.data[idx];
}

//int main() { printf("Trying this out\n"); }
