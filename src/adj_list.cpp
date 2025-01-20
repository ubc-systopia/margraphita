#include "adj_list.h"

#include <algorithm>

#include "common_util.h"

using namespace std;

[[maybe_unused]] const std::string GRAPH_PREFIX = "adj";

AdjList::AdjList(graph_opts &opt_params, WT_CONNECTION *conn)
    : GraphBase(opt_params, conn)

{
  init_cursors();
}

/**
 * @brief This function is used to check if a node identified by node_id exists
 * in the node table.
 *
 * @param node_id the node_id to be searched.
 * @return true if the node is found; false otherwise.
 */
bool AdjList::has_node(node_id_t node_id)
{
  CommonUtil::set_key(node_cursor, node_id);
  int ret = node_cursor->search(node_cursor);
  node_cursor->reset(node_cursor);
  if (!ret)
  {
    return true;
  }
  else
  {
    return false;
  }
}
void AdjList::create_wt_tables(graph_opts &opts, WT_CONNECTION *conn)
{
  // Initialize edge ID for the edge table
  // AdjList has no edge id in the edge table, but we are using the same
  // structure as used by Standardgraph. So, the edge_id value will be -999
  // for all edges in the AdjList implementation.

  // Set up the node table
  // The node entry is of the form: <id>,<in_degree>,<out_degree>
  // If the graph is opts.read_optimized, add columns and format for in/out
  // degrees
  WT_SESSION *sess;
  if (CommonUtil::open_session(conn, &sess) != 0)
  {
    throw GraphException("Cannot open session");
  }
  vector<string> node_columns = {ID};
  string node_value_format;
  string node_key_format = "u";
  if (opts.read_optimize)
  {
    if (opts.is_directed)
    {
      node_columns.push_back(IN_DEGREE);
      node_columns.push_back(OUT_DEGREE);
      node_value_format = "II";
    }
    else
    {
      // Only one degree for undirected graph.
      node_columns.push_back(OUT_DEGREE);
      node_value_format = "I";
    }
  }
  else
  {
    node_columns.emplace_back(
        "na");  // have to do this because the column count must match
    node_value_format = "s";  // 1 byte fixed length char[] to hold ""
  }
  // Now Create the Node Table
  CommonUtil::set_table(
      sess, NODE_TABLE, node_columns, node_key_format, node_value_format);

  // ******** Now set up the Edge Table     **************
  // Edge Column Format : <src><dst><weight>
  // Now prepare the edge value format. starts with uu for src,dst. Add
  // another I if weighted
  vector<string> edge_columns = {SRC, DST};
  string edge_key_format = "uu";  // SRC DST in the edge table
  string edge_value_format;       // Make I if weighted , x otherwise
  if (opts.is_weighted)
  {
    edge_columns.push_back(WEIGHT);
    edge_value_format += "i";
  }
  else
  {
    edge_columns.emplace_back("NA");
    edge_value_format +=
        "b";  // uses 8 bits, which is the smallest possible value (?) other
              // than x -- padded byte which I don't fully understand
    // TODO: check if this is referred anywhere in the unweighted code path
  }

  // Create edge table
  CommonUtil::set_table(
      sess, EDGE_TABLE, edge_columns, edge_key_format, edge_value_format);

  string adjlist_key_format = "u";  // int32_t
  string adjlist_value_format =
      "Iu";  // uint32_t for in/out degree, and a variable length byte array
             // for the adjacency list. This HAS to be u. S does not work. s
             // needs the number.

  /**
   * We only make the in_adjlist table if the graph is directed.
   * The out_adjlist table is always created.
   */
  if (opts.is_directed)
  {
    // Create adjlist_in_edges table for a directed graph
    vector<string> in_adjlist_columns = {ID, IN_DEGREE, IN_ADJLIST};
    CommonUtil::set_table(sess,
                          IN_ADJLIST,
                          in_adjlist_columns,
                          adjlist_key_format,
                          adjlist_value_format);
  }

  // Create adjlist_out_edges table
  vector<string> out_adjlist_columns = {ID, OUT_DEGREE, OUT_ADJLIST};
  CommonUtil::set_table(sess,
                        OUT_ADJLIST,
                        out_adjlist_columns,
                        adjlist_key_format,
                        adjlist_value_format);
  sess->close(sess, nullptr);
}

/**
 * This function is used to initialize all cursors.
 *
 * If the graph is undirected, we only need one (out) adjacency list table.
 * In that case, the in_adjlist table does not exist and the in_adjlist cursor
 * should also not exist. While we could make a more complicated
 * change, adding checks everywhere, the easiest way is to just construct the
 * in_adjlist coursor to be on the out_adjlist table.
 */
void AdjList::init_cursors()
{
  // metadata_cursor initialization
  int ret;
  if ((ret = _get_table_cursor(
           METADATA, &metadata_cursor, session, false, false)))
  {
    throw GraphException("Could not get a cursor to the metadata table:" +
                         string(wiredtiger_strerror(ret)));
  }

  // node_cursor initialization
  if ((ret = _get_table_cursor(NODE_TABLE, &node_cursor, session, false, true)))
  {
    throw GraphException("Could not get a cursor to the node table:" +
                         string(wiredtiger_strerror(ret)));
  }

  // edge_cursor initialization
  if ((ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, session, false, true)))
  {
    throw GraphException("Could not get a cursor to the edge table:" +
                         string(wiredtiger_strerror(ret)));
  }

  // out_adjlist_cursors initialization
  if ((ret = _get_table_cursor(
           OUT_ADJLIST, &out_adjlist_cursor, session, false, false)))
  {
    throw GraphException("Could not get a cursor to the out adjlist table : " +
                         string(wiredtiger_strerror(ret)));
  }
  // in_adjlist_cursor initialization
  opts.is_directed ? (ret = _get_table_cursor(IN_ADJLIST,
                                              &in_adjlist_cursor,
                                              session,
                                              false,
                                              false))  // directed
                   : (ret = _get_table_cursor(OUT_ADJLIST,
                                              &in_adjlist_cursor,
                                              session,
                                              false,
                                              false));  // undirected
  if (ret)
  {
    throw GraphException("Could not get a cursor to the in_Adjlist table: " +
                         string(wiredtiger_strerror(ret)));
  }
}

/**
 * @brief The information that gets persisted to WT is of the form:
 * <node_id>,in_degree,out_degree.
 * in_degree and out_degree are persisted if opts.read_optimize is true
 *
 * @param to_insert the node object to be inserted
 * @returns 0 if operation is successful
 * @returns WT_ROLLBACK if there is a concurrent read/write conflict
 * @returns WT_DUPLICATE_KEY if there is a key conflict within the database(can
 * happen with concurrent add_edges/add_nodes)
 * @throws GraphException for other WT errors
 */
