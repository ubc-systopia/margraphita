#include "adj_list.h"

#include <omp.h>

#include <algorithm>
#include <thread>

#include "common_util.h"
#include "prop_schema.h"
using namespace std;

[[maybe_unused]] const std::string GRAPH_PREFIX = "adj";

AdjList::AdjList(graph_opts &opt_params, WT_CONNECTION *conn)
    : GraphBase(opt_params, conn)

{
  init_cursors();
}

/**
 * @brief This function is used to check if a node identified by node_id exists
 *
 * @param node_id the node_id to be searched.
 * @return true if the node is found; false otherwise.
 */
bool AdjList::has_node(node_id_t node_id)
{
#ifdef MK_NEDGES
  CommonUtil::set_key(node_cursor, node_id);
  int ret = node_cursor->search(node_cursor);
  node_cursor->reset(node_cursor);
#else
  CommonUtil::set_key(out_adjlist_cursor, node_id);
  int ret = out_adjlist_cursor->search(out_adjlist_cursor);
  out_adjlist_cursor->reset(out_adjlist_cursor);
#endif
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
#ifdef MK_NEDGES
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
      sess, NODE_TABLE, node_columns, node_key_format, node_value_format, "");

  // ******** Now set up the Edge Table     **************
  // Edge Column Format : <src><dst><weight>
  // Now prepare the edge value format. starts with uu for src,dst. Add
  // another I if weighted
  vector<string> edge_columns = {SRC, DST};
  string edge_key_format = "uu";  // SRC DST in the edge table
  string edge_value_format;       // Make I if weighted , x otherwise
  if (opts.is_weighted)
  {
    edge_columns.emplace_back("weight");
    edge_value_format += "u";
  }
  else if (opts.has_edge_props)
  {
    // Use raw blob format so the edge value slot holds property bytes directly.
    // get_edge_wt is always guarded by is_weighted, so repurposing this slot
    // for props is safe.
    edge_columns.emplace_back("props");
    edge_value_format += "u";
  }
  else
  {
    edge_columns.emplace_back("NA");
    edge_value_format += "b";
  }

  // Create edge table
  CommonUtil::set_table(
      sess, EDGE_TABLE, edge_columns, edge_key_format, edge_value_format, "");
#endif

  string adjlist_key_format = "u";  // int32_t
  string adjlist_value_format =
      "u";  // Single raw byte array: [degree (4 bytes) | node_id_t array].
            // cursor->modify() requires value_format="u" (no structured fields)
            // and no columns= specification.
  std::string table_config =
      "leaf_page_max=64KB,"
      "internal_page_max=16KB,"
      "memory_page_max=10MB,"
      "split_pct=90";
  /**
   * We only make the in_adjlist table if the graph is directed.
   * The out_adjlist table is always created.
   */
  if (opts.is_directed)
  {
    // Create adjlist_in_edges table for a directed graph
    // No columns -- required for cursor->modify() support
    CommonUtil::set_table(sess,
                          IN_ADJLIST,
                          {},
                          adjlist_key_format,
                          adjlist_value_format, table_config);
  }

  // Create adjlist_out_edges table
  // No columns -- required for cursor->modify() support
  CommonUtil::set_table(sess,
                        OUT_ADJLIST,
                        {},
                        adjlist_key_format,
                        adjlist_value_format, table_config);

  // Property tables (separate from frozen adjlist tables)
  if (opts.has_node_props && opts.prop_mode != COLUMNAR)
  {
    string node_props_cfg = "key_format=u,value_format=u,leaf_page_max=64KB";
    int ret = sess->create(
        sess, ("table:" + string(NODE_PROPS_TABLE)).c_str(), node_props_cfg.c_str());
    if (ret != 0)
      throw GraphException("Failed to create NODE_PROPS table: " +
                           string(wiredtiger_strerror(ret)));
  }

  // COLUMNAR mode: per-type property tables with column groups (same schema as SplitEdgeKey)
  if (opts.prop_mode == COLUMNAR)
  {
    int ret;
    ret = sess->create(sess, ("table:" + PERSON_PROPS_TABLE).c_str(),
        "key_format=Q,value_format=QQb,columns=(vid,creationDate,birthday,gender)");
    if (ret != 0)
      throw GraphException("AdjList: failed to create person_props: " + string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + PERSON_PROPS_TABLE + ":" + CG_TEMPORAL).c_str(),
        "columns=(creationDate,birthday)");
    if (ret != 0)
      throw GraphException("AdjList: failed to create person_props:temporal: " + string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + PERSON_PROPS_TABLE + ":" + CG_IDENTITY).c_str(),
        "columns=(gender)");
    if (ret != 0)
      throw GraphException("AdjList: failed to create person_props:identity: " + string(wiredtiger_strerror(ret)));

    ret = sess->create(sess, ("table:" + POST_PROPS_TABLE).c_str(),
        "key_format=Q,value_format=Qi,columns=(vid,creationDate,length)");
    if (ret != 0)
      throw GraphException("AdjList: failed to create post_props: " + string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + POST_PROPS_TABLE + ":" + CG_TEMPORAL).c_str(),
        "columns=(creationDate,length)");
    if (ret != 0)
      throw GraphException("AdjList: failed to create post_props:temporal: " + string(wiredtiger_strerror(ret)));

    ret = sess->create(sess, ("table:" + KNOWS_PROPS_TABLE).c_str(),
        "key_format=QQ,value_format=Q,columns=(src,dst,creationDate)");
    if (ret != 0)
      throw GraphException("AdjList: failed to create knows_props: " + string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + KNOWS_PROPS_TABLE + ":" + CG_TEMPORAL).c_str(),
        "columns=(creationDate)");
    if (ret != 0)
      throw GraphException("AdjList: failed to create knows_props:temporal: " + string(wiredtiger_strerror(ret)));

    ret = sess->create(sess, ("table:" + LIKES_PROPS_TABLE).c_str(),
        "key_format=QQ,value_format=Q,columns=(src,dst,creationDate)");
    if (ret != 0)
      throw GraphException("AdjList: failed to create likes_props: " + string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + LIKES_PROPS_TABLE + ":" + CG_TEMPORAL).c_str(),
        "columns=(creationDate)");
    if (ret != 0)
      throw GraphException("AdjList: failed to create likes_props:temporal: " + string(wiredtiger_strerror(ret)));
  }

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
  if ((ret = _get_table_cursor(METADATA,
                               &metadata_cursor,
                               session,
                               false,
                               true,  // overwrite must be allowed
                               opts.checkpoint_name)))
  {
    throw GraphException("Could not get a cursor to the metadata table:" +
                         string(wiredtiger_strerror(ret)));
  }
#ifdef MK_NEDGES
  // node_cursor initialization
  if ((ret = _get_table_cursor(NODE_TABLE,
                               &node_cursor,
                               session,
                               false,
                               false, //no overwrite for node table
                               opts.checkpoint_name)))
  {
    throw GraphException("Could not get a cursor to the node table:" +
                         string(wiredtiger_strerror(ret)));
  }

  // edge_cursor initialization
  if ((ret = _get_table_cursor(EDGE_TABLE,
                               &edge_cursor,
                               session,
                               false,
                               true, // allow overwrite for edge table 
                               opts.checkpoint_name)))

  {
    throw GraphException("Could not get a cursor to the edge table:" +
                         string(wiredtiger_strerror(ret)));
  }