int AdjList::add_node(node to_insert, bool is_bulk)
{
  (void)is_bulk;
  session->begin_transaction(session, "isolation=snapshot");
  int ret;

  CommonUtil::set_key(node_cursor, to_insert.id);

  if (opts.read_optimize)
  {
    opts.is_directed
        ? node_cursor->set_value(
              node_cursor, to_insert.in_degree, to_insert.out_degree)
        : node_cursor->set_value(node_cursor, to_insert.out_degree);
  }
  else
  {
    node_cursor->set_value(node_cursor, "");
  }

  if ((ret = error_check_insert_txn(node_cursor->insert(node_cursor), false)))
  {
    if (ret == WT_DUPLICATE_KEY)
    {
      LOG_MSG("Duplicate key: Node {} already exists: {}",
              to_insert.id,
              wiredtiger_strerror(ret));
    }
    return ret;
  }

  if (opts.is_directed)
  {
    if ((ret = error_check_insert_txn(
             add_adjlist(in_adjlist_cursor, to_insert.id), false)))
    {
      return ret;
    }
  }

  if ((ret = error_check_insert_txn(
           add_adjlist(out_adjlist_cursor, to_insert.id), false)))
  {
    return ret;
  }

  session->commit_transaction(session, nullptr);
  GraphBase::increment_nodes(1);
  return ret;
}

// This function does not handle the add_node numbers
int AdjList::add_node_in_txn(node to_insert, bool ignore_duplicate)
{
  int ret;
  CommonUtil::set_key(node_cursor, to_insert.id);

  if (opts.read_optimize)
  {
    opts.is_directed
        ? node_cursor->set_value(
              node_cursor, to_insert.in_degree, to_insert.out_degree)
        : node_cursor->set_value(node_cursor, to_insert.out_degree);
  }
  else
  {
    node_cursor->set_value(node_cursor, "");
  }

  if ((ret = error_check_insert_txn(node_cursor->insert(node_cursor),
                                    ignore_duplicate)))
  {
    if (ret == WT_DUPLICATE_KEY)
    {
      LOG_MSG("Duplicate key in add_node_in_txn. Node {} already exists: {}",
              to_insert.id,
              wiredtiger_strerror(ret));
    }
    return ret;  // The transaction has been rolled back. Abort.
  }
  if ((ret = error_check_insert_txn(
           add_adjlist(out_adjlist_cursor, to_insert.id), ignore_duplicate)))
  {
    if (ret == WT_DUPLICATE_KEY)
    {
      DEBUG_MSG("Duplicate key in add_adjlist. An adjlist for node " +
                to_string(to_insert.id) +
                " already exists. : " + wiredtiger_strerror(ret));
    }
    return ret;
  }
  if (opts.is_directed)
  {
    if ((ret = error_check_insert_txn(
             add_adjlist(in_adjlist_cursor, to_insert.id), ignore_duplicate)))
    {
      if (ret == WT_DUPLICATE_KEY)
      {
        LOG_MSG(
            "Duplicate key in add_node_in_txn. Node {} already exists: "
            "{}",
            to_insert.id,
            wiredtiger_strerror(ret));
      }
      return ret;
    }
  }
  return ret;
}

void AdjList::add_node(node_id_t to_insert,
                       std::vector<node_id_t> &inlist,
                       std::vector<node_id_t> &outlist)
{
  int ret;

  CommonUtil::set_key(node_cursor, to_insert);

  if (opts.read_optimize)
  {
    opts.is_directed
        ? node_cursor->set_value(node_cursor, inlist.size(), outlist.size())
        : node_cursor->set_value(node_cursor, inlist.size());
  }
  else
  {
    node_cursor->set_value(node_cursor, "");
  }

  ret = node_cursor->insert(node_cursor);

  if (ret != 0)
  {
    throw GraphException("Failed to add node_id" + std::to_string(to_insert));
  }
  // Now add the adjlist entries
  if (opts.is_directed) add_adjlist(in_adjlist_cursor, to_insert, inlist);
  add_adjlist(out_adjlist_cursor, to_insert, outlist);

  GraphBase::increment_nodes(1);
}

/**
 * @brief Add a record for the node_id in the in or out adjlist,
 * as pointed by the cursor.
 * if the node_id record already exists then reset it with an empty list.
 **/
// TODO:create an overloaded function that accepts a fully formed in adj list
// and adds it directly.
int AdjList::add_adjlist(WT_CURSOR *cursor, node_id_t node_id)
{
  // Check if the cursor is not NULL, else throw exception
  if (cursor == nullptr)
  {
    throw GraphException("Uninitiated Cursor passed to add_adjlist call");
  }

  CommonUtil::set_key(cursor, node_id);

  // Now, initialize the in/out degree to 0 and adjlist to empty list
  WT_ITEM item = {.data = {}, .size = 0};  // todo: check
  cursor->set_value(cursor, 0, &item);     // serialize the vector and send ""

  return cursor->insert(cursor);
}

// TODO: Clarify use case of this method, may result in inconsistencies between
// in/out adjlist tables and in/out degree within node table
void AdjList::add_adjlist(WT_CURSOR *cursor,
                          node_id_t node_id,
                          std::vector<node_id_t> &list)
{
  // Check if the cursor is not NULL, else throw exception
  if (cursor == nullptr)
  {
    throw GraphException("Uninitiated Cursor passed to add_adjlist call");
  }

  CommonUtil::set_key(cursor, node_id);

  // Now, initialize the in/out degree to 0 and adjlist to empty list
  WT_ITEM item;
  // item.data = CommonUtil::pack_int_vector_wti(session, list, &item.size);
  item.data = list.data();
  item.size = list.size() * sizeof(node_id_t);
  cursor->set_value(cursor,
                    list.size(),
                    &item);  // serialize the vector and send ""

  if (cursor->insert(cursor) != 0)
  {
    throw GraphException("Failed to add node_id" + std::to_string(node_id));
  }
}

/**
 * @brief Delete the record of the node_id in the in or out
 * adjlist as pointed by the cursor.
 **/
int AdjList::delete_adjlist(WT_CURSOR *cursor, node_id_t node_id)
{
  int ret;
  // Check if the cursor is not NULL, else throw exception
  if (cursor == nullptr)
  {
    throw GraphException("Uninitiated Cursor passed to delete_adjlist");
  }

  CommonUtil::set_key(cursor, node_id);
  ret = error_check_remove_txn(cursor->remove(cursor));
  if (ret)
  {
    DEBUG_MSG("Failed to delete adjlist for node_id " +
              std::to_string(node_id) + "; TX rolled back.");
    return ret;
  }
  cursor->reset(cursor);
  return ret;
}

/**
 * @brief Adds an edge to the graph; will attempt to add nodes associated with
 * the edge
 *
 * @param to_insert the edge object to be inserted
 * @param is_bulk_insert indicates whether system is in bulk_insert mode
 * @returns 0 if operation is successful
 * @returns WT_ROLLBACK if there was a conflict and the transaction was rolled
 * back.
 * @throws GraphException for other database errors
 */

int AdjList::add_edge(edge to_insert, bool is_bulk)
{
  (void)is_bulk;
  int ret;
  int num_nodes_added = 0;

  session->begin_transaction(session, "isolation=snapshot");
  /*****Insert SRC and DST if they don't exist.*****/
  node src{.id = to_insert.src_id};
  ret = add_node_in_txn(src, true);  // ok to have duplicate key
  if (ret == WT_ROLLBACK)
  {
    DEBUG_MSG("Failed to add node_id " + to_string(to_insert.src_id));
    return WT_ROLLBACK;
  }
  else if (ret == WT_DUPLICATE_KEY)
  {
    LOG_MSG("Duplicate node, no change to node count");
  }
  else
    num_nodes_added++;

  node dst{.id = to_insert.dst_id};
  ret = add_node_in_txn(dst, true);  // ok to have duplicate key
  if (ret == WT_ROLLBACK)            // ok to have duplicate key
  {
    DEBUG_MSG("Failed to add node_id " + to_string(to_insert.dst_id));
    return WT_ROLLBACK;
  }
  else if (ret == WT_DUPLICATE_KEY)
  {
    LOG_MSG("Duplicate node, no change to node count");
  }
  else
    num_nodes_added++;

  /***** Insert edge *****/
  CommonUtil::set_key(edge_cursor, to_insert.src_id, to_insert.dst_id);

  if (opts.is_weighted)
  {
    edge_cursor->set_value(edge_cursor, to_insert.edge_weight);
  }
  else
  {
    edge_cursor->set_value(edge_cursor, 0);
  }
  if ((ret = error_check_insert_txn(edge_cursor->insert(edge_cursor), false)))
  {
    return ret;
  }
  // insert the reverse edge if undirected
  if (!opts.is_directed)  // ####This is fine :)
  {
    CommonUtil::set_key(edge_cursor, to_insert.dst_id, to_insert.src_id);
    if (opts.is_weighted)
    {
      edge_cursor->set_value(edge_cursor, to_insert.edge_weight);
    }
    else
    {
      edge_cursor->set_value(edge_cursor, 0);
    }
    if ((ret = error_check_insert_txn(edge_cursor->insert(edge_cursor), false)))
    {
      return ret;
    }
  }
  // Insert the nodes into the adjacency list tables. We assume that there are
  // no duplicate edges.
  ret = add_to_adjlists(out_adjlist_cursor, to_insert.src_id, to_insert.dst_id);
  if (ret != 0)
  {
    return ret;
  }

  // Update the in_adj table if the graph is directed (or the out_table for
  // the reverse edge is the graph is undirected)
  ret = add_to_adjlists(in_adjlist_cursor, to_insert.dst_id, to_insert.src_id);
  if (ret)
  {
    return ret;
  }

  // If opts.read_optimized is true, we update in/out degrees in the node
  // table.
  if (this->opts.read_optimize)
  {
    // update out degree for src node in NODE_TABLE
    if ((ret = update_node_degree(node_cursor, to_insert.src_id, 0, 1)))
    {
      return ret;
    }

    // update in degree for the dst node in the NODE_TABLE
    if ((ret = update_node_degree(node_cursor, to_insert.dst_id, 1, 0)))
    {
      return ret;
    }
  }
  session->commit_transaction(session, nullptr);
  std::cout << "number of nodes before:" << GraphBase::get_num_nodes()
            << std::endl;
  std::cout << "number of nodes added: " << num_nodes_added << std::endl;
  std::cout << "number of nodes after:" << GraphBase::get_num_nodes()
            << std::endl;
  GraphBase::increment_nodes(num_nodes_added);
  GraphBase::increment_edges(1);
  if (!opts.is_directed)
  {
    GraphBase::increment_edges(1);
  }
  return ret;
}

node AdjList::get_random_node()
{
  node found = {0};
  WT_CURSOR *random_cursor = nullptr;
  int ret = _get_table_cursor(NODE_TABLE, &random_cursor, session, true, false);
  if (ret != 0)
  {
    throw GraphException("could not get a random cursor to the node table");
  }
  ret = random_cursor->next(random_cursor);
  if (ret != 0)
  {
    throw GraphException("Could not seek a random node in the table");
  }

  CommonUtil::record_to_node(
      random_cursor, &found, opts.read_optimize, opts.is_directed);
  CommonUtil::get_key(random_cursor, &found.id);
  return found;
}

void AdjList::get_random_node_ids(std::vector<node_id_t> &random_nodes,
                                  int count)
{
  WT_CURSOR *random_cursor = nullptr;
  int ret = _get_table_cursor(NODE_TABLE, &random_cursor, session, true, false);
  if (ret != 0)
  {
    throw GraphException("could not get a random cursor to the node table");
  }
  node found = {0};
  int _count = 0;
  while (random_cursor->next(random_cursor) == 0)
  {
    CommonUtil::get_key(random_cursor, &found.id);
    CommonUtil::record_to_node(
        random_cursor, &found, opts.read_optimize, opts.is_directed);
    if (found.out_degree > 0)
    {
      random_nodes.push_back(found.id);
      _count++;
    }
    if (_count == count)
    {
      break;
    }
  }
}

/**
 * @brief Deletes the to_delete from the node table and also removes all of its
 * related edges and adjacency list, and to_delete's in and out adj lists.
 *
 * @param to_delete the node to be removed
 */
int AdjList::delete_node(node_id_t to_delete)
{
  session->begin_transaction(session, "isolation=snapshot");
  int ret;
  int num_deleted_edges = 0;
  delete_related_edges_and_adjlists(to_delete, &num_deleted_edges);
  CommonUtil::set_key(node_cursor, to_delete);
  if ((ret = error_check_remove_txn(node_cursor->remove(node_cursor))))
  {
    DEBUG_MSG("Failed to delete to_delete " + std::to_string(to_delete) +
              "; TX rolled back.");
    return ret;
  }
  node_cursor->reset(node_cursor);

  // delete OUT_ADJLIST entries
  if ((ret = delete_adjlist(out_adjlist_cursor, to_delete)))
  {
    return ret;
  }

  if (opts.is_directed)
  {
    // delete IN_ADJLIST entries
    if ((ret = delete_adjlist(in_adjlist_cursor, to_delete)))
    {
      return ret;
    }
  }

  session->commit_transaction(session, nullptr);
  GraphBase::increment_nodes(-1);
  GraphBase::increment_edges(-num_deleted_edges);
  return ret;
}

/**
 * @brief Get the in degree for the node provided
 *
 * @param node_id ID for which in_degree is required
 * @return int in degree of the node node_id
 */
uint32_t AdjList::get_in_degree(node_id_t node_id)
{
  int ret;
  if (opts.read_optimize)
  {
    CommonUtil::set_key(node_cursor, node_id);
    ret = node_cursor->search(node_cursor);
    if (ret != 0)
    {
      // throw GraphException("Could not find a node with ID " +
      //                      std::to_string(node_id));
      return 0;
    }
    node found{.id = node_id, .in_degree = 0, .out_degree = 0};
    CommonUtil::record_to_node(
        node_cursor, &found, opts.read_optimize, opts.is_directed);
    if (!opts.is_directed) found.in_degree = found.out_degree;
    node_cursor->reset(node_cursor);
    return found.in_degree;
  }
  else
  {
    CommonUtil::set_key(in_adjlist_cursor, node_id);
    ret = in_adjlist_cursor->search(in_adjlist_cursor);
    if (ret != 0)
    {
      // throw GraphException("Could not find node with ID" +
      //                      std::to_string(node_id) + " in the
      //                      adjlist");
      return 0;
    }
    adjlist in_edges;
    in_edges.node_id = node_id;
    CommonUtil::record_to_adjlist(in_adjlist_cursor, &in_edges);
    in_adjlist_cursor->reset(in_adjlist_cursor);
    return in_edges.degree;
  }
}