#endif

  // out_adjlist_cursors initialization
  if ((ret = _get_table_cursor(OUT_ADJLIST,
                               &out_adjlist_cursor,
                               session,
                               false,
                               false,
                               opts.checkpoint_name)))
  {
    throw GraphException("Could not get a cursor to the out adjlist table : " +
                         string(wiredtiger_strerror(ret)));
  }
  // in_adjlist_cursor initialization
  opts.is_directed
      ? (ret = _get_table_cursor(IN_ADJLIST,
                                 &in_adjlist_cursor,
                                 session,
                                 false,
                                 false,
                                 opts.checkpoint_name))  // directed
      : (ret = _get_table_cursor(OUT_ADJLIST,
                                 &in_adjlist_cursor,
                                 session,
                                 false,
                                 false,
                                 opts.checkpoint_name));  // undirected
  if (ret)
  {
    throw GraphException("Could not get a cursor to the in_Adjlist table: " +
                         string(wiredtiger_strerror(ret)));
  }

  // Property cursors — only opened when property tables exist
  if (opts.has_node_props && opts.prop_mode != COLUMNAR)
  {
    if ((ret = session->open_cursor(
             session,
             ("table:" + string(NODE_PROPS_TABLE)).c_str(),
             nullptr,
             nullptr,
             &node_props_cursor)) != 0)
      throw GraphException("Could not open node_props cursor: " +
                           string(wiredtiger_strerror(ret)));
  }

  if (opts.prop_mode == COLUMNAR)
  {
    if ((ret = _get_table_cursor(PERSON_PROPS_TABLE, &person_props_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("AdjList: could not open person_props cursor: " + string(wiredtiger_strerror(ret)));
    if ((ret = _get_table_cursor(POST_PROPS_TABLE, &post_props_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("AdjList: could not open post_props cursor: " + string(wiredtiger_strerror(ret)));
    if ((ret = _get_table_cursor(KNOWS_PROPS_TABLE, &knows_props_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("AdjList: could not open knows_props cursor: " + string(wiredtiger_strerror(ret)));
    if ((ret = _get_table_cursor(LIKES_PROPS_TABLE, &likes_props_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("AdjList: could not open likes_props cursor: " + string(wiredtiger_strerror(ret)));
  }
}

void AdjList::set_node_properties(node_id_t id,
                                   const uint8_t *data,
                                   size_t size)
{
  if (opts.prop_mode == COLUMNAR)
  {
    switch (VTYPE_OF(id))
    {
      case VT_PERSON: {
        uint64_t cDate  = (uint64_t)SNBPersonSchema::get_creation_date(data);
        uint64_t bday   = (uint64_t)SNBPersonSchema::get_birthday(data);
        int8_t   gender = SNBPersonSchema::get_gender(data);
        person_props_cursor->set_key(person_props_cursor, (uint64_t)id);
        person_props_cursor->set_value(person_props_cursor, cDate, bday, gender);
        int ret = person_props_cursor->insert(person_props_cursor);
        if (ret == WT_DUPLICATE_KEY)
        {
          person_props_cursor->set_key(person_props_cursor, (uint64_t)id);
          person_props_cursor->set_value(person_props_cursor, cDate, bday, gender);
          ret = person_props_cursor->update(person_props_cursor);
        }
        if (ret != 0)
          throw GraphException("AdjList: set_node_properties person failed for " +
                               std::to_string(id) + ": " + wiredtiger_strerror(ret));
        break;
      }
      case VT_POST: {
        uint64_t cDate  = (uint64_t)SNBPostSchema::get_creation_date(data);
        int32_t  length = SNBPostSchema::get_length(data);
        post_props_cursor->set_key(post_props_cursor, (uint64_t)id);
        post_props_cursor->set_value(post_props_cursor, cDate, length);
        int ret = post_props_cursor->insert(post_props_cursor);
        if (ret == WT_DUPLICATE_KEY)
        {
          post_props_cursor->set_key(post_props_cursor, (uint64_t)id);
          post_props_cursor->set_value(post_props_cursor, cDate, length);
          ret = post_props_cursor->update(post_props_cursor);
        }
        if (ret != 0)
          throw GraphException("AdjList: set_node_properties post failed for " +
                               std::to_string(id) + ": " + wiredtiger_strerror(ret));
        break;
      }
      default:
        break;
    }
    return;
  }

  // EMBEDDED / SPLIT mode
  static const uint8_t placeholder = 0;
  CommonUtil::set_key(node_props_cursor, id);
  WT_ITEM item;
  item.data = (data != nullptr && size > 0) ? static_cast<const void *>(data)
                                             : static_cast<const void *>(&placeholder);
  item.size = (size > 0) ? size : 1;
  node_props_cursor->set_value(node_props_cursor, &item);
  int ret = node_props_cursor->insert(node_props_cursor);
  if (ret == WT_DUPLICATE_KEY)
  {
    node_props_cursor->set_value(node_props_cursor, &item);
    ret = node_props_cursor->update(node_props_cursor);
  }
  if (ret != 0)
    throw GraphException("set_node_properties failed for node " +
                         std::to_string(id) + ": " +
                         string(wiredtiger_strerror(ret)));
}

prop_blob AdjList::get_node_properties(node_id_t id)
{
  if (opts.prop_mode == COLUMNAR)
  {
    switch (VTYPE_OF(id))
    {
      case VT_PERSON: {
        person_props_cursor->set_key(person_props_cursor, (uint64_t)id);
        if (person_props_cursor->search(person_props_cursor) != 0)
          return {nullptr, 0};
        uint64_t cDate, bday; int8_t gender;
        person_props_cursor->get_value(person_props_cursor, &cDate, &bday, &gender);
        uint8_t *buf = new uint8_t[SNBPersonSchema::TOTAL_SIZE];
        SNBPersonSchema::set_creation_date(buf, (int64_t)cDate);
        SNBPersonSchema::set_birthday(buf, (int64_t)bday);
        SNBPersonSchema::set_gender(buf, gender);
        return {buf, SNBPersonSchema::TOTAL_SIZE};
      }
      case VT_POST: {
        post_props_cursor->set_key(post_props_cursor, (uint64_t)id);
        if (post_props_cursor->search(post_props_cursor) != 0)
          return {nullptr, 0};
        uint64_t cDate; int32_t length;
        post_props_cursor->get_value(post_props_cursor, &cDate, &length);
        uint8_t *buf = new uint8_t[SNBPostSchema::TOTAL_SIZE];
        SNBPostSchema::set_creation_date(buf, (int64_t)cDate);
        SNBPostSchema::set_length(buf, length);
        return {buf, SNBPostSchema::TOTAL_SIZE};
      }
      default:
        return {nullptr, 0};
    }
  }

  // EMBEDDED / SPLIT mode
  CommonUtil::set_key(node_props_cursor, id);
  if (node_props_cursor->search(node_props_cursor) != 0)
    return {nullptr, 0};
  WT_ITEM item;
  node_props_cursor->get_value(node_props_cursor, &item);
  uint8_t *copy = new uint8_t[item.size];
  memcpy(copy, item.data, item.size);
  return {copy, item.size};
}

void AdjList::set_edge_properties(node_id_t src,
                                   node_id_t dst,
                                   const uint8_t *data,
                                   size_t size)
{
  if (opts.prop_mode == COLUMNAR)
  {
    uint8_t s = (uint8_t)VTYPE_OF(src);
    uint8_t d = (uint8_t)VTYPE_OF(dst);
    WT_CURSOR *cur = nullptr;
    if (s == VT_PERSON && d == VT_PERSON) cur = knows_props_cursor;
    else if (s == VT_PERSON && d == VT_POST) cur = likes_props_cursor;
    // POST→PERSON = hasCreator, no props
    if (cur == nullptr || data == nullptr || size == 0)
      return;
    uint64_t cDate = (uint64_t)SNBKnowsSchema::get_creation_date(data);
    cur->set_key(cur, (uint64_t)src, (uint64_t)dst);
    cur->set_value(cur, cDate);
    int ret = cur->insert(cur);
    if (ret == WT_DUPLICATE_KEY)
    {
      cur->set_key(cur, (uint64_t)src, (uint64_t)dst);
      cur->set_value(cur, cDate);
      ret = cur->update(cur);
    }
    if (ret != 0)
      throw GraphException("AdjList: set_edge_properties failed (" +
                           std::to_string(src) + ", " + std::to_string(dst) + "): " +
                           wiredtiger_strerror(ret));
    return;
  }

  // EMBEDDED / SPLIT mode
  static const uint8_t placeholder = 0;
  WT_ITEM item;
  item.data = (data != nullptr && size > 0) ? static_cast<const void *>(data)
                                             : static_cast<const void *>(&placeholder);
  item.size = (size > 0) ? size : 1;

  CommonUtil::set_key(edge_cursor, src, dst);
  edge_cursor->set_value(edge_cursor, &item);
  int ret = edge_cursor->update(edge_cursor);
  if (ret != 0)
    throw GraphException("set_edge_properties failed for edge (" +
                         std::to_string(src) + ", " + std::to_string(dst) +
                         "): " + string(wiredtiger_strerror(ret)));

  if (!opts.is_directed)
  {
    CommonUtil::set_key(edge_cursor, dst, src);
    edge_cursor->set_value(edge_cursor, &item);
    edge_cursor->update(edge_cursor);
  }
}

prop_blob AdjList::get_edge_properties(node_id_t src, node_id_t dst)
{
  if (opts.prop_mode == COLUMNAR)
  {
    uint8_t s = (uint8_t)VTYPE_OF(src);
    uint8_t d = (uint8_t)VTYPE_OF(dst);
    WT_CURSOR *cur = nullptr;
    if (s == VT_PERSON && d == VT_PERSON) cur = knows_props_cursor;
    else if (s == VT_PERSON && d == VT_POST) cur = likes_props_cursor;
    if (cur == nullptr)
      return {nullptr, 0};
    cur->set_key(cur, (uint64_t)src, (uint64_t)dst);
    if (cur->search(cur) != 0)
      return {nullptr, 0};
    uint64_t cDate;
    cur->get_value(cur, &cDate);
    uint8_t *buf = new uint8_t[SNBKnowsSchema::TOTAL_SIZE];
    SNBKnowsSchema::set_creation_date(buf, (int64_t)cDate);
    return {buf, SNBKnowsSchema::TOTAL_SIZE};
  }

  // EMBEDDED / SPLIT mode
  CommonUtil::set_key(edge_cursor, src, dst);
  if (edge_cursor->search(edge_cursor) != 0)
    return {nullptr, 0};
  WT_ITEM item;
  edge_cursor->get_value(edge_cursor, &item);
  uint8_t *copy = new uint8_t[item.size];
  memcpy(copy, item.data, item.size);
  return {copy, item.size};
}

WT_CURSOR *AdjList::open_colgroup_cursor(const std::string &table,
                                          const std::string &colgroup)
{
  if (opts.prop_mode != COLUMNAR)
    throw GraphException("open_colgroup_cursor: only valid in COLUMNAR mode");
  std::string uri = "colgroup:" + table + ":" + colgroup;
  WT_CURSOR *cur = nullptr;
  int ret = session->open_cursor(session, uri.c_str(), nullptr, nullptr, &cur);
  if (ret != 0)
    throw GraphException("AdjList: open_colgroup_cursor failed to open " + uri + ": " +
                         wiredtiger_strerror(ret));
  return cur;
}

/**
 * @brief The information that gets persisted to WT is of the form:
 * <node_id>,in_degree,out_degree.
 * in_degree and out_degree are persisted if opts.read_optimize is true
 *
 * @param to_insert the node object to be inserted
 * @returns 0 if operation is successful
 * @returns WT_ROLLBACK if there is a concurrent write conflict
 * @returns WT_DUPLICATE_KEY if the node already exists
 * @throws GraphException for other WT errors
 */
int AdjList::add_node(node to_insert, bool is_bulk)
{
  (void)is_bulk;
  // LOG_MSG("Adding node with ID {}", to_insert.id);
  session->begin_transaction(session, "isolation=snapshot");
  int ret;
#ifdef MK_NEDGES
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
  if ((ret = error_check_insert_txn(node_cursor->insert(node_cursor))))
  {
    node_cursor->reset(node_cursor);
    if (ret == WT_ROLLBACK)
    {
      DEBUG_MSG("Failed to add node_id " + to_string(to_insert.id) +
                "; TX rolled back.");
      return WT_ROLLBACK;
    }
    else if (ret == WT_DUPLICATE_KEY)
    //The node already exists, so we need to rollback this transaction and return WT_DUPLICATE_KEY so that the caller can know that the node already exists. It is up to the caller to decide what's next.
    {
      LOG_MSG(
          "Duplicate node in node table. Node {} already exists. Rolling back.\n\tWT_ERROR: "
          "{}",
          to_string(to_insert.id),
          wiredtiger_strerror(ret));
      session->rollback_transaction(session, nullptr);
      return WT_DUPLICATE_KEY;
    }
    else
    { /**no-op */
    }
  }
#endif

  if (opts.is_directed)
  {
    if ((ret = error_check_insert_txn(
             add_adjlist(in_adjlist_cursor, to_insert.id))))
    {
      if (ret == WT_DUPLICATE_KEY)
      {
        DEBUG_MSG("Duplicate key in add_adjlist. An adjlist for node " +
                  to_string(to_insert.id) +
                  " already exists. : " + wiredtiger_strerror(ret));
        session->rollback_transaction(session, nullptr);
        
      }
      return ret;
    }
  }

  if ((ret = error_check_insert_txn(
           add_adjlist(out_adjlist_cursor, to_insert.id))))
  {
    if (ret == WT_DUPLICATE_KEY)
    {
      DEBUG_MSG("Duplicate key in add_adjlist. An adjlist for node " +
                to_string(to_insert.id) +
                " already exists. : " + wiredtiger_strerror(ret));
      session->rollback_transaction(session, nullptr);
    }
    return ret;
  }

  session->commit_transaction(session, nullptr);
  GraphBase::increment_nodes(1);
  return ret;
}

/**
 * @brief This function is used to add a node to the adjacency list. This is a
 * private function and is always called from another function wiht an active
 * transaction. We do not do any error checking here. The caller is responsible
 * for that.
 * @param to_insert
 * @return int (return code from wiredtiger)
 */
int AdjList::add_node_in_txn(node to_insert)
{
  int ret = 0;
#ifdef MK_NEDGES
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

  if ((ret = error_check_insert_txn(node_cursor->insert(node_cursor))))
  {
    if (ret == WT_DUPLICATE_KEY)
    {
      // get the value, and update the in/out degree and update.
      node found{};
      CommonUtil::record_to_node(
          node_cursor, &found, opts.read_optimize, opts.is_directed);
      if (opts.read_optimize)
      {
        opts.is_directed
            ? node_cursor->set_value(node_cursor,
                                     to_insert.in_degree + found.in_degree,
                                     to_insert.out_degree + found.out_degree)
            : node_cursor->set_value(node_cursor,
                                     to_insert.out_degree + found.out_degree);
      }
      else
      {
        node_cursor->set_value(node_cursor, "");
      }
      ret = node_cursor->update(node_cursor);
      if (ret != 0)
      {
        // If the update fails, we should rollback the transaction.
        session->rollback_transaction(session, nullptr);
        return WT_ROLLBACK;
      }
      return WT_DUPLICATE_KEY;  // node already exists, but we updated it
    }
    else
    {
      return ret;  // other errors
    }
  }
#endif
  return ret;
}

[[maybe_unused]] int AdjList::add_node(node_id_t to_insert,
                                       std::vector<node_id_t> &inlist,
                                       std::vector<node_id_t> &outlist)
{
  int ret = 0;
#ifdef MK_NEDGES
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
#endif

  // Now add the adjlist entries
  if (opts.is_directed)
  {
    if ((ret = error_check_insert_txn(
             add_adjlist(in_adjlist_cursor, to_insert, inlist))))
    {
      if (ret == WT_DUPLICATE_KEY)
      {
        DEBUG_MSG("Duplicate key in add_adjlist. An adjlist for node " +
                  to_string(to_insert) +
                  " already exists. : " + wiredtiger_strerror(ret));
      }
      return ret;
    }
  }

  if ((ret = error_check_insert_txn(
           add_adjlist(out_adjlist_cursor, to_insert, outlist))))
  {
    if (ret == WT_DUPLICATE_KEY)
    {
      DEBUG_MSG("Duplicate key in add_adjlist. An adjlist for node " +
                to_string(to_insert) +
                " already exists. : " + wiredtiger_strerror(ret));
    }
    return ret;
  }

  GraphBase::increment_nodes(1);
  return 0;
}

/**
 * @brief Add a record for the node_id in the in or out adjlist,
 * as pointed by the cursor.
 * if the node_id record already exists then reset it with an empty list.
 * No error checking is done here.
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

  // Initialize with degree=0 packed as raw bytes (value_format=u)
  degree_t zero = 0;
  WT_ITEM item = {.data = &zero, .size = sizeof(degree_t)};
  cursor->set_value(cursor, &item);

  return error_check_insert_txn(cursor->insert(cursor));
}

// TODO: Clarify use case of this method, may result in inconsistencies between
// in/out adjlist tables and in/out degree within node table
int AdjList::add_adjlist(WT_CURSOR *cursor,
                         node_id_t node_id,
                         std::vector<node_id_t> &list)
{
  // Check if the cursor is not NULL, else throw exception
  if (cursor == nullptr)
  {
    throw GraphException("Uninitiated Cursor passed to add_adjlist call");
  }

  CommonUtil::set_key(cursor, node_id);

  // Pack [degree (4 bytes) | edgelist bytes] into a single raw buffer
  size_t edgelist_bytes = list.size() * sizeof(node_id_t);
  std::vector<uint8_t> buf(sizeof(degree_t) + edgelist_bytes);
  degree_t deg = static_cast<degree_t>(list.size());
  memcpy(buf.data(), &deg, sizeof(degree_t));
  if (edgelist_bytes > 0)
  {
    memcpy(buf.data() + sizeof(degree_t), list.data(), edgelist_bytes);
  }

  WT_ITEM item;
  item.data = buf.data();
  item.size = buf.size();
  cursor->set_value(cursor, &item);

  return cursor->insert(cursor);
}

/**
 * @brief Delete the record of the node_id in the in or out
 * adjlist as pointed by the cursor.
 **/
int AdjList::delete_adjlist(WT_CURSOR *cursor,
                            node_id_t node_id,
                            degree_t *num_edges_deleted)
{
  int ret;
  // Check if the cursor is not NULL, else throw exception
  if (cursor == nullptr)
  {
    throw GraphException("Uninitiated Cursor passed to delete_adjlist");
  }

  adjlist adj_list;

  CommonUtil::set_key(cursor, node_id);
  ret = cursor->search(cursor);
  if (ret == WT_NOTFOUND)
  {
    LOG_MSG("The node with ID {} does not exist", node_id);
    return 0;  // no-op, nothing to delete
  }

  CommonUtil::record_to_adjlist(cursor, &adj_list);

  // The cursor is still positioned.
  ret = cursor->remove(cursor);
  if (ret == 0)
  {
    *num_edges_deleted += adj_list.degree;
  }
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
  int ret = 0;
  int num_nodes_added = 0;

  session->begin_transaction(session, "isolation=snapshot");
#ifdef DEBUG
  std::cout << "Adding edge: " << to_insert.src_id << " -> " << to_insert.dst_id
            << std::endl;
#endif
#ifdef MK_NEDGES
  node src_node, dst_node;

  // Set degree increments based on graph type
  src_node.id = to_insert.src_id;
  src_node.in_degree = 0;   // Source never gets incoming degree
  src_node.out_degree = 1;  // Source always gets +1 outgoing

  dst_node.id = to_insert.dst_id;
  dst_node.in_degree = opts.is_directed ? 1 : 0;
  // Destination gets +1 incoming only if directed
  dst_node.out_degree = opts.is_directed ? 0 : 1;
  // Destination gets +1 outgoing only if undirected (only have out_degrees)

// Determine processing order for consistent locking
#ifdef OrderNodes
  // Process in ascending ID order to prevent deadlocks
  node first, second;
  if (to_insert.src_id < to_insert.dst_id)
  {
    first = src_node;
    second = dst_node;
  }
  else
  {
    first = dst_node;
    second = src_node;
  }
#else
  // Process in natural src -> dst order
  node first = src_node;
  node second = dst_node;
#endif

  /*****Insert SRC and DST if they don't exist.*****/
  ret = add_node_in_txn(first);  // ok to have duplicate key
  if (ret == WT_ROLLBACK)
  {
    //    DEBUG_MSG("Failed to add node_id " + to_string(to_insert.src_id));
    LOG_ROLLBACK_LOCATION("add_node_in_txn(first)", to_insert);
    return WT_ROLLBACK;
  }
  else if (ret == WT_DUPLICATE_KEY)
  {
    LOG_MSG("Duplicate node, no change to node count");
  }
  else
    num_nodes_added++;
  // Now add the second node
  ret = add_node_in_txn(second);  // ok to have duplicate key
  if (ret == WT_ROLLBACK)         // ok to have duplicate key
  {
    // DEBUG_MSG("Failed to add node_id " + to_string(to_insert.dst_id));
    LOG_ROLLBACK_LOCATION("add_node_in_txn(first)", to_insert);
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
    set_edge_wt(edge_cursor, to_insert.edge_weight);
  }
  else if (opts.has_edge_props)
  {
    // Store a 1-byte placeholder; real properties written later via
    // set_edge_properties.  value_format="u" so we must provide a WT_ITEM.
    static const uint8_t empty = 0;
    WT_ITEM item = {.data = &empty, .size = 1};
    edge_cursor->set_value(edge_cursor, &item);
  }
  else
  {
    set_edge_wt(edge_cursor, 0.0);
  }
  if ((ret = error_check_insert_txn(edge_cursor->insert(edge_cursor))))
  {
    return ret;
  }
  // insert the reverse edge if undirected
  if (!opts.is_directed)  // ####This is fine :)
  {
    CommonUtil::set_key(edge_cursor, to_insert.dst_id, to_insert.src_id);
    if (opts.is_weighted)
    {
      set_edge_wt(edge_cursor, to_insert.edge_weight);
    }
    else if (opts.has_edge_props)
    {
      static const uint8_t empty = 0;
      WT_ITEM item = {.data = &empty, .size = 1};
      edge_cursor->set_value(edge_cursor, &item);
    }
    else
    {
      set_edge_wt(edge_cursor, 0.0);
    }
    if ((ret = error_check_insert_txn(edge_cursor->insert(edge_cursor))))
    {
      LOG_ROLLBACK_LOCATION("add_node_in_txn(first)", to_insert);
    }
  }
#endif
  bool node_added = false;
  node_id_t node1, node2;
#ifdef OrderNodes
  to_insert.src_id < to_insert.dst_id
      ? (node1 = to_insert.src_id, node2 = to_insert.dst_id)
      : (node1 = to_insert.dst_id, node2 = to_insert.src_id);
#else
  node1 = to_insert.src_id;
  node2 = to_insert.dst_id;
#endif

  ret = add_to_adjlists(out_adjlist_cursor, node1, node2, node_added) != 0;
  if (ret)
  {
    LOG_ROLLBACK_LOCATION(
        " add_to_adjlists(out_adjlist_cursor, node1, node2, node_added)",
        to_insert);
    return ret;
  }

  // We add all nodes to the out_adjlist_cursor, even ones with no (yet known)
  // adjlist. This is because we need to be able to get the node count from
  // out_adjlist_cursor. The add_to_adjlist call above has already set the key
  // to node1 so we add node2 to the out_adjlist_cursor
  CommonUtil::set_key(out_adjlist_cursor, node2);
  if (out_adjlist_cursor->search(out_adjlist_cursor) != 0)
  {
    ret = add_adjlist(out_adjlist_cursor, node2);
    if (ret != 0)
    {
      LOG_ROLLBACK_LOCATION("add_adjlist(out_adjlist_cursor, node2)",
                            to_insert);
      return ret;
    }
#ifndef MK_NEDGES
    num_nodes_added++;  //+1 for node2, needed only when not already done for
                        // node table
#endif
  }

#ifndef MK_NEDGES
  // we need to increment node counts here if the node table is not made.
  if (node_added)
  {
    /* We really added a new node if the node was not already present in the
    out_adjlist_cursor AND the in_adjlist_cursor (matters for directed
    graphs where the tables are separate). Now check in_adjlist_cursor.*/
    CommonUtil::set_key(in_adjlist_cursor, node1);
    if (in_adjlist_cursor->search(in_adjlist_cursor) != 0)
    {
      // Truly a new node, so increment the count.
      num_nodes_added++;  // +1 for node1
    }
  }
#endif

  /*  don't need to check for is_directed here, as the in_adjlist_cursor is  a
   * dup of the out_adjlist_cursor in the undirected case. We do not care  about
   * the node_added flag here because we have already fixed the node  count
   * above.*/
  ret = add_to_adjlists(in_adjlist_cursor, node2, node1, node_added);
  if (ret != 0)
  {
    LOG_ROLLBACK_LOCATION(
        "add_to_adjlists(in_adjlist_cursor, node2, node1, node_added)",
        to_insert);
    return ret;
  }

  if (session->commit_transaction(session, nullptr) != 0)
  {
    LOG_ROLLBACK_LOCATION("commit_transaction(session, nullptr)", to_insert);
    return WT_ROLLBACK;
  }
#ifdef DEBUG
  std::cout << "number of nodes before:" << GraphBase::get_num_nodes()
            << std::endl;
  std::cout << "number of nodes added: " << num_nodes_added << std::endl;
#endif
  if (num_nodes_added > 2)
  {
    std::cerr << "num_nodes_added > 2. " << num_nodes_added << std::endl;
  }
  GraphBase::increment_nodes(num_nodes_added);
  GraphBase::increment_edges(1);
  // if (!opts.is_directed)
  // {
  //   GraphBase::increment_edges(1);
  // }
#ifdef DEBUG
  std::cout << "number of nodes after:" << GraphBase::get_num_nodes()
            << std::endl;
#endif
  return ret;
}

node AdjList::get_random_node()
{
  adjlist found{};
  // Get a random node from the out_adjlist table.
  WT_CURSOR *random_cursor = nullptr;
  int ret = _get_table_cursor(
      OUT_ADJLIST, &random_cursor, session, true, false, opts.checkpoint_name);
  if (ret != 0)
  {
    throw GraphException("could not get a random cursor to the node table");
  }
  ret = random_cursor->next(random_cursor);
  if (ret != 0)
  {
    throw GraphException("Could not seek a random node in the table");
  }

  CommonUtil::record_to_adjlist(random_cursor, &found);
  CommonUtil::get_key(random_cursor, &found.node_id);

  return {.id = found.node_id,
          .in_degree = found.degree,
          .out_degree = found.degree};
}

void AdjList::get_random_node_ids(std::vector<node_id_t> &random_nodes,
                                  int count)
{
  WT_CURSOR *random_cursor = nullptr;
  int ret = _get_table_cursor(
      OUT_ADJLIST, &random_cursor, session, true, false, opts.checkpoint_name);
  if (ret != 0)
  {
    throw GraphException("could not get a random cursor to the node table");
  }
  adjlist found{};
  int _count = 0;
  while (random_cursor->next(random_cursor) == 0)
  {
    CommonUtil::get_key(random_cursor, &found.node_id);
    CommonUtil::record_to_adjlist(random_cursor, &found);
    if (found.degree > 0)
    {
      random_nodes.push_back(found.node_id);
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
  int ret;
  degree_t num_deleted_edges = 0;
  session->begin_transaction(session, "isolation=snapshot");
// first delete the node from the node table (if the table exists)
#ifdef MK_NEDGES
  CommonUtil::set_key(node_cursor, to_delete);
  if ((ret = error_check_insert_txn(node_cursor->remove(node_cursor))))
  {
    DEBUG_MSG("Failed to delete to_delete " + std::to_string(to_delete) +
              "; TX rolled back.");
    return ret;
  }
  node_cursor->reset(node_cursor);
  // delete the node from the adjlists and edge table (if exists)

#endif
  delete_related_edges_and_adjlists(to_delete, &num_deleted_edges);

  // delete the node from the in_adjlist table
  // For whatever reasson, placing this in the delete_related_edges_and_adjlists
  // function does not work. So, we do it here.
  if (opts.is_directed)
  {
    in_adjlist_cursor->reset(in_adjlist_cursor);
    CommonUtil::set_key(in_adjlist_cursor, to_delete);
    if ((ret = error_check_insert_txn(
             in_adjlist_cursor->remove(in_adjlist_cursor))))
    {
      if (ret == WT_NOTFOUND)
      {
        // this means that the node was not found in the in_adjlist table
        // which is fine, we can just return.
      }
      else
      {
        DEBUG_MSG("Failed to delete node " + std::to_string(to_delete) +
                  " from in_adjlist; TX rolled back.");
        return ret;
      }
    }
    in_adjlist_cursor->reset(in_adjlist_cursor);
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
#ifdef MK_NEDGES
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

#else
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
#endif
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
#ifdef MK_NEDGES
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
#else
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
#endif
}

/**
 * @brief Get all nodes in the graph
 *
 * @return std::vector<node> vector of all nodes.
 */
std::vector<node> AdjList::get_nodes()
{
  std::vector<node> nodelist;
#ifdef MK_NEDGES
  if (opts.read_optimize)
  {
    node_cursor->reset(node_cursor);
    while ((node_cursor->next(node_cursor) == 0))
    {
      node found;
      CommonUtil::record_to_node(
          node_cursor, &found, opts.read_optimize, opts.is_directed);
      CommonUtil::get_key(node_cursor, &found.id);
      nodelist.push_back(found);
    }
    node_cursor->reset(node_cursor);
  }
  else
  {
    while (out_adjlist_cursor->next(out_adjlist_cursor) == 0)
    {
      adjlist found{};

      CommonUtil::record_to_adjlist(out_adjlist_cursor, &found);
      CommonUtil::get_key(out_adjlist_cursor, &found.node_id);
      node temp = {
          .id = found.node_id, .in_degree = 0, .out_degree = found.degree};
      opts.is_directed ? temp.in_degree = get_in_degree(found.node_id)
                       : temp.in_degree = temp.out_degree;
      nodelist.push_back(temp);
    }
  }
#else
  while (out_adjlist_cursor->next(out_adjlist_cursor) == 0)
  {
    adjlist found{};

    CommonUtil::record_to_adjlist(out_adjlist_cursor, &found);
    CommonUtil::get_key(out_adjlist_cursor, &found.node_id);
    node temp = {
        .id = found.node_id, .in_degree = 0, .out_degree = found.degree};
    opts.is_directed ? temp.in_degree = get_in_degree(found.node_id)
                     : temp.in_degree = temp.out_degree;
    nodelist.push_back(temp);
  }
#endif
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
#ifdef MK_NEDGES
  if (!opts.read_optimize)
  {
    adjlist found = {};

    CommonUtil::set_key(out_adjlist_cursor, node_id);
    int ret = out_adjlist_cursor->search(out_adjlist_cursor);
    if (ret == 0)
    {
      node temp;
      CommonUtil::record_to_adjlist(out_adjlist_cursor, &found);
      temp.id = node_id;
      temp.out_degree = found.degree;
      opts.is_directed
          ? temp.in_degree = get_in_degree(node_id)
          : temp.in_degree = 0;  // in_degree is 0 for undirected graphs
      return temp;
    }
    // If the node is not in the out_adjlist.
    return {.id = OutOfBand_ID_MAX, .in_degree = 0, .out_degree = 0};
  }
  else
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

#else
  adjlist found = {};

  CommonUtil::set_key(out_adjlist_cursor, node_id);
  int ret = out_adjlist_cursor->search(out_adjlist_cursor);
  if (ret == 0)
  {
    node temp;
    CommonUtil::record_to_adjlist(out_adjlist_cursor, &found);
    temp.id = node_id;
    temp.out_degree = found.degree;
    opts.is_directed
        ? temp.in_degree = get_in_degree(node_id)
        : temp.in_degree = 0;  // in_degree is 0 for undirected graphs
    return temp;
  }
  // If the node is not in the out_adjlist.
  return {.id = OutOfBand_ID_MAX, .in_degree = 0, .out_degree = 0};
#endif
}

/**
 * @brief Get a list of all the edges in the graph
 *
 * @return std::vector<edge> Vector containing all the edges in the graph
 */
std::vector<edge> AdjList::get_edges()
{
#ifdef MK_NEDGES
  std::vector<edge> edgelist;
  while (edge_cursor->next(edge_cursor) == 0)
  {
    edge found = {0};
    CommonUtil::get_key(edge_cursor, &found.src_id, &found.dst_id);
    if (opts.is_weighted)
    {
      // CommonUtil::record_to_edge(edge_cursor, &found);
      get_edge_wt(edge_cursor, &found.edge_weight);
    }

    edgelist.push_back(found);
  }
  edge_cursor->reset(edge_cursor);
  return edgelist;
#else
  std::vector<edge> edgelist;
  while (out_adjlist_cursor->next(out_adjlist_cursor) == 0)
  {
    adjlist found{};
    CommonUtil::record_to_adjlist(out_adjlist_cursor, &found);
    CommonUtil::get_key(out_adjlist_cursor, &found.node_id);
    for (auto dst_id : found.edgelist)
    {
      edge e = {.src_id = found.node_id, .dst_id = dst_id};
      edgelist.push_back(e);
    }
  }
  out_adjlist_cursor->reset(out_adjlist_cursor);
  return edgelist;
#endif
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
#ifdef MK_NEDGES
  edge found = {};
  CommonUtil::set_key(edge_cursor, src_id, dst_id);
  int ret = edge_cursor->search(edge_cursor);
  if (ret == 0)
  {
    found.src_id = src_id;
    found.dst_id = dst_id;
    if (opts.is_weighted)
    {
      // CommonUtil::record_to_edge(edge_cursor, &found);
      get_edge_wt(edge_cursor, &found.edge_weight);
    }
  }
  else
  {
    found.src_id = OutOfBand_ID_MAX;
    found.dst_id = OutOfBand_ID_MAX;
  }
  edge_cursor->reset(edge_cursor);
  return found;
#else
  edge found = {};
  CommonUtil::set_key(out_adjlist_cursor, src_id);
  int ret = out_adjlist_cursor->search(out_adjlist_cursor);
  if (ret == 0)
  {
    adjlist out_edges;
    CommonUtil::record_to_adjlist(out_adjlist_cursor, &out_edges);
    for (auto dst : out_edges.edgelist)
    {
      if (dst_id == dst)
      {
        found.src_id = src_id;
        found.dst_id = dst_id;
        // TODO: extend for properties when we have them.
        break;
      }
    }
  }
  else
  {
    found.src_id = OutOfBand_ID_MAX;
    found.dst_id = OutOfBand_ID_MAX;
  }
  out_adjlist_cursor->reset(out_adjlist_cursor);
  return found;
#endif
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
#ifdef MK_NEDGES
  int ret;
  CommonUtil::set_key(edge_cursor, src_id, dst_id);
  ret = edge_cursor->search(edge_cursor);
  edge_cursor->reset(edge_cursor);
  return (ret == 0);  // true if found :)
#else
  int ret;
  CommonUtil::set_key(out_adjlist_cursor, src_id);
  ret = out_adjlist_cursor->search(out_adjlist_cursor);
  if (ret == 0)
  {
    adjlist out_edges;
    CommonUtil::record_to_adjlist(out_adjlist_cursor, &out_edges);
    CommonUtil::get_key(out_adjlist_cursor, &out_edges.node_id);
    for (auto dst : out_edges.edgelist)
    {
      if (dst_id == dst)
      {
        return true;
      }
    }
  }
  out_adjlist_cursor->reset(out_adjlist_cursor);
  return false;
#endif
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

#ifdef DEBUG
  std::cout << "before update: ";
  CommonUtil::dump_node(found);
#endif

  found.out_degree += outdeg_change;
  opts.is_directed ? found.in_degree += indeg_change
                   : found.out_degree += indeg_change;
#ifdef DEBUG
  std::cout << "after update: ";
  CommonUtil::dump_node(found);
#endif
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
  std::sort(adjlist.begin(), adjlist.end());
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
  std::sort(adjlist.begin(), adjlist.end());
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
  // out_adjlist_cursor->set_key(out_adjlist_cursor, node_id);
  CommonUtil::set_key(out_adjlist_cursor, node_id);
  ret = out_adjlist_cursor->search(out_adjlist_cursor);
  if (ret == 0)
  {
    adjlist out_edges_list;
    CommonUtil::record_to_adjlist(out_adjlist_cursor, &out_edges_list);
    CommonUtil::get_key(out_adjlist_cursor, &out_edges_list.node_id);
    std::sort(out_edges_list.edgelist.begin(),
              out_edges_list.edgelist.end());  // need this in sorted order
    for (auto dst_id : out_edges_list.edgelist)
    {
      edge found = {.src_id = node_id, .dst_id = dst_id};
#ifdef MK_NEDGES
      if (opts.is_weighted)
      {
        CommonUtil::set_key(edge_cursor, found.src_id, found.dst_id);
        edge_cursor->search(edge_cursor);
        get_edge_wt(edge_cursor, &found.edge_weight);
        edge_cursor->reset(edge_cursor);
      }
#else
      found.edge_weight = 0;
#endif
      out_edges.push_back(found);
    }
  }
  else
  {
    throw GraphException("Could not find node with ID " +
                         std::to_string(node_id) + " in the adjlist");
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
  std::sort(adjlist.begin(), adjlist.end());
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

  src_nodes = get_adjlist(in_adjlist_cursor, node_id);
  std::sort(src_nodes.begin(), src_nodes.end());
  for (auto src_id : src_nodes)
  {
    edge found = {.src_id = src_id, .dst_id = node_id};
#ifdef MK_NEDGES
    if (opts.is_weighted)
    {
      CommonUtil::set_key(edge_cursor, found.src_id, found.dst_id);
      edge_cursor->search(edge_cursor);
      get_edge_wt(edge_cursor, &found.edge_weight);
    }
#else
    found.edge_weight = 0;
#endif
    in_edges.push_back(found);
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
#ifdef MK_NEDGES
  CommonUtil::set_key(edge_cursor, src_id, dst_id);
  if ((ret = error_check_insert_txn(edge_cursor->remove(edge_cursor))))
  {
    DEBUG_MSG("Failed to delete edge ()" + std::to_string(src_id) + "," +
              std::to_string(dst_id) + "); TX rolled back.");
    return ret;
  }
  // delete (dst_id, src_id) from edge table if undirected
  if (!opts.is_directed)
  {
    CommonUtil::set_key(edge_cursor, dst_id, src_id);
    if ((ret = error_check_insert_txn(edge_cursor->remove(edge_cursor))))
    {
      DEBUG_MSG("Failed to delete edge ()" + std::to_string(dst_id) + "," +
                std::to_string(src_id) + "); TX rolled back.");
      return ret;
    }
  }
  edge_cursor->reset(edge_cursor);
#endif
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
  /**
   * @brief This is completely unnecessary since the in_adjlist_cursor and
   * out_adjlist_cursor point to the same table for undirected graphs.
   *
   */
  // if (!opts.is_directed)
  // {
  //   if ((ret = delete_from_adjlists(in_adjlist_cursor, src_id, dst_id)))
  //   {
  //     return ret;
  //   }
  //   if ((ret = delete_from_adjlists(out_adjlist_cursor, dst_id, src_id)))
  //   {
  //     return ret;
  //   }
  // }

  // if opts.read_optimized -- update in/out degrees in the node table
#ifdef MK_NEDGES
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
#endif
  session->commit_transaction(session, nullptr);
  GraphBase::increment_edges(-1);
  if (!opts.is_directed)
  {
    GraphBase::increment_edges(-1);
  }
  return ret;
}

/**
 * @brief This function is used for graphalytics workloads. for each edge, read
 * its current weight. If the current weight exist, add e's weight with current
 * weight and update the edge weight otherwise insert e as the new edge.
 *
 * @param to_update weighted edge to insert/update.
 * @return true if the operation succeeds
 */
bool AdjList::update_edge(edge to_update)
{
  session->begin_transaction(session, "isolation=snapshot");
  // Search for the edge in the edge table
  WT_CURSOR *edge_cur = get_edge_cursor();
  CommonUtil::set_key(edge_cur, to_update.src_id, to_update.dst_id);
  int ret = edge_cur->search(edge_cur);
  if (ret == WT_NOTFOUND)
  {
    // Edge does not exist, insert it
    session->rollback_transaction(session, nullptr);
    return (add_edge(to_update, false) == 0);
  }
  else
  {
    edgeweight_t new_edge_weight = 0.0;
    get_edge_wt(edge_cur, &new_edge_weight);
    new_edge_weight += to_update.edge_weight;
    set_edge_wt(edge_cur, new_edge_weight);

    if ((ret = error_check_insert_txn(edge_cur->update(edge_cur))))
    {
      LOG_ROLLBACK_LOCATION("update_edge(to_update)", to_update);
      return false;
    }
  }
  session->commit_transaction(session, nullptr);
  return true;
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

  CommonUtil::set_key(cursor, node_id);
  ret = cursor->search(cursor);
  if (ret == WT_NOTFOUND)
  {
    LOG_MSG("Node ID {} does not have an adjacency list", node_id);
    return {};
  }
  adjlist adj_list;
  CommonUtil::record_to_adjlist(cursor, &adj_list);
  cursor->reset(cursor);
  return adj_list.edgelist;
}

int AdjList::add_to_adjlists(WT_CURSOR *cursor,
                             node_id_t node_id,
                             node_id_t to_insert,
                             bool &node_added)
{
  int ret;

  // Pack [degree=1 (4 bytes) | to_insert node_id] for the initial insert
  degree_t initial_degree = 1;
  std::vector<uint8_t> init_buf(sizeof(degree_t) + sizeof(node_id_t));
  memcpy(init_buf.data(), &initial_degree, sizeof(degree_t));
  memcpy(init_buf.data() + sizeof(degree_t), &to_insert, sizeof(node_id_t));

  CommonUtil::set_key(cursor, node_id);
  WT_ITEM item;
  item.data = init_buf.data();
  item.size = init_buf.size();
  cursor->set_value(cursor, &item);

  ret = error_check_insert_txn(cursor->insert(cursor));
  if (ret)  // this should return a WT_DUPLICATE_KEY if the
  // node_id already exists in the table.
  {
    if (ret == WT_DUPLICATE_KEY)
    {
      // The node_id already exists. The cursor is positioned at the key.
      node_added = false;

      if (opts.sort_edges)
      {
        // Sorted insert requires a full read-modify-write (can't use
        // cursor->modify to insert at an arbitrary position efficiently)
        adjlist found = adjlist(node_id, 0);
        CommonUtil::record_to_adjlist(cursor, &found);
        found.insert_sorted(to_insert);
        return error_check_insert_txn(
            CommonUtil::adjlist_to_record(session, cursor, found));
      }

      // Use cursor->modify() for O(1) append
      WT_ITEM current_val;
      cursor->get_value(cursor, &current_val);

      degree_t current_degree = 0;
      if (current_val.size >= sizeof(degree_t))
      {
        memcpy(&current_degree, current_val.data, sizeof(degree_t));
      }
      degree_t new_degree = current_degree + 1;

      WT_MODIFY mods[2];
      // Entry 0: Replace the degree field (first 4 bytes) with degree+1
      mods[0].data.data = &new_degree;
      mods[0].data.size = sizeof(degree_t);
      mods[0].offset = 0;
      mods[0].size = sizeof(degree_t);

      // Entry 1: Append the new node_id at the end
      mods[1].data.data = &to_insert;
      mods[1].data.size = sizeof(node_id_t);
      mods[1].offset = current_val.size;
      mods[1].size = 0;  // replace 0 bytes = pure append

      ret = cursor->modify(cursor, mods, 2);
      return error_check_insert_txn(ret);
    }
    node_added = false;
    return ret;
  }
  node_added = true;  // this means that the node_id was not present
  // and we added it to the table.
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

  return error_check_insert_txn(
      CommonUtil::adjlist_to_record(session, cursor, found));
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
                                               degree_t *num_edges_deleted)
{
  // initialize all the cursors
  int ret = 0;
  *num_edges_deleted = 0;

#ifdef MK_NEDGES
  WT_CURSOR *edge_cursor_new = nullptr;
  _get_table_cursor(EDGE_TABLE,
                    &edge_cursor_new,
                    session,
                    false,
                    false,
                    opts.checkpoint_name);
  CommonUtil::set_key(edge_cursor, to_delete, OutOfBand_ID_MIN);
  int status;
  edge_cursor->search_near(edge_cursor, &status);
  if (status < 0)
  {
    ret = edge_cursor->next(edge_cursor);
    if (ret != 0)
    {
      DEBUG_MSG("No edges found with to_delete as src_id " +
                std::to_string(to_delete) + ". No edges to delete.");
      goto edge_done;
    }
  }
  // now check if the cursor is positioned at an edge with to_delete as src_id
  do
  {
    node_id_t src_id, dst_id;
    CommonUtil::get_key(edge_cursor, &src_id, &dst_id);
    if (src_id != to_delete)
    {
      // this means that there are no edges with to_delete as src_id
      // edge_cursor->reset(edge_cursor);
      // nothing to delete from the edge table.
      break;
    }
    // delete the edge (to_delete, dst_id)
    if ((ret = error_check_insert_txn(edge_cursor->remove(edge_cursor))))
    {
      DEBUG_MSG("Failed to delete edge ()" + std::to_string(to_delete) + "," +
                std::to_string(dst_id) + "); TX rolled back.");
      return ret;
    }
    // Now delete the opposite edge if undirected
    if (!opts.is_directed)
    {
      // if undirected, delete the edge (dst_id, to_delete) as well
      CommonUtil::set_key(edge_cursor_new, dst_id, to_delete);
      if ((ret = error_check_insert_txn(
               edge_cursor_new->remove(edge_cursor_new))))
      {
        DEBUG_MSG("Failed to delete edge (" + std::to_string(dst_id) + "," +
                  std::to_string(to_delete) + "); TX rolled back.");
        return ret;
      }
    }
  } while (edge_cursor->next(edge_cursor) == 0);

edge_done:
  edge_cursor->reset(edge_cursor);
#endif

  // Get outgoing nodes
  std::vector<node_id_t> out_nodes = get_adjlist(out_adjlist_cursor, to_delete);
  for (auto dst : out_nodes)
  {
    // this works for both directed and undirected graphs because the in_adjlist
    //  cursor points to the same table as out_adjlist for undirected graphs.
    delete_from_adjlists(in_adjlist_cursor, dst, to_delete);
#ifdef MK_NEDGES
    // if opts.read_optimized -- update in degrees in the node table
    if (opts.read_optimize)
    {
      if ((ret = update_node_degree(node_cursor, dst, -1, 0)))
      {
        return ret;
      }
    }
#endif
    *num_edges_deleted += 1;
  }
  // delete the node from the out_adjlist table
  CommonUtil::set_key(out_adjlist_cursor, to_delete);
  if ((ret = error_check_insert_txn(
           out_adjlist_cursor->remove(out_adjlist_cursor))))
  {
    DEBUG_MSG("Failed to delete node " + std::to_string(to_delete) +
              " from out_adjlist; TX rolled back.");
    return ret;
  }
  out_adjlist_cursor->reset(out_adjlist_cursor);

  return ret;
}

/**
 * @brief This function deletes the edge (src_id, dst_id) from the
 * edge table. It also deletes the
 *
 * @param src_id
 * @param dst_id
 * @param nbd2prune
 * @return int
 */
int AdjList::delete_edge_in_txn(node_id_t src_id,
                                node_id_t dst_id,
                                WT_CURSOR *nbd2prune)
{
  int ret = 0;
  if ((ret = delete_from_adjlists(nbd2prune, dst_id, src_id)))
  {
    return ret;
  }

  if (!opts.is_directed)
  {
    if ((ret = delete_from_adjlists(nbd2prune, src_id, dst_id)))
    {
      return ret;
    }
  }

  return ret;
}

OutCursor *AdjList::get_outnbd_iter()
{
  WT_CURSOR *iter_cursor = get_out_adjlist_cursor();
  OutCursor *toReturn = new AdjOutCursor(
      iter_cursor, session, opts.is_directed, opts.read_optimize);
  toReturn->set_key_range({OutOfBand_ID_MAX, OutOfBand_ID_MAX});
  return toReturn;
}

InCursor *AdjList::get_innbd_iter()
{
  WT_CURSOR *iter_cursor = get_in_adjlist_cursor();
  InCursor *toReturn = new AdjInCursor(
      iter_cursor, session, opts.is_directed, opts.read_optimize);
  toReturn->set_key_range({OutOfBand_ID_MAX, OutOfBand_ID_MAX});
  return toReturn;
}

NodeCursor *AdjList::get_node_iter()
{
  NodeCursor *toReturn = new AdjNodeCursor(
      get_node_cursor(), session, opts.is_directed, opts.read_optimize);
  toReturn->set_key_range({OutOfBand_ID_MAX, OutOfBand_ID_MAX});
  return toReturn;
}

EdgeCursor *AdjList::get_edge_iter()
{
  EdgeCursor *toReturn = new AdjEdgeCursor(
      get_edge_cursor(), session, opts.is_weighted, opts.read_optimize);
  edge_range range;
  range.start = key_pair(OutOfBand_ID_MAX, OutOfBand_ID_MAX);
  range.end = key_pair(OutOfBand_ID_MAX, OutOfBand_ID_MAX);
  toReturn->set_key_range(range);
  return toReturn;
}

WT_CURSOR *AdjList::get_node_cursor()
{
#ifdef MK_NEDGES
  WT_CURSOR *new_node_cursor = nullptr;
  int ret = _get_table_cursor(
      NODE_TABLE, &new_node_cursor, session, false, true, opts.checkpoint_name);
  if (ret != 0)
  {
    throw GraphException("Could not get a test node cursor");
  }
  return new_node_cursor;
#else
  WT_CURSOR *temp = nullptr;
  int ret = _get_table_cursor(
      OUT_ADJLIST, &temp, session, false, true, opts.checkpoint_name);
  if (ret != 0)
  {
    throw GraphException("Could not get a test node cursor");
  }

  return temp;
#endif
}

WT_CURSOR *AdjList::get_edge_cursor()
{
#ifdef MK_NEDGES
  WT_CURSOR *new_edge_cursor = nullptr;
  int ret = _get_table_cursor(EDGE_TABLE,
                              &new_edge_cursor,
                              session,
                              false,
                              false,
                              opts.checkpoint_name);
  if (ret != 0)
  {
    throw GraphException("Could not get a test edge cursor");
  }

  return new_edge_cursor;
#else
  WT_CURSOR *temp = nullptr;
  int ret = _get_table_cursor(
      OUT_ADJLIST, &temp, session, false, true, opts.checkpoint_name);
  if (ret != 0)
  {
    throw GraphException("Could not get a test edge cursor");
  }
  return temp;
#endif
}

WT_CURSOR *AdjList::get_in_adjlist_cursor()
{
  WT_CURSOR *new_in_adjlist_cursor = nullptr;
  int ret;
  opts.is_directed
      ? (ret = _get_table_cursor(IN_ADJLIST,
                                 &new_in_adjlist_cursor,
                                 session,
                                 false,
                                 false,
                                 opts.checkpoint_name))  // directed
      : (ret = _get_table_cursor(OUT_ADJLIST,
                                 &new_in_adjlist_cursor,
                                 session,
                                 false,
                                 false,
                                 opts.checkpoint_name));  // undirected
  if (ret != 0)
  {
    throw GraphException("Could not get a test in_adjlist cursor");
  }
  return new_in_adjlist_cursor;
}

WT_CURSOR *AdjList::get_out_adjlist_cursor()
{
  WT_CURSOR *new_out_adjlist_cursor = nullptr;
  int ret = _get_table_cursor(OUT_ADJLIST,
                              &out_adjlist_cursor,
                              session,
                              false,
                              true,
                              opts.checkpoint_name);
  if (ret != 0)
  {
    throw GraphException("Could not get a test out_adjlist cursor");
  }
  return out_adjlist_cursor;
}

WT_CURSOR *AdjList::get_new_random_outadj_cursor()
{
  WT_CURSOR *rand_out_adjlist_cursor = nullptr;
  int ret = _get_table_cursor(OUT_ADJLIST,
                              &rand_out_adjlist_cursor,
                              session,
                              true,
                              false,
                              opts.checkpoint_name);
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
      // CommonUtil::record_to_edge(e_cur, &found);
      get_edge_wt(e_cur, &found.edge_weight);
    }
  }
  else
  {
    found = {.src_id = OutOfBand_ID_MAX, .dst_id = OutOfBand_ID_MAX};
  }
  return found;
}

[[maybe_unused]] void AdjList::dump_table(const std::string &table_name,
                                          int num_records)
{
  if (table_name == NODE_TABLE)
  {
    std::ofstream outfile("adjlist_nodes_dump.txt");
    node_cursor->reset(node_cursor);
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
    std::ofstream outfile("adjlist_edges_dump.txt");
    edge_cursor->reset(edge_cursor);
    while (edge_cursor->next(edge_cursor) == 0 && num_records > 0)
    {
      edge found;
      num_records--;
      CommonUtil::get_key(edge_cursor, &found.src_id, &found.dst_id);
      if (opts.is_weighted)
      {
        // CommonUtil::record_to_edge(edge_cursor, &found);
        get_edge_wt(edge_cursor, &found.edge_weight);
      }
      CommonUtil::dump_edge(found, outfile);
    }
    edge_cursor->reset(edge_cursor);
  }
  else if (table_name == OUT_ADJLIST)
  {
    std::ofstream outfile("outadj_dump.txt");
    out_adjlist_cursor->reset(out_adjlist_cursor);
    while (out_adjlist_cursor->next(out_adjlist_cursor) == 0 && num_records > 0)
    {
      adjlist found;
      // found.edgelist.reserve(15000);
      num_records--;
      CommonUtil::get_key(out_adjlist_cursor, &found.node_id);
      //      std::cout << "Node ID: " << found.node_id << std::endl;
      CommonUtil::record_to_adjlist(out_adjlist_cursor, &found);
      CommonUtil::dump_adjlist(found, outfile);
    }
  }
  else if (table_name == IN_ADJLIST)
  {
    std::ofstream outfile("outadj_dump.txt");
    in_adjlist_cursor->reset(in_adjlist_cursor);
    while (in_adjlist_cursor->next(in_adjlist_cursor) == 0 && num_records > 0)
    {
      adjlist found;
      num_records--;
      CommonUtil::get_key(in_adjlist_cursor, &found.node_id);
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
int AdjList::error_check_insert_txn(int return_val)
{
  switch (return_val)
  {
    case 0:
      return 0;
    case WT_ROLLBACK:
      session->rollback_transaction(session, nullptr);
      return WT_ROLLBACK;
    case WT_DUPLICATE_KEY:
      LOG_MSG("WT_DUPLICATE_KEY in TX");
      return WT_DUPLICATE_KEY;
    case WT_NOTFOUND:
      LOG_MSG("WT_NOTFOUND in TX");
      return WT_NOTFOUND;
    default:
      DEBUG_MSG("Failed to complete action : " +
                std::string(wiredtiger_strerror(return_val)));
      session->rollback_transaction(session, nullptr);
      return WT_ROLLBACK;
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

node_id_t AdjList::get_max_node_id()
{
  WT_CURSOR *cursor = get_node_cursor();
  node_id_t to_return = 0;
  assert(cursor != nullptr);
  cursor->prev(cursor);
  CommonUtil::get_key(cursor, &to_return);
#ifdef DEBUG
  std::cout << "Max node id: " << to_return << std::endl;
#endif
  cursor->close(cursor);
  return to_return;
}

node_id_t AdjList::get_min_node_id()
{
  WT_CURSOR *cursor = get_node_cursor();
  node_id_t to_return = 0;
  assert(cursor != nullptr);
  cursor->next(cursor);
  CommonUtil::get_key(cursor, &to_return);
#ifdef DEBUG
  std::cout << "Min node id: " << to_return << std::endl;
#endif
  cursor->close(cursor);
  return to_return;
}