/**
 * @brief Get the out degree for the node requested
 *
 * @param node_id The ID of the node for which the degree is sought
 * @return int the node degree for the node with ID node_id.
 */
uint32_t AdjList::get_out_degree(node_id_t node_id)
{
  node_cursor->reset(node_cursor);
  if (opts.read_optimize)
  {
    CommonUtil::set_key(node_cursor, node_id);
    if (node_cursor->search(node_cursor) != 0)
    {
      // throw GraphException("Could not find a node with ID " +
      //                      std::to_string(node_id));
      return 0;
    }
    node found{.id = node_id, .in_degree = 0, .out_degree = 0};
    CommonUtil::record_to_node(
        node_cursor, &found, opts.read_optimize, opts.is_directed);
    node_cursor->reset(node_cursor);
    return found.out_degree;
  }

  else
  {
    CommonUtil::set_key(out_adjlist_cursor, node_id);
    if (out_adjlist_cursor->search(out_adjlist_cursor) != 0)
    {
      // throw GraphException("Could not find a node with ID " +
      //                      std::to_string(node_id) + " in the
      //                      adjlist");
      return 0;
    }
    adjlist out_edges;
    out_edges.node_id = node_id;
    CommonUtil::record_to_adjlist(out_adjlist_cursor, &out_edges);
    out_adjlist_cursor->reset(out_adjlist_cursor);
    return out_edges.degree;
  }
}

/**
 * @brief Get all nodes in the graph
 *
 * @return std::vector<node> vector of all nodes
 */
std::vector<node> AdjList::get_nodes()
{
  std::vector<node> nodelist;

  while ((node_cursor->next(node_cursor) == 0))
  {
    node found;
    CommonUtil::record_to_node(
        node_cursor, &found, opts.read_optimize, opts.is_directed);
    CommonUtil::get_key(node_cursor, &found.id);
    nodelist.push_back(found);
  }
  node_cursor->reset(node_cursor);
  return nodelist;
}

/**
 * @brief Get the node identified by node ID
 *
 * @param node_id the Node ID
 * @return node the node struct containing the node
 */
node AdjList::get_node(node_id_t node_id)
{
  node found = {};
  CommonUtil::set_key(node_cursor, node_id);
  int ret = node_cursor->search(node_cursor);
  if (ret == 0)
  {
    CommonUtil::record_to_node(
        node_cursor, &found, opts.read_optimize, opts.is_directed);
    found.id = node_id;
  }
  else
  {
    found.id = OutOfBand_ID_MAX;
    found.out_degree = 0;
    found.in_degree = 0;
  }
  node_cursor->reset(node_cursor);
  return found;
}

/**
 * @brief Get a list of all the edges in the graph
 *
 * @return std::vector<edge> Vector containing all the edges in the graph
 */
std::vector<edge> AdjList::get_edges()
{
  std::vector<edge> edgelist;
  while (edge_cursor->next(edge_cursor) == 0)
  {
    edge found = {0};
    CommonUtil::get_key(edge_cursor, &found.src_id, &found.dst_id);
    if (opts.is_weighted)
    {
      CommonUtil::record_to_edge(edge_cursor, &found);
    }

    edgelist.push_back(found);
  }
  edge_cursor->reset(edge_cursor);
  return edgelist;
}

/**
 * @brief Get the edge identified by (src_id, dst_id)
 *
 * @param src_id source id
 * @param dst_id destination id
 * @return edge edge identified by (src,dst) pair
 */
edge AdjList::get_edge(node_id_t src_id, node_id_t dst_id)
{
  edge found = {};
  CommonUtil::set_key(edge_cursor, src_id, dst_id);
  int ret = edge_cursor->search(edge_cursor);
  if (ret == 0)
  {
    found.src_id = src_id;
    found.dst_id = dst_id;
    if (opts.is_weighted)
    {
      CommonUtil::record_to_edge(edge_cursor, &found);
    }
  }
  else
  {
    found.src_id = OutOfBand_ID_MAX;
    found.dst_id = OutOfBand_ID_MAX;
  }
  edge_cursor->reset(edge_cursor);
  return found;
}

/**
 * @brief Check if an edge (srd_id, dst_id) exists in the graph
 *
 * @param src_id source id
 * @param dst_id destination id
 * @return true if the edge exists
 * @return false if the edge does not exist
 */
bool AdjList::has_edge(node_id_t src_id, node_id_t dst_id)
{
  int ret;
  CommonUtil::set_key(edge_cursor, src_id, dst_id);
  ret = edge_cursor->search(edge_cursor);
  edge_cursor->reset(edge_cursor);
  return (ret == 0);  // true if found :)
}

/**
 * @brief update the in/out degree for the node identified by node_id
 . The key must already be set in the cursor.
 * @param cursor Cursor to the node table
 * @param node_id node for which in/out degrees are being modified
 * @param in_degree increment or decrement amount for in_degree
 * @param out_degree increment or decrement amount for out_degree
 * @throws GraphException if the node degree could not be updated
 *
 */
int AdjList::update_node_degree(WT_CURSOR *cursor,
                                node_id_t node_id,
                                int32_t indeg_change,
                                int32_t outdeg_change)
{
  node found = {.id = node_id};
  CommonUtil::set_key(cursor, found.id);
  int ret;
  if (error_check_read_txn(ret = cursor->search(cursor)))
  {
    return ret;
  }

  CommonUtil::record_to_node(
      cursor, &found, opts.read_optimize, opts.is_directed);
  found.out_degree += outdeg_change;
  opts.is_directed ? found.in_degree += indeg_change
                   : found.out_degree += indeg_change;

  CommonUtil::dump_node(found);

  opts.is_directed
      ? cursor->set_value(cursor, found.in_degree, found.out_degree)
      : cursor->set_value(cursor, found.out_degree);
  ret = cursor->update(cursor);

  switch (ret)
  {
    case 0:
      return 0;
    case WT_ROLLBACK:
      LOG_MSG("Failed to update node degree for node {}; TX rolled back: {}",
              node_id,
              wiredtiger_strerror(ret));
      session->rollback_transaction(session, nullptr);
      return WT_ROLLBACK;
    default:
      throw GraphException("Failed to update node degree for node_id " +
                           std::to_string(node_id));
  }
}

/**
 * @brief Get a list of nodes that have an incoming edge from node_id (Are an
 * out node for node_id)
 *
 * @param node_id The node which is being queried.
 * @return std::vector<node> The vector of out nodes
 * @throws GraphException if could not acquire cursors to node table or the
 * out_adjlist tables; if a dst node was found in the adj_list for node_id but
 * does not exist in the node table
 */
std::vector<node> AdjList::get_out_nodes(node_id_t node_id)
{
  std::vector<node> out_nodes;
  if (!has_node(node_id))
  {
    throw GraphException("There is no node with ID " + to_string(node_id));
  }
  std::vector<node_id_t> adjlist = get_adjlist(out_adjlist_cursor, node_id);

  for (auto dst_id : adjlist)
  {
    out_nodes.push_back(get_node(dst_id));
  }
  out_adjlist_cursor->reset(out_adjlist_cursor);
  return out_nodes;
}

/**
 * @brief Get a list of ids of nodes that have an incoming edge from node_id
 * (are out nodes for node_id); does one less cursor search than get_out_nodes
 *
 * @param node_id The node which is being queried.
 * @return std::vector<node> The vector of out nodes
 * @throws GraphException if could not acquire cursors to node table or the
 * out_adjlist tables;
 */
std::vector<node_id_t> AdjList::get_out_nodes_id(node_id_t node_id)
{
  std::vector<node_id_t> adjlist;
  if (!has_node(node_id))
  {
    throw GraphException("There is no node with ID " + to_string(node_id));
  }
  adjlist = get_adjlist(out_adjlist_cursor, node_id);
  out_adjlist_cursor->reset(out_adjlist_cursor);
  return adjlist;
}

/**
 * @brief Return a vector of all edges that have node_id as their source node.
 *
 * @param node_id The source node
 * @return std::vector<edge> the vector of all edges which have node_id as src
 */
std::vector<edge> AdjList::get_out_edges(node_id_t node_id)
{
  int ret;
  std::vector<edge> out_edges;
  std::vector<node_id_t> dst_nodes;
  if (!has_node(node_id))
  {
    throw GraphException("There is no node with ID " + to_string(node_id));
  }
  dst_nodes = get_adjlist(out_adjlist_cursor, node_id);

  for (auto dst : dst_nodes)
  {
    CommonUtil::set_key(edge_cursor, node_id, dst);
    ret = edge_cursor->search(edge_cursor);
    if (ret != 0)
    {
      throw GraphException("expected to find an edge from " +
                           to_string(node_id) + " to " + to_string(dst) +
                           " but didn't");
    }
    edge found;
    found.src_id = node_id;
    found.dst_id = dst;
    CommonUtil::record_to_edge(edge_cursor, &found);
    edge_cursor->reset(edge_cursor);
    out_edges.push_back(found);
  }
  return out_edges;
}

/**
 * @brief Get a list of nodes that have an outgoing edge to node_id (are an in
 * node for node_id)
 *
 * @param node_id The node which is being queried.
 * @return std::vector<node> the vector of in nodes
 * @throws GraphException if could not acquire cursors to node table or the
 * in_adjlist tables; if a src node was found in the in_adj_list for node_id but
 * does not exist in the node table
 */
std::vector<node> AdjList::get_in_nodes(node_id_t node_id)
{
  std::vector<node> in_nodes;
  if (!has_node(node_id))
  {
    throw GraphException("There is no node with ID " + to_string(node_id));
  }
  std::vector<node_id_t> adjlist = get_adjlist(in_adjlist_cursor, node_id);
  for (auto src_id : adjlist)
  {
    in_nodes.push_back(get_node(src_id));
  }
  in_adjlist_cursor->reset(in_adjlist_cursor);
  return in_nodes;
}

/**
 * @brief Get a list of ids of nodes that have an outgoing edge to node_id (are
 * in nodes for node_id); does one less cursor search than get_in_nodes
 *
 * @param node_id The node which is being queried.
 * @return std::vector<node> the vector of in nodes
 * @throws GraphException if could not acquire cursors to node table or the
 * in_adjlist tables
 */
std::vector<node_id_t> AdjList::get_in_nodes_id(node_id_t node_id)
{
  std::vector<node_id_t> in_nodes_id;
  if (!has_node(node_id))
  {
    throw GraphException("There is no node with ID " + to_string(node_id));
  }
  std::vector<node_id_t> adjlist = get_adjlist(in_adjlist_cursor, node_id);
  in_adjlist_cursor->reset(in_adjlist_cursor);
  return adjlist;
}

/**
 * @brief Return a vector of all edges that have node_id as their dst node
 *
 * @param node_id the destination node
 * @return std::vector<edge> the vector of all edges that have node_id as dst
 */
std::vector<edge> AdjList::get_in_edges(node_id_t node_id)
{
  std::vector<edge> in_edges;
  std::vector<node_id_t> src_nodes;
  int ret;

  src_nodes = get_adjlist(in_adjlist_cursor, node_id);

  for (auto src : src_nodes)
  {
    CommonUtil::set_key(edge_cursor, src, node_id);
    ret = edge_cursor->search(edge_cursor);
    if (ret != 0)
    {
      throw GraphException("Expected to find an edge from " + to_string(src) +
                           " to " + to_string(node_id) + " but didn't");
    }
    edge found;
    found.src_id = src;
    found.dst_id = node_id;
    CommonUtil::record_to_edge(edge_cursor, &found);
    in_edges.push_back(found);
    edge_cursor->reset(edge_cursor);
  }
  return in_edges;
}

/**
 * @brief Delete an edge identified by (src_id, dst_id)
 *
 * @param src_id source node ID
 * @param dst_id Destination node ID
 * @returns void
 * @throw GraphException if could not delete the edge or if the node degree
 * could not be updated.
 */
int AdjList::delete_edge(node_id_t src_id, node_id_t dst_id)
{
  // Delete (src_id, dst_id) from edge table
  session->begin_transaction(session, "isolation=snapshot");
  int ret = 0;
  CommonUtil::set_key(edge_cursor, src_id, dst_id);
  if ((ret = error_check_remove_txn(edge_cursor->remove(edge_cursor))))
  {
    DEBUG_MSG("Failed to delete edge ()" + std::to_string(src_id) + "," +
              std::to_string(dst_id) + "); TX rolled back.");
    return ret;
  }
  // delete (dst_id, src_id) from edge table if undirected
  if (!opts.is_directed)
  {
    CommonUtil::set_key(edge_cursor, dst_id, src_id);
    if ((ret = error_check_remove_txn(edge_cursor->remove(edge_cursor))))
    {
      DEBUG_MSG("Failed to delete edge ()" + std::to_string(dst_id) + "," +
                std::to_string(src_id) + "); TX rolled back.");
      return ret;
    }
  }
  edge_cursor->reset(edge_cursor);

  // remove from adjacency lists
  if ((ret = delete_from_adjlists(out_adjlist_cursor, src_id, dst_id)))
  {
    return ret;
  }
  if ((ret = delete_from_adjlists(in_adjlist_cursor, dst_id, src_id)))
  {
    return ret;
  }

  // remove reverse from adj lists if undirected
  if (!opts.is_directed)
  {
    if ((ret = delete_from_adjlists(in_adjlist_cursor, src_id, dst_id)))
    {
      return ret;
    }
    if ((ret = delete_from_adjlists(out_adjlist_cursor, dst_id, src_id)))
    {
      return ret;
    }
  }

  // if opts.read_optimized -- update in/out degrees in the node table
  if (opts.read_optimize)
  {
    if ((ret = update_node_degree(node_cursor, src_id, 0, -1)))
    {
      return ret;
    }
    if ((ret = update_node_degree(node_cursor, dst_id, -1, 0)))
    {
      return ret;
    }
  }
  session->commit_transaction(session, nullptr);
  GraphBase::increment_edges(-1);
  if (!opts.is_directed)
  {
    GraphBase::increment_edges(-1);
  }
  return ret;
}

/**
 * @brief For the edge identified with (src_id, dst_id) update the edge weight
 *
 * @param src_id source id
 * @param dst_id dst ID
 * @param edge_weight new edge weight
 * @throws GraphException if trying to update weight for an unweighted graph, if
 * the edge cursor could not be found, or if the update operation fails.
 */
[[maybe_unused]] void AdjList::update_edge_weight(node_id_t src_id,
                                                  node_id_t dst_id,
                                                  int32_t edge_weight)
{
  if (!opts.is_weighted)
  {
    throw GraphException("Trying to insert weight for an unweighted graph");
  }
  int ret;
  CommonUtil::set_key(edge_cursor, src_id, dst_id);
  edge_cursor->set_value(edge_cursor, edge_weight);
  ret = edge_cursor->insert(edge_cursor);
  if (ret != 0)
  {
    throw GraphException("Could not update edge weight for edge (" +
                         std::to_string(src_id) + ", " +
                         std::to_string(dst_id) + ")");
  }
}

/**
 * @brief Return the adjacency list for the node using the cursor provided. The
 * caller passes the cursor to the table from which the adjlist is required to
 * be read.
 *
 * @param cursor A cursor to the in_list or out_list table.
 * @param node_id Node ID for which the adjlist is to be read from cursor.
 * @return std::vector<int> The AdjList for the node.
 */
std::vector<node_id_t> AdjList::get_adjlist(WT_CURSOR *cursor,
                                            node_id_t node_id)
{
  int ret;
  adjlist adj_list;

  CommonUtil::set_key(cursor, node_id);
  ret = cursor->search(cursor);
  if (ret == WT_NOTFOUND)
  {
    LOG_MSG("The node with ID {} does not exist", node_id);
    return {};
  }

  CommonUtil::record_to_adjlist(cursor, &adj_list);
  cursor->reset(cursor);
  return adj_list.edgelist;
}

int AdjList::add_to_adjlists(WT_CURSOR *cursor,
                             node_id_t node_id,
                             node_id_t to_insert)
{
  int ret;
  CommonUtil::set_key(cursor, node_id);
  ret = cursor->search(cursor);
  if (error_check_read_txn(ret))
  {
    return ret;
  }
  adjlist found = adjlist();
  found.node_id = node_id;
  CommonUtil::record_to_adjlist(cursor, &found);  //<-- This works just fine.
  found.edgelist.emplace_back(to_insert);  // this needs to be converted first.
  found.degree += 1;
  ret = error_check_insert_txn(
      CommonUtil::adjlist_to_record(session, cursor, found), true);
  if (ret != 0)
  {
    DEBUG_MSG("Could not insert adjlist for " + std::to_string(node_id) +
              "; TX NOT rolled back.");
  }
  cursor->reset(cursor);
  return ret;
}

int AdjList::delete_from_adjlists(WT_CURSOR *cursor,
                                  node_id_t node_id,
                                  node_id_t to_delete)
{
  // Not checking for directional or undirectional that would be taken
  // care by the caller.

  int ret;

  CommonUtil::set_key(cursor, node_id);
  if (error_check_read_txn(ret = cursor->search(cursor)))
  {
    return ret;
  }

  adjlist found;
  found.node_id = node_id;
  CommonUtil::record_to_adjlist(cursor, &found);
  for (size_t i = 0; i < found.edgelist.size(); i++)
  {
    if (found.edgelist.at(i) == to_delete)
    {
      found.edgelist.erase(found.edgelist.begin() + i);
    }
  }

  found.degree = found.edgelist.size();

  if ((ret = CommonUtil::adjlist_to_record(session, cursor, found)))
  {
    return ret;
  }
  cursor->reset(cursor);
  return 0;
}

[[maybe_unused]] void AdjList::delete_node_from_adjlists(node_id_t node_id)
{
  // We need to delete the node from both tables
  // and go through all the adjlist values, iterate over its edgelist and
  // correspondingly delete the node_id from the edgelist of its
  // neighbors.
  std::vector<node_id_t> in_edgelist;
  std::vector<node_id_t> out_edgelist;

  WT_CURSOR *in_cursor = nullptr;
  _get_table_cursor(IN_ADJLIST, &in_cursor, session, false, false);
  CommonUtil::set_key(in_cursor, node_id);

  if (in_cursor->search(in_cursor) != 0)
  {
    throw GraphException("Could not find " + std::to_string(node_id) +
                         " in the AdjList Table");
  }

  in_edgelist = get_adjlist(in_cursor, node_id);
  // Iterate through the in_edgelist in the OUT_ADJLIST and delete the
  // node_id from its neighbors

  WT_CURSOR *out_cursor = nullptr;
  _get_table_cursor(OUT_ADJLIST, &out_cursor, session, false, false);

  for (auto neighbor : in_edgelist)
  {
    delete_from_adjlists(out_cursor, neighbor, node_id);
  }

  // Now, get the out_edgelist from the OUTLIST Table to delete from the
  // IN_Table

  CommonUtil::set_key(out_cursor, node_id);
  if (out_cursor->search(out_cursor) != 0)
  {
    throw GraphException("Could not find " + std::to_string(node_id) +
                         " in the AdjList Table");
  }

  out_edgelist = get_adjlist(out_cursor, node_id);

  // Now, go to IN_TABLE and delete node_id from its tables in edgelist
  for (auto neighbor : out_edgelist)
  {
    delete_from_adjlists(in_cursor, neighbor, node_id);
  }

  // Now, remove the node from both the tables
  CommonUtil::set_key(out_cursor, node_id);

  if (out_cursor->remove(out_cursor) != 0)
  {
    throw GraphException("Could not delete node with ID " + to_string(node_id) +
                         " from the OUT_ADJLIST");
  }

  CommonUtil::set_key(in_cursor, node_id);

  if (in_cursor->remove(in_cursor) != 0)
  {
    throw GraphException("Could not delete node with ID " + to_string(node_id) +
                         " from the IN_ADJLIST");
  }
}

/**
 * @brief Delete all edges related to the node to_delete. This means deleting
 * edges that have to_delete as either src or dst node. Entries are deleted
 from
 * the edge and the adjlist tables.

 * @param to_delete node ID which has to be deleted from edge and adjlist
 tables
 * @returns number of edges deleted
 */
int AdjList::delete_related_edges_and_adjlists(node_id_t to_delete,
                                               int *num_edges_deleted)
{
  // initialize all the cursors
  int ret = 0;
  *num_edges_deleted = 0;

  // Get outgoing nodes
  std::vector<node_id_t> out_nodes = get_adjlist(out_adjlist_cursor, to_delete);
  for (auto dst : out_nodes)
  {
    if ((ret = delete_edge_in_txn(to_delete, dst, in_adjlist_cursor)))
    {
      return ret;
    }
    else
    {
      *num_edges_deleted += 1;
    }
  }

  if (opts.is_directed)
  {
    // Get incoming nodes
    std::vector<node_id_t> in_nodes = get_adjlist(in_adjlist_cursor, to_delete);
    for (auto src : in_nodes)
    {
      if ((ret = delete_edge_in_txn(src, to_delete, out_adjlist_cursor)))
      {
        return ret;
      }
      else
      {
        *num_edges_deleted += 1;
      }
    }
  }

  return ret;
}

int AdjList::delete_edge_in_txn(node_id_t src_id,
                                node_id_t dst_id,
                                WT_CURSOR *nbd2prune)
{
  int ret = 0;
  CommonUtil::set_key(edge_cursor, src_id, dst_id);
  if ((ret = error_check_remove_txn(edge_cursor->remove(edge_cursor))))
  {
    DEBUG_MSG("Failed to delete edge ()" + std::to_string(src_id) + "," +
              std::to_string(dst_id) + "); TX rolled back.");
    return ret;
  }
  // delete (dst_id, src_id) from edge table if undirected
  if (!opts.is_directed)
  {
    CommonUtil::set_key(edge_cursor, dst_id, src_id);
    if ((ret = error_check_remove_txn(edge_cursor->remove(edge_cursor))))
    {
      DEBUG_MSG("Failed to delete edge ()" + std::to_string(dst_id) + "," +
                std::to_string(src_id) + "); TX rolled back.");
      return ret;
    }
  }
  edge_cursor->reset(edge_cursor);

  if ((ret = delete_from_adjlists(nbd2prune, dst_id, src_id)))
  {
    return ret;
  }

  if (opts.is_directed)
  {
    if ((ret = delete_from_adjlists(nbd2prune, src_id, dst_id)))
    {
      return ret;
    }
  }

  //    // remove reverse from adj lists if undirected
  //    if (!opts.is_directed)
  //    {
  //        if ((ret = delete_from_adjlists(in_adjlist_cursor, src_id,
  //        dst_id)))
  //        {
  //            return ret;
  //        }
  //        if ((ret = delete_from_adjlists(out_adjlist_cursor, dst_id,
  //        src_id)))
  //        {
  //            return ret;
  //        }
  //    }

  // if opts.read_optimized -- update in/out degrees in the node table
  if (opts.read_optimize)
  {
    if ((ret = update_node_degree(node_cursor, src_id, 0, -1)))
    {
      return ret;
    }
    if ((ret = update_node_degree(node_cursor, dst_id, -1, 0)))
    {
      return ret;
    }
  }
  return ret;
}

OutCursor *AdjList::get_outnbd_iter()
{
  OutCursor *toReturn = new AdjOutCursor(get_new_out_adjlist_cursor(),
                                         session,
                                         opts.is_directed,
                                         opts.read_optimize);
  toReturn->set_key_range({OutOfBand_ID_MAX, OutOfBand_ID_MAX});
  return toReturn;
}

InCursor *AdjList::get_innbd_iter()
{
  InCursor *toReturn = new AdjInCursor(get_new_in_adjlist_cursor(),
                                       session,
                                       opts.is_directed,
                                       opts.read_optimize);
  toReturn->set_key_range({OutOfBand_ID_MAX, OutOfBand_ID_MAX});
  return toReturn;
}

NodeCursor *AdjList::get_node_iter()
{
  NodeCursor *toReturn = new AdjNodeCursor(
      get_new_node_cursor(), session, opts.is_directed, opts.read_optimize);
  toReturn->set_key_range({OutOfBand_ID_MAX, OutOfBand_ID_MAX});
  return toReturn;
}

EdgeCursor *AdjList::get_edge_iter()
{
  EdgeCursor *toReturn = new AdjEdgeCursor(
      get_new_edge_cursor(), session, opts.is_weighted, opts.read_optimize);
  edge_range range;
  range.start = key_pair(OutOfBand_ID_MAX, OutOfBand_ID_MAX);
  range.end = key_pair(OutOfBand_ID_MAX, OutOfBand_ID_MAX);
  toReturn->set_key_range(range);
  return toReturn;
}

WT_CURSOR *AdjList::get_node_cursor()
{
  if (node_cursor == nullptr)
  {
    int ret = _get_table_cursor(NODE_TABLE, &node_cursor, session, false, true);
    if (ret != 0)
    {
      throw GraphException("Could not get a test node cursor");
    }
  }

  return node_cursor;
}

WT_CURSOR *AdjList::get_new_node_cursor()
{
  WT_CURSOR *new_node_cursor = nullptr;
  int ret =
      _get_table_cursor(NODE_TABLE, &new_node_cursor, session, false, true);
  if (ret != 0)
  {
    throw GraphException("Could not get a test node cursor");
  }
  return new_node_cursor;
}

WT_CURSOR *AdjList::get_edge_cursor()
{
  if (edge_cursor == nullptr)
  {
    int ret =
        _get_table_cursor(EDGE_TABLE, &edge_cursor, session, false, false);
    if (ret != 0)
    {
      throw GraphException("Could not get a test edge cursor");
    }
  }
  return edge_cursor;
}

WT_CURSOR *AdjList::get_new_edge_cursor()
{
  WT_CURSOR *new_edge_cursor = nullptr;
  int ret =
      _get_table_cursor(EDGE_TABLE, &new_edge_cursor, session, false, false);
  if (ret != 0)
  {
    throw GraphException("Could not get a test edge cursor");
  }
  return new_edge_cursor;
}

WT_CURSOR *AdjList::get_in_adjlist_cursor()
{
  if (in_adjlist_cursor == nullptr)
  {
    int ret;
    opts.is_directed ? (ret = _get_table_cursor(IN_ADJLIST,
                                                &in_adjlist_cursor,
                                                session,
                                                false,
                                                false))  // directed
                     : (ret = _get_table_cursor(OUT_ADJLIST,
                                                &in_adjlist_cursor,
                                                session,
                                                false,
                                                false));  // undirected
    if (ret != 0)
    {
      throw GraphException("Could not get a test in_adjlist cursor");
    }
  }
  return in_adjlist_cursor;
}

WT_CURSOR *AdjList::get_new_in_adjlist_cursor()
{
  WT_CURSOR *new_in_adjlist_cursor = nullptr;
  int ret;
  opts.is_directed ? (ret = _get_table_cursor(IN_ADJLIST,
                                              &new_in_adjlist_cursor,
                                              session,
                                              false,
                                              false))  // directed
                   : (ret = _get_table_cursor(OUT_ADJLIST,
                                              &new_in_adjlist_cursor,
                                              session,
                                              false,
                                              false));  // undirected
  if (ret != 0)
  {
    throw GraphException("Could not get a test in_adjlist cursor");
  }
  return new_in_adjlist_cursor;
}

WT_CURSOR *AdjList::get_out_adjlist_cursor()
{
  if (out_adjlist_cursor == nullptr)
  {
    int ret = _get_table_cursor(
        OUT_ADJLIST, &out_adjlist_cursor, session, false, true);
    if (ret != 0)
    {
      throw GraphException("Could not get a test out_adjlist cursor");
    }
  }
  return out_adjlist_cursor;
}

WT_CURSOR *AdjList::get_new_out_adjlist_cursor()
{
  WT_CURSOR *new_out_adjlist_cursor = nullptr;
  int ret = _get_table_cursor(
      OUT_ADJLIST, &new_out_adjlist_cursor, session, false, false);
  if (ret != 0)
  {
    throw GraphException("Could not get a test out_adjlist cursor");
  }
  return new_out_adjlist_cursor;
}

WT_CURSOR *AdjList::get_new_random_outadj_cursor()
{
  WT_CURSOR *rand_out_adjlist_cursor = nullptr;
  int ret = _get_table_cursor(
      OUT_ADJLIST, &rand_out_adjlist_cursor, session, true, false);
  if (ret != 0)
  {
    throw GraphException("Could not get a test node cursor");
  }
  return rand_out_adjlist_cursor;
}

[[maybe_unused]] node AdjList::get_next_node(WT_CURSOR *n_cur)
{
  node found = {0};
  if (n_cur->next(n_cur) == 0)
  {
    CommonUtil::record_to_node(
        n_cur, &found, opts.read_optimize, opts.is_directed);
    n_cur->get_key(n_cur, &found.id);
  }
  else
  {
    found.id = OutOfBand_ID_MAX;
  }
  return found;
}

[[maybe_unused]] edge AdjList::get_next_edge(WT_CURSOR *e_cur)
{
  edge found = {0};
  if (e_cur->next(e_cur) == 0)
  {
    e_cur->get_key(e_cur, &found.src_id, &found.dst_id);
    if (opts.is_weighted)
    {
      CommonUtil::record_to_edge(e_cur, &found);
    }
  }
  else
  {
    found = {.src_id = OutOfBand_ID_MAX, .dst_id = OutOfBand_ID_MAX};
  }
  return found;
}

[[maybe_unused]] void AdjList::dump_table(std::string &table_name,
                                          int num_records)
{
  if (table_name == NODE_TABLE)
  {
    std::ofstream outfile("nodes_dump.txt");
    while (node_cursor->next(node_cursor) == 0 && num_records > 0)
    {
      num_records--;
      node found;
      CommonUtil::record_to_node(
          node_cursor, &found, opts.read_optimize, opts.is_directed);
      CommonUtil::get_key(node_cursor, &found.id);
      CommonUtil::dump_node(found, outfile);
    }
    node_cursor->reset(node_cursor);
    outfile.close();
  }
  else if (table_name == EDGE_TABLE)
  {
    std::ofstream outfile("edges_dump.txt");
    while (edge_cursor->next(edge_cursor) == 0 && num_records > 0)
    {
      edge found;
      num_records--;
      CommonUtil::get_key(edge_cursor, &found.src_id, &found.dst_id);
      if (opts.is_weighted)
      {
        CommonUtil::record_to_edge(edge_cursor, &found);
      }
      CommonUtil::dump_edge(found, outfile);
    }
    edge_cursor->reset(edge_cursor);
  }
  else if (table_name == OUT_ADJLIST)
  {
    std::ofstream outfile("outedge_dump.txt");
    while (out_adjlist_cursor->next(out_adjlist_cursor) == 0 && num_records > 0)
    {
      adjlist found;
      num_records--;
      out_adjlist_cursor->get_key(out_adjlist_cursor, &found.node_id);
      CommonUtil::record_to_adjlist(out_adjlist_cursor, &found);
      CommonUtil::dump_adjlist(found, outfile);
    }
  }
  else if (table_name == IN_ADJLIST)
  {
    std::ofstream outfile("inedge_dump.txt");
    while (in_adjlist_cursor->next(in_adjlist_cursor) == 0 && num_records > 0)
    {
      adjlist found;
      num_records--;
      in_adjlist_cursor->get_key(in_adjlist_cursor, &found.node_id);
      CommonUtil::record_to_adjlist(in_adjlist_cursor, &found);
      CommonUtil::dump_adjlist(found, outfile);
    }
  }
}

/**
 * This function is used to check the errors retruned by the WiredTiger API on
 * an insert transaction. We are interested in checking if the transaction needs
 * to be rolled back and what value to return. depending on
 * the ignore_duplicate_key param, decide whether to rollback the transaction
 * when a duplicate key is found.
 *
 * @param return_val
 * @param ignore_duplicate_key boolean to ignore duplicate key errors
 * @return WT_ROLLBACK if the transaction has been rolled back.
 * WT_DUPLICATE_KEY is returned if the key is a duplicate. The transaction is
 * rolled back if the ignore_duplicate_key flag is false. if the key is a
 * duplicate.
 * return 0 if the transaction was successful.
 */
int AdjList::error_check_insert_txn(int return_val, bool ignore_duplicate_key)
{
  switch (return_val)
  {
    case 0:
      return 0;
    case WT_ROLLBACK:
      session->rollback_transaction(session, nullptr);
      return WT_ROLLBACK;
    case WT_DUPLICATE_KEY:
      if (!ignore_duplicate_key)
      {
        LOG_MSG("Rolling back transaction in insert_txn");
        session->rollback_transaction(session, nullptr);
      }
      return WT_DUPLICATE_KEY;
    default:
      DEBUG_MSG("Rolling back transaction in insert_txn" +
                std::string(wiredtiger_strerror(return_val)));
      session->rollback_transaction(session, nullptr);
      return return_val;
  }
}

int AdjList::error_check_read_txn(int return_val)
{
  switch (return_val)
  {
    case 0:
      return 0;
    case WT_ROLLBACK:
      session->rollback_transaction(session, nullptr);
      return WT_ROLLBACK;
    case WT_NOTFOUND:
      session->rollback_transaction(session, nullptr);
      return WT_NOTFOUND;
    default:
      session->rollback_transaction(session, nullptr);
      DEBUG_MSG("Failed to complete read action : " +
                std::string(wiredtiger_strerror(return_val)));
      return return_val;
  }
}

int AdjList::error_check_remove_txn(int return_val)
{
  switch (return_val)
  {
    case 0:
      return 0;
    case WT_ROLLBACK:
      session->rollback_transaction(session, nullptr);
      DEBUG_MSG("Rollback in transaction remove : " +
                std::string(wiredtiger_strerror(return_val)));
      return WT_ROLLBACK;
    default:
      session->rollback_transaction(session, nullptr);
      DEBUG_MSG("Rollback in transaction remove : " +
                std::string(wiredtiger_strerror(return_val)));
  }
  return return_val;
}

node_id_t AdjList::get_max_node_id()
{
  WT_CURSOR *cursor = get_new_node_cursor();
  node_id_t to_return = 0;
  assert(cursor != nullptr);
  cursor->prev(cursor);
  CommonUtil::get_key(cursor, &to_return);
  std::cout << "Max node id: " << to_return << std::endl;
  cursor->close(cursor);
  return to_return;
}

node_id_t AdjList::get_min_node_id()
{
  WT_CURSOR *cursor = get_new_node_cursor();
  node_id_t to_return = 0;
  assert(cursor != nullptr);
  cursor->next(cursor);
  CommonUtil::get_key(cursor, &to_return);
  std::cout << "Min node id: " << to_return << std::endl;
  cursor->close(cursor);
  return to_return;
}
