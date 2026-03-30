#include "edgekey_split.h"

#include <wiredtiger.h>

#include <string>

#include "common_util.h"
#include "prop_schema.h"

using namespace std;

[[maybe_unused]] const std::string GRAPH_PREFIX = "ekey_split";

SplitEdgeKey::SplitEdgeKey(graph_opts &opt_params, WT_CONNECTION *conn)
    : GraphBase(opt_params, conn)

{
  init_cursors();
}

/**create_wt_tables
 * @brief This function is used to check if a node identified by node_id exists
 * in the node table.
 *
 * @param node_id the node_id to be searched.
 * @return true if the node is found; false otherwise.
 */

void SplitEdgeKey::create_wt_tables(graph_opts &opts, WT_CONNECTION *conn)
{
  // ******** Now set up the Edge Tables **************
  WT_SESSION *sess;
  if (CommonUtil::open_session(conn, &sess) != 0)
  {
    throw GraphException("Cannot open session");
  }

  // Table tuning parameters for better concurrent write performance:
  // - leaf_page_max=64KB: larger leaf pages reduce page splits
  // - internal_page_max=16KB: reasonable internal page size
  // - memory_page_max=10MB: defer page splits longer in memory
  // - split_pct=90: pack pages tighter before splitting
  std::string out_table_config =
      "key_format=uu,value_format=u,"
      "columns=(" + std::string(SRC) + "," + std::string(DST) + "," + std::string(ATTR) + "),"
      "leaf_page_max=64KB,"
      "internal_page_max=16KB,"
      "memory_page_max=32MB,"
      "split_pct=100";

  // Set up the out-edge table
  std::string out_table = "table:" + std::string(OUT_EDGES);
  int ret = sess->create(sess, out_table.c_str(), out_table_config.c_str());
  if (ret != 0)
  {
    throw GraphException("Failed to create OUT_EDGES table: " +
                         std::string(wiredtiger_strerror(ret)));
  }

  if (opts.is_directed)
  {
    // Set up the in-edge table with same tuning but different column order
    std::string in_table_config =
        "key_format=uu,value_format=u,"
        "columns=(" + std::string(DST) + "," + std::string(SRC) + "," + std::string(ATTR) + "),"
        "leaf_page_max=64KB,"
        "internal_page_max=16KB,"
        "memory_page_max=32MB,"
        "split_pct=100";

    std::string in_table = "table:" + std::string(IN_EDGES);
    ret = sess->create(sess, in_table.c_str(), in_table_config.c_str());
    if (ret != 0)
    {
      throw GraphException("Failed to create IN_EDGES table: " +
                           std::string(wiredtiger_strerror(ret)));
    }
  }

  // ---- COLUMNAR mode: per-type property tables with column groups ----
  if (opts.prop_mode == COLUMNAR)
  {
    // person_props: key=typed_id(Q), value=creationDate(Q) birthday(Q) gender(b)
    //               firstName(32s) lastName(32s) browserUsed(32s) locationIP(32s)
    // colgroups: temporal=(creationDate,birthday)  name=(firstName,lastName,gender)
    //            contact=(browserUsed,locationIP)
    ret = sess->create(sess, ("table:" + PERSON_PROPS_TABLE).c_str(),
        "key_format=Q,value_format=QQb32s32s32s32s,"
        "columns=(vid,creationDate,birthday,gender,firstName,lastName,browserUsed,locationIP),"
        "colgroups=(temporal,name,contact)");
    if (ret != 0)
      throw GraphException("Failed to create person_props: " + std::string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + PERSON_PROPS_TABLE + ":" + CG_TEMPORAL).c_str(),
        "columns=(creationDate,birthday)");
    if (ret != 0)
      throw GraphException("Failed to create person_props:temporal colgroup: " + std::string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + PERSON_PROPS_TABLE + ":" + CG_NAME).c_str(),
        "columns=(firstName,lastName,gender)");
    if (ret != 0)
      throw GraphException("Failed to create person_props:name colgroup: " + std::string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + PERSON_PROPS_TABLE + ":" + CG_CONTACT).c_str(),
        "columns=(browserUsed,locationIP)");
    if (ret != 0)
      throw GraphException("Failed to create person_props:contact colgroup: " + std::string(wiredtiger_strerror(ret)));

    // post_props: key=typed_id(Q), value=creationDate(Q) length(i) tag(b) content(S)
    // colgroups: temporal=(creationDate,length)  content=(tag,content)
    ret = sess->create(sess, ("table:" + POST_PROPS_TABLE).c_str(),
        "key_format=Q,value_format=QibS,"
        "columns=(vid,creationDate,length,tag,content),"
        "colgroups=(temporal,content)");
    if (ret != 0)
      throw GraphException("Failed to create post_props: " + std::string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + POST_PROPS_TABLE + ":" + CG_TEMPORAL).c_str(),
        "columns=(creationDate,length)");
    if (ret != 0)
      throw GraphException("Failed to create post_props:temporal colgroup: " + std::string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + POST_PROPS_TABLE + ":" + CG_CONTENT).c_str(),
        "columns=(tag,content)");
    if (ret != 0)
      throw GraphException("Failed to create post_props:content colgroup: " + std::string(wiredtiger_strerror(ret)));

    // person_email: key=(person_id Q, idx Q), value=email(S)
    ret = sess->create(sess, ("table:" + PERSON_EMAIL_TABLE).c_str(),
        "key_format=QQ,value_format=S,columns=(person_id,idx,email)");
    if (ret != 0)
      throw GraphException("Failed to create person_email: " + std::string(wiredtiger_strerror(ret)));

    // person_speaks: key=(person_id Q, idx Q), value=language(S)
    ret = sess->create(sess, ("table:" + PERSON_SPEAKS_TABLE).c_str(),
        "key_format=QQ,value_format=S,columns=(person_id,idx,language)");
    if (ret != 0)
      throw GraphException("Failed to create person_speaks: " + std::string(wiredtiger_strerror(ret)));

    // knows_props: key=(src Q, dst Q), value=creationDate(Q)
    ret = sess->create(sess, ("table:" + KNOWS_PROPS_TABLE).c_str(),
        "key_format=QQ,value_format=Q,"
        "columns=(src,dst,creationDate),"
        "colgroups=(temporal)");
    if (ret != 0)
      throw GraphException("Failed to create knows_props: " + std::string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + KNOWS_PROPS_TABLE + ":" + CG_TEMPORAL).c_str(),
        "columns=(creationDate)");
    if (ret != 0)
      throw GraphException("Failed to create knows_props:temporal colgroup: " + std::string(wiredtiger_strerror(ret)));

    // likes_props: same schema as knows_props
    ret = sess->create(sess, ("table:" + LIKES_PROPS_TABLE).c_str(),
        "key_format=QQ,value_format=Q,"
        "columns=(src,dst,creationDate),"
        "colgroups=(temporal)");
    if (ret != 0)
      throw GraphException("Failed to create likes_props: " + std::string(wiredtiger_strerror(ret)));
    ret = sess->create(sess, ("colgroup:" + LIKES_PROPS_TABLE + ":" + CG_TEMPORAL).c_str(),
        "columns=(creationDate)");
    if (ret != 0)
      throw GraphException("Failed to create likes_props:temporal colgroup: " + std::string(wiredtiger_strerror(ret)));
  }

  sess->close(sess, nullptr);
}

void SplitEdgeKey::init_cursors()
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

  // out_edge_cursor
  if ((ret = _get_table_cursor(OUT_EDGES,
                               &out_edge_cursor,
                               session,
                               false,
                               true,
                               opts.checkpoint_name)))
  {
    throw GraphException("Could not get an out edge cursor: " +
                         string(wiredtiger_strerror(ret)));
  }

  /**
   * in_edge_cursor points to the IN_EDGES table if the graph is directed. It
   * points to the OUT_EDGES table if the graph is undirected.
   */

  opts.is_directed ? (ret = _get_table_cursor(IN_EDGES,
                                              &in_edge_cursor,
                                              session,
                                              false,
                                              true,
                                              opts.checkpoint_name))
                   : (ret = _get_table_cursor(OUT_EDGES,
                                              &in_edge_cursor,
                                              session,
                                              false,
                                              true,
                                              opts.checkpoint_name));
  if (ret)
  {
    throw GraphException("Could not get an in edge cursor: " +
                         string(wiredtiger_strerror(ret)));
  }
  // degree_cursor: separate cursor on OUT_EDGES for node property read/write
  // (avoids position conflicts with out_edge_cursor during load)
  if ((ret = _get_table_cursor(OUT_EDGES,
                               &degree_cursor,
                               session,
                               false,
                               true,
                               opts.checkpoint_name)))
  {
    throw GraphException("Could not get degree cursor: " +
                         string(wiredtiger_strerror(ret)));
  }

  // rd_out_cursor / rd_in_cursor: dedicated read-only cursors for
  // get_out_nodes_id() and get_in_nodes_id().  These are kept separate from
  // out_edge_cursor / in_edge_cursor (which are used for writes and may hold a
  // cursor position mid-transaction) so that read traversals never disturb the
  // write cursor position.  Using pre-opened cursors here avoids the
  // _get_table_cursor overhead (cursor-cache lookup + config-string parsing)
  // on every get_out_nodes_id / get_in_nodes_id call.
  if ((ret = _get_table_cursor(OUT_EDGES,
                               &rd_out_cursor,
                               session,
                               false,
                               false,  // read-only: overwrite not needed
                               opts.checkpoint_name)))
  {
    throw GraphException("Could not open rd_out_cursor: " +
                         string(wiredtiger_strerror(ret)));
  }
  opts.is_directed
      ? (ret = _get_table_cursor(IN_EDGES,  &rd_in_cursor, session, false, false,
                                 opts.checkpoint_name))
      : (ret = _get_table_cursor(OUT_EDGES, &rd_in_cursor, session, false, false,
                                 opts.checkpoint_name));
  if (ret)
  {
    throw GraphException("Could not open rd_in_cursor: " +
                         string(wiredtiger_strerror(ret)));
  }

  // COLUMNAR mode: open per-type property table cursors
  if (opts.prop_mode == COLUMNAR)
  {
    if ((ret = _get_table_cursor(PERSON_PROPS_TABLE, &person_props_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("Could not open person_props cursor: " + string(wiredtiger_strerror(ret)));
    if ((ret = _get_table_cursor(POST_PROPS_TABLE, &post_props_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("Could not open post_props cursor: " + string(wiredtiger_strerror(ret)));
    if ((ret = _get_table_cursor(KNOWS_PROPS_TABLE, &knows_props_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("Could not open knows_props cursor: " + string(wiredtiger_strerror(ret)));
    if ((ret = _get_table_cursor(LIKES_PROPS_TABLE, &likes_props_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("Could not open likes_props cursor: " + string(wiredtiger_strerror(ret)));
    if ((ret = _get_table_cursor(PERSON_EMAIL_TABLE, &person_email_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("Could not open person_email cursor: " + string(wiredtiger_strerror(ret)));
    if ((ret = _get_table_cursor(PERSON_SPEAKS_TABLE, &person_speaks_cursor,
                                 session, false, true, opts.checkpoint_name)))
      throw GraphException("Could not open person_speaks cursor: " + string(wiredtiger_strerror(ret)));
  }
}

/**
 * This method adds a node to the "Out_Edge" table. In case the graph is
 * undirected, everything is fine. In case the graph is directed, we make the
 * node entries only in the out-edge table. The in-edge table only holds the
 * reverse edges.
 * @param to_insert
 * @param is_bulk
 * @return
 */
int SplitEdgeKey::add_node(node to_insert, bool is_bulk)
{
  session->begin_transaction(session, "isolation=snapshot");
  CommonUtil::ekey_set_node_key(out_edge_cursor, to_insert.id);
  if (out_edge_cursor->search(out_edge_cursor) == 0)
  {
    //node exists, rollback and return WT_DUPLICATE_KEY
    session->rollback_transaction(session, nullptr);
    return WT_DUPLICATE_KEY;
  }else{
    //node doesn't exist, insert it with degree 0
    if (opts.read_optimize)
    {
      ekey_set_node_value(out_edge_cursor, to_insert.in_degree, to_insert.out_degree);
    }
    else
    {
      ekey_set_node_value(out_edge_cursor, 0, 0);
    }
    
    int ret = out_edge_cursor->insert(out_edge_cursor);
    if (ret != 0)
    {
      session->rollback_transaction(session, nullptr);
      return ret;
    }
  }
  session->commit_transaction(session, nullptr);
  GraphBase::increment_nodes(1);
  return 0;
}

/**
 * This method adds a node to the out-edge table, but must be called from within
 * a running transaction. It is used in the add_edge method where we must check
 * and add the src and dst nodes if they don't exist.
 *
 * OPTIMIZATION: Degree updates are now inlined here to avoid a separate
 * search operation in update_node_degree.
 *
 * @param to_insert
 * @return
 */
int SplitEdgeKey::add_node_txn(node to_insert,
                               int *num_nodes_added_ptr,
                               int32_t indeg_change,
                               int32_t outdeg_change)
{
  CommonUtil::ekey_set_node_key(out_edge_cursor, to_insert.id);
  if (out_edge_cursor->search(out_edge_cursor) == 0)
  {
    // Node exists - inline the degree update here to avoid extra search
    degree_t in_deg, out_deg;
    ekey_get_node_value(out_edge_cursor, &in_deg, &out_deg);
    ekey_set_node_value(out_edge_cursor, in_deg + indeg_change, out_deg + outdeg_change);
    int ret = out_edge_cursor->update(out_edge_cursor);
    return error_check_insert_txn(ret, false);
  }
  else
  {
    // Node doesn't exist - insert it with initial degrees
    CommonUtil::ekey_set_node_key(out_edge_cursor, to_insert.id);
    if (opts.read_optimize)
    {
      if (indeg_change > 0 || outdeg_change > 0)
      {
        ekey_set_node_value(out_edge_cursor, indeg_change, outdeg_change);
      }
      else
      {
        ekey_set_node_value(out_edge_cursor, 0, 0);
      }
    }
    else
    {
      ekey_set_node_value(out_edge_cursor, 0, 0);
    }
    int insert_result = out_edge_cursor->insert(out_edge_cursor);
    int txn_result = error_check_insert_txn(insert_result, false);
    if (!txn_result) {
      (*num_nodes_added_ptr)++;
    }
    return txn_result;
  }
}

bool SplitEdgeKey::has_node(node_id_t node_id)
{
  int ret;
  CommonUtil::ekey_set_node_key(out_edge_cursor, node_id);
  ret = out_edge_cursor->search(out_edge_cursor);
  out_edge_cursor->reset(out_edge_cursor);
  return (ret == 0);
}

int SplitEdgeKey::add_edge(edge to_insert, bool is_bulk)
{
  (void)is_bulk;
#ifdef DEBUG
  std::cout << "Adding edge (" << to_insert.src_id << ", " << to_insert.dst_id
            << ", " << to_insert.edge_weight << ")\n";
#endif
  int num_nodes_to_add = 0;
  int num_edges_to_add = 0;
  int ret;
  session->begin_transaction(session, "isolation=snapshot");
  node src{.id = to_insert.src_id};
  opts.is_directed ? (ret = add_node_txn(src, &num_nodes_to_add, 0, 1))
                   : (ret = add_node_txn(src, &num_nodes_to_add, 1, 1));
  if (ret == 0)
    LOG_MSG("added node {}", to_insert.src_id);
  else if (ret == WT_DUPLICATE_KEY)
  {
    LOG_MSG("Node {} already exists, updated degrees.", to_insert.src_id);
  }
  else if (ret == WT_ROLLBACK)
  {
    return WT_ROLLBACK;
  }

  node dst{.id = to_insert.dst_id};
  opts.is_directed ? (ret = add_node_txn(dst, &num_nodes_to_add, 1, 0))
                   : (ret = add_node_txn(dst, &num_nodes_to_add, 1, 1));
  if (ret == 0)
  {
    LOG_MSG("Added node {}", to_insert.dst_id);
  }
  else if (ret == WT_DUPLICATE_KEY)
  {
    LOG_MSG("Node {} already exists, updated degrees.", to_insert.dst_id);
  }
  else if (ret == WT_ROLLBACK)
  {
    return WT_ROLLBACK;
  }

  // Now add the edge into out-edges table.
  // Check for a duplicate edge first: if the key already exists, roll back
  // the entire transaction (which also undoes the degree increments above)
  // and return WT_DUPLICATE_KEY to the caller.
  CommonUtil::ekey_set_edge_key(out_edge_cursor, to_insert.src_id, to_insert.dst_id);
  if (out_edge_cursor->search(out_edge_cursor) == 0)
  {
    session->rollback_transaction(session, nullptr);
    return WT_DUPLICATE_KEY;
  }

  if (opts.is_weighted)
  {
    ekey_set_edge_value(out_edge_cursor, to_insert.edge_weight);
  }
  else if (opts.has_edge_props && opts.prop_mode != COLUMNAR)
  {
    // EMBEDDED mode: store a 1-byte sentinel so set_edge_properties can
    // overwrite the value later.  Never pass size=0 to WiredTiger (double-free).
    // In COLUMNAR mode this branch is skipped because set_edge_properties writes
    // to separate per-type tables and never touches the edge table value, so we
    // must store a properly-sized weight here or EdgeCursor will throw on read.
    static const uint8_t placeholder = 0;
    WT_ITEM empty = { &placeholder, 1 };
    out_edge_cursor->set_value(out_edge_cursor, &empty);
  }
  else
  {
    ekey_set_edge_value(out_edge_cursor, 0.0);
  }

  if (ret = error_check_insert_txn(out_edge_cursor->insert(out_edge_cursor), false))
    return ret;

  // Insert into the in-edges table
  CommonUtil::ekey_set_edge_key(in_edge_cursor, to_insert.dst_id, to_insert.src_id);
  if (opts.is_weighted)
  {
    ekey_set_edge_value(in_edge_cursor, to_insert.edge_weight);
  }
  else if (opts.has_edge_props && opts.prop_mode != COLUMNAR)
  {
    static const uint8_t placeholder = 0;
    WT_ITEM empty = { &placeholder, 1 };
    in_edge_cursor->set_value(in_edge_cursor, &empty);
  }
  else
  {
    ekey_set_edge_value(in_edge_cursor, 0.0);
  }
  if (ret = error_check_insert_txn(in_edge_cursor->insert(in_edge_cursor), false))
    return ret;

  num_edges_to_add++;
  session->commit_transaction(session, nullptr);
  GraphBase::increment_nodes(num_nodes_to_add);
  GraphBase::increment_edges(num_edges_to_add);
  return 0;
}

void SplitEdgeKey::set_node_properties(node_id_t id,
                                        const uint8_t *prop_data,
                                        size_t prop_size)
{
  if (opts.prop_mode == COLUMNAR)
  {
    switch (VTYPE_OF(id))
    {
      case VT_PERSON: {
        uint64_t    cDate   = (uint64_t)SNBPersonSchema::get_creation_date(prop_data);
        uint64_t    bday    = (uint64_t)SNBPersonSchema::get_birthday(prop_data);
        int8_t      gender  = SNBPersonSchema::get_gender(prop_data);
        const char *fname   = SNBPersonSchema::get_first_name(prop_data);
        const char *lname   = SNBPersonSchema::get_last_name(prop_data);
        const char *browser = SNBPersonSchema::get_browser_used(prop_data);
        const char *loc_ip  = SNBPersonSchema::get_location_ip(prop_data);
        person_props_cursor->set_key(person_props_cursor, (uint64_t)id);
        person_props_cursor->set_value(person_props_cursor,
                                       cDate, bday, gender,
                                       fname, lname, browser, loc_ip);
        int ret = person_props_cursor->insert(person_props_cursor);
        if (ret == WT_DUPLICATE_KEY)
        {
          person_props_cursor->set_key(person_props_cursor, (uint64_t)id);
          person_props_cursor->set_value(person_props_cursor,
                                         cDate, bday, gender,
                                         fname, lname, browser, loc_ip);
          ret = person_props_cursor->update(person_props_cursor);
        }
        if (ret != 0)
          throw GraphException("set_node_properties: person failed for " +
                               std::to_string(id) + ": " + wiredtiger_strerror(ret));
        break;
      }
      case VT_POST: {
        uint64_t    cDate   = (uint64_t)SNBPostSchema::get_creation_date(prop_data);
        int32_t     length  = SNBPostSchema::get_length(prop_data);
        int8_t      tag     = SNBPostSchema::get_tag(prop_data);
        const char *content = SNBPostSchema::get_content(prop_data, prop_size);
        post_props_cursor->set_key(post_props_cursor, (uint64_t)id);
        post_props_cursor->set_value(post_props_cursor, cDate, length, tag, content);
        int ret = post_props_cursor->insert(post_props_cursor);
        if (ret == WT_DUPLICATE_KEY)
        {
          post_props_cursor->set_key(post_props_cursor, (uint64_t)id);
          post_props_cursor->set_value(post_props_cursor, cDate, length, tag, content);
          ret = post_props_cursor->update(post_props_cursor);
        }
        if (ret != 0)
          throw GraphException("set_node_properties: post failed for " +
                               std::to_string(id) + ": " + wiredtiger_strerror(ret));
        break;
      }
      default:
        break;
    }
    return;
  }

  // EMBEDDED / SPLIT mode: node sentinel in OUT_EDGES at (MAKE_EKEY(id), 0).
  CommonUtil::ekey_set_node_key(degree_cursor, id);
  int ret = degree_cursor->search(degree_cursor);
  if (ret != 0)
    throw GraphException("set_node_properties: node not found: " +
                         std::to_string(id));

  degree_t in_deg = 0, out_deg = 0;
  ekey_get_node_value(degree_cursor, &in_deg, &out_deg);

  size_t total = sizeof(degree_t) * 2 + prop_size;
  std::vector<uint8_t> buf(total);
  memcpy(buf.data(), &in_deg, sizeof(degree_t));
  memcpy(buf.data() + sizeof(degree_t), &out_deg, sizeof(degree_t));
  if (prop_size > 0)
    memcpy(buf.data() + sizeof(degree_t) * 2, prop_data, prop_size);

  WT_ITEM item;
  item.data = buf.data();
  item.size = total;
  degree_cursor->set_value(degree_cursor, &item);
  if (degree_cursor->update(degree_cursor) != 0)
    throw GraphException("set_node_properties: update failed for node " +
                         std::to_string(id));
}

prop_blob SplitEdgeKey::get_node_properties(node_id_t id)
{
  if (opts.prop_mode == COLUMNAR)
  {
    switch (VTYPE_OF(id))
    {
      case VT_PERSON: {
        person_props_cursor->set_key(person_props_cursor, (uint64_t)id);
        if (person_props_cursor->search(person_props_cursor) != 0)
          return {nullptr, 0};
        uint64_t    cDate, bday;
        int8_t      gender;
        const char *fname, *lname, *browser, *loc_ip;
        person_props_cursor->get_value(person_props_cursor,
                                       &cDate, &bday, &gender,
                                       &fname, &lname, &browser, &loc_ip);
        uint8_t *buf = new uint8_t[SNBPersonSchema::TOTAL_SIZE]();
        SNBPersonSchema::set_creation_date(buf, (int64_t)cDate);
        SNBPersonSchema::set_birthday(buf, (int64_t)bday);
        SNBPersonSchema::set_gender(buf, gender);
        SNBPersonSchema::set_first_name(buf, fname);
        SNBPersonSchema::set_last_name(buf, lname);
        SNBPersonSchema::set_browser_used(buf, browser);
        SNBPersonSchema::set_location_ip(buf, loc_ip);
        return {buf, SNBPersonSchema::TOTAL_SIZE};
      }
      case VT_POST: {
        post_props_cursor->set_key(post_props_cursor, (uint64_t)id);
        if (post_props_cursor->search(post_props_cursor) != 0)
          return {nullptr, 0};
        uint64_t cDate; int32_t length; int8_t tag; const char *content;
        post_props_cursor->get_value(post_props_cursor, &cDate, &length, &tag, &content);
        size_t content_len = (content && *content) ? strlen(content) : 0;
        size_t total = SNBPostSchema::TOTAL_SIZE + content_len + 1;
        uint8_t *buf = new uint8_t[total]();
        SNBPostSchema::set_creation_date(buf, (int64_t)cDate);
        SNBPostSchema::set_length(buf, length);
        SNBPostSchema::set_tag(buf, tag);
        SNBPostSchema::set_content(buf, content ? content : "");
        return {buf, total};
      }
      default:
        return {nullptr, 0};
    }
  }

  // EMBEDDED / SPLIT mode
  CommonUtil::ekey_set_node_key(degree_cursor, id);
  if (degree_cursor->search(degree_cursor) != 0)
    return {nullptr, 0};

  WT_ITEM item;
  degree_cursor->get_value(degree_cursor, &item);

  constexpr size_t HDR = sizeof(degree_t) * 2;
  if (item.size <= HDR)
    return {nullptr, 0};

  size_t prop_size = item.size - HDR;
  uint8_t *copy = new uint8_t[prop_size];
  memcpy(copy, static_cast<const uint8_t *>(item.data) + HDR, prop_size);
  return {copy, prop_size};
}

// Route (src,dst) to the right edge prop cursor based on vertex types.
// Returns nullptr for hasCreator (POST→PERSON): no properties to store.
static WT_CURSOR *route_edge_cursor(node_id_t src, node_id_t dst,
                                    WT_CURSOR *knows, WT_CURSOR *likes)
{
  uint8_t s = (uint8_t)VTYPE_OF(src);
  uint8_t d = (uint8_t)VTYPE_OF(dst);
  if (s == VT_PERSON && d == VT_PERSON) return knows;
  if (s == VT_PERSON && d == VT_POST)   return likes;
  return nullptr;  // POST→PERSON = hasCreator, no props
}

void SplitEdgeKey::set_edge_properties(node_id_t src,
                                        node_id_t dst,
                                        const uint8_t *data,
                                        size_t size)
{
  if (opts.prop_mode == COLUMNAR)
  {
    WT_CURSOR *cur = route_edge_cursor(src, dst, knows_props_cursor, likes_props_cursor);
    if (cur == nullptr || data == nullptr || size == 0)
      return;  // hasCreator or no-prop edge
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
      throw GraphException("set_edge_properties: failed (" +
                           std::to_string(src) + ", " + std::to_string(dst) + "): " +
                           wiredtiger_strerror(ret));
    return;
  }

  // EMBEDDED / SPLIT mode
  static const uint8_t placeholder = 0;
  WT_ITEM item;
  if (data == nullptr || size == 0) {
    item.data = &placeholder;
    item.size = 1;
  } else {
    item.data = data;
    item.size = size;
  }

  CommonUtil::ekey_set_edge_key(out_edge_cursor, src, dst);
  if (out_edge_cursor->search(out_edge_cursor) != 0)
    throw GraphException("set_edge_properties: edge not found (" +
                         std::to_string(src) + ", " + std::to_string(dst) + ")");
  out_edge_cursor->set_value(out_edge_cursor, &item);
  if (out_edge_cursor->update(out_edge_cursor) != 0)
    throw GraphException("set_edge_properties: update failed on OUT_EDGES");

  CommonUtil::ekey_set_edge_key(in_edge_cursor, dst, src);
  if (in_edge_cursor->search(in_edge_cursor) != 0)
    throw GraphException("set_edge_properties: reverse edge not found (" +
                         std::to_string(dst) + ", " + std::to_string(src) + ")");
  in_edge_cursor->set_value(in_edge_cursor, &item);
  if (in_edge_cursor->update(in_edge_cursor) != 0)
    throw GraphException("set_edge_properties: update failed on IN_EDGES");
}

prop_blob SplitEdgeKey::get_edge_properties(node_id_t src, node_id_t dst)
{
  if (opts.prop_mode == COLUMNAR)
  {
    WT_CURSOR *cur = route_edge_cursor(src, dst, knows_props_cursor, likes_props_cursor);
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
  CommonUtil::ekey_set_edge_key(out_edge_cursor, src, dst);
  if (out_edge_cursor->search(out_edge_cursor) != 0)
    return {nullptr, 0};

  WT_ITEM item;
  out_edge_cursor->get_value(out_edge_cursor, &item);
  if (item.size <= 1)
    return {nullptr, 0};

  uint8_t *copy = new uint8_t[item.size];
  memcpy(copy, item.data, item.size);
  return {copy, item.size};
}

WT_CURSOR *SplitEdgeKey::open_colgroup_cursor(const std::string &table,
                                               const std::string &colgroup)
{
  if (opts.prop_mode != COLUMNAR)
    throw GraphException("open_colgroup_cursor: only valid in COLUMNAR mode");
  std::string uri = "colgroup:" + table + ":" + colgroup;
  WT_CURSOR *cur = nullptr;
  int ret = session->open_cursor(session, uri.c_str(), nullptr, nullptr, &cur);
  if (ret != 0)
    throw GraphException("open_colgroup_cursor: failed to open " + uri + ": " +
                         wiredtiger_strerror(ret));
  return cur;
}

void SplitEdgeKey::add_person_email(node_id_t person_id, uint64_t idx, const char *email)
{
  if (opts.prop_mode != COLUMNAR || !person_email_cursor) return;
  person_email_cursor->set_key(person_email_cursor, (uint64_t)person_id, idx);
  person_email_cursor->set_value(person_email_cursor, email ? email : "");
  person_email_cursor->insert(person_email_cursor);
}

void SplitEdgeKey::add_person_language(node_id_t person_id, uint64_t idx, const char *lang)
{
  if (opts.prop_mode != COLUMNAR || !person_speaks_cursor) return;
  person_speaks_cursor->set_key(person_speaks_cursor, (uint64_t)person_id, idx);
  person_speaks_cursor->set_value(person_speaks_cursor, lang ? lang : "");
  person_speaks_cursor->insert(person_speaks_cursor);
}

static std::vector<std::string> scan_secondary_table(WT_CURSOR *cur, uint64_t person_id)
{
  std::vector<std::string> result;
  if (!cur) return result;
  int cmp = 0;
  cur->set_key(cur, person_id, (uint64_t)0);
  int ret = cur->search_near(cur, &cmp);
  if (ret == 0 && cmp < 0) ret = cur->next(cur);
  while (ret == 0) {
    uint64_t pid, idx;
    cur->get_key(cur, &pid, &idx);
    if (pid != person_id) break;
    const char *val;
    cur->get_value(cur, &val);
    result.emplace_back(val ? val : "");
    ret = cur->next(cur);
  }
  return result;
}

std::vector<std::string> SplitEdgeKey::get_person_emails(node_id_t person_id)
{
  return scan_secondary_table(person_email_cursor, (uint64_t)person_id);
}

std::vector<std::string> SplitEdgeKey::get_person_languages(node_id_t person_id)
{
  return scan_secondary_table(person_speaks_cursor, (uint64_t)person_id);
}

/**
 * Check if the edge between src_id and dst_id exists in the graph.
 * @param src_id
 * @param dst_id
 * @return
 */
edge SplitEdgeKey::get_edge(node_id_t src_id, node_id_t dst_id)
{
  edge found = {};

  CommonUtil::ekey_set_edge_key(out_edge_cursor, src_id, dst_id);
  if (out_edge_cursor->search(out_edge_cursor) == 0)
  {
    found.src_id = src_id;
    found.dst_id = dst_id;
    if (opts.is_weighted)
      // CommonUtil::record_to_edge_ekey(out_edge_cursor, &found);
      ekey_get_edge_value(out_edge_cursor, &found.edge_weight);
  }
  return found;
}

/**
 * @brief This function is used for graphalytics workloads. for each edge, read
 * its current weight. If the current weight exist, add e's weight with current
 * weight and update the edge weight otherwise insert e as the new edge.
 *
 * @param to_update weighted edge to insert/update.
 * @return true if the operation succeeds
 */
bool SplitEdgeKey::update_edge(edge to_update)
{
  int ret;
  session->begin_transaction(session, "isolation=snapshot");
  CommonUtil::ekey_set_edge_key(out_edge_cursor, to_update.src_id, to_update.dst_id);
  //edge found, update the weight
  if (out_edge_cursor->search(out_edge_cursor) == 0)
  {
    edge found = {0};
    ekey_get_edge_value(out_edge_cursor, &found.edge_weight);
    found.edge_weight += to_update.edge_weight;
    CommonUtil::ekey_set_edge_key(
        out_edge_cursor, to_update.src_id, to_update.dst_id);
    ekey_set_edge_value(out_edge_cursor, found.edge_weight);
    ret = error_check_insert_txn(out_edge_cursor->update(out_edge_cursor), false);
    if (ret) return false; // OK, error_check rolls back TXN

    //do the reverse edge too
    CommonUtil::ekey_set_edge_key(
        in_edge_cursor, to_update.dst_id, to_update.src_id);
    ekey_set_edge_value(in_edge_cursor, found.edge_weight);
    ret = error_check_insert_txn(in_edge_cursor->update(in_edge_cursor), false);
    if (ret) return false; // OK, error_check rolls back TXN

    session->commit_transaction(session, nullptr);
    return true;
  }
  else{
    session->rollback_transaction(session, nullptr);
    //edge not found, insert it
    ret = add_edge(to_update, false);
    return (ret == 0);
  }
}

std::vector<node> SplitEdgeKey::get_nodes()
{
  std::vector<node> nodes;

  int search_exact;
  // Start by positioning cursor to (OutOfBand_ID_MIN, OutOfBand_ID_MIN)
  // We need to use ekey_set_key because we are doing a sentinel search 
  CommonUtil::ekey_set_key(out_edge_cursor, OutOfBand_ID_MIN, OutOfBand_ID_MIN);
  out_edge_cursor->search_near(out_edge_cursor, &search_exact);

  if (search_exact < 0)
  {
    // Position to first record if search_near returns negative
    if (out_edge_cursor->next(out_edge_cursor) != 0)
    {
      out_edge_cursor->reset(out_edge_cursor);
      return nodes;  // Empty graph
    }
  }

  // Keep doing search_near in a loop till there are no more nodes found
  node_id_t src, dst;
  bool continue_search = true;

  while (continue_search)
  {
    // Get current key. We use ekey_get_key because we need to check for
    // OutOfBand_ID_MIN in dst to identify node entries.
    if (CommonUtil::ekey_get_key(out_edge_cursor, &src, &dst) != 0)
    {
      break;  // Error getting key
    }

    if (dst == OutOfBand_ID_MIN)
    {
      // Found a node entry
      node n;
      n.id = src;
      if (opts.read_optimize)
      {
        // out_edge_cursor->get_value(
        //     out_edge_cursor, &n.in_degree, &n.out_degree);
        ekey_get_node_value(out_edge_cursor, &n.in_degree, &n.out_degree);
      }
      else
      {
        n.in_degree = 0;
        n.out_degree = 0;
      }
      nodes.push_back(n);
    }

    // Search for next node by setting key to (src+1, OutOfBand_ID_MIN)
    // This works for both cases: found a node (src) or found an edge (src)
    CommonUtil::ekey_set_key(out_edge_cursor, src + 1, OutOfBand_ID_MIN);
    if (out_edge_cursor->search_near(out_edge_cursor, &search_exact) != 0)
    {
      continue_search = false;  // No more records
    }
    else if (search_exact < 0)
    {
      // Position to next record if search_near returns negative
      if (out_edge_cursor->next(out_edge_cursor) != 0)
      {
        continue_search = false;  // No more records
      }
    }
  }

  out_edge_cursor->reset(out_edge_cursor);
  return nodes;
}

std::vector<edge> SplitEdgeKey::get_edges()
{
  std::vector<edge> edges;

  while (out_edge_cursor->next(out_edge_cursor) == 0)
  {
    edge found = {0};
    //Since we are comparing for OutOfBand_ID_MIN, we use ekey_get_key not
    //ekey_get_edge_key
    CommonUtil::ekey_get_key(out_edge_cursor, &found.src_id, &found.dst_id);
    if (found.dst_id != OutOfBand_ID_MIN)
    {
      if (opts.is_weighted)
      {
        ekey_get_edge_value(out_edge_cursor, &found.edge_weight);
      }
      edges.push_back(found);
    }
  }
  out_edge_cursor->reset(out_edge_cursor);
  return edges;
}

bool SplitEdgeKey::has_edge(node_id_t src_id, node_id_t dst_id)
{
  int ret;
  //because edges need MAKE_EDGE_KEY on both.
  CommonUtil::ekey_set_edge_key(out_edge_cursor, src_id, dst_id);
  ret = out_edge_cursor->search(out_edge_cursor);
  out_edge_cursor->reset(out_edge_cursor);
  return (ret == 0);
}

node SplitEdgeKey::get_node(node_id_t node_id)
{
  CommonUtil::ekey_set_node_key(out_edge_cursor, node_id);
  node found = {0};
  if (out_edge_cursor->search(out_edge_cursor) == 0)
  {
    found.id = node_id;
    // CommonUtil::record_to_node_ekey(out_edge_cursor, &found);
    ekey_get_node_value(out_edge_cursor, &found.in_degree, &found.out_degree);
  }
  out_edge_cursor->reset(out_edge_cursor);
  return found;
}

node SplitEdgeKey::get_random_node()
{
  node rando = {0};
  int ret = 0;
  if (random_node_cursor == nullptr)
  {
    ret = _get_table_cursor(OUT_EDGES,
                            &random_node_cursor,
                            session,
                            true,
                            true,
                            opts.checkpoint_name);
  }
  if (ret != 0)
  {
    throw GraphException("Could not get a random cursor to the node table");
  }
  while ((ret = random_node_cursor->next(random_node_cursor)) == 0)
  {
    node_id_t src, dst;
    ret = CommonUtil::ekey_get_key(random_node_cursor, &src, &dst);
    if (ret != 0)
    {
      break;
    }
    if (dst == OutOfBand_ID_MIN)
    {
      rando.id = src;
      ekey_get_node_value(
          random_node_cursor, &rando.in_degree, &rando.out_degree);
      break;
    }
  }
  if (ret != 0)
  {
    throw GraphException("Issue with random cursor iteration:" +
                         string(wiredtiger_strerror(ret)));
  }
  return rando;
}
degree_t SplitEdgeKey::get_in_degree(node_id_t node_id)
{
  degree_t in_deg = 0;
  //if read_optimize is on, we have in-degree stored in the node entry.
  if (opts.read_optimize)
  {
    CommonUtil::ekey_set_node_key(out_edge_cursor, node_id);
    if (out_edge_cursor->search(out_edge_cursor) != 0)
    {
      out_edge_cursor->reset(out_edge_cursor);
      throw GraphException("Node with ID " + std::to_string(node_id) +
                           " does not exist");
    }
    node found = {0};
    ekey_get_node_value(out_edge_cursor, &found.in_degree, &found.out_degree);
    out_edge_cursor->reset(out_edge_cursor);
    return found.in_degree;
  }
  else //search by scanning the in-edge table
  {
    CommonUtil::ekey_set_node_key(in_edge_cursor, node_id);
    int search_exact;  // this can not be 0 because we are not creating
                       // node_id, 0 entires for nodes in this table.
    in_edge_cursor->search_near(in_edge_cursor, &search_exact);
    if (search_exact < 0)
    {
      // we should call next and see if the next edge is relevant
      in_edge_cursor->next(in_edge_cursor);
      node_id_t src, dst;
      CommonUtil::ekey_get_edge_key(in_edge_cursor, &dst, &src);
      if (dst != node_id)
      {
        in_edge_cursor->reset(in_edge_cursor);
        return 0;  // no in-edges for this node.
      }
    }
    if (search_exact == 0)
    {
      in_edge_cursor->next(in_edge_cursor);  // to position to the next edge
    }
    // Now the case where search_exact >0
    do
    {
      node_id_t src, dst;
      CommonUtil::ekey_get_edge_key(in_edge_cursor, &dst, &src);
      if (dst != node_id)
      {
        break;  // reached next node
      }
      in_deg++;
    } while (in_edge_cursor->next(in_edge_cursor) == 0);
    in_edge_cursor->reset(in_edge_cursor);
  }
  return in_deg;
}

degree_t SplitEdgeKey::get_out_degree(node_id_t node_id)
{
  degree_t out_deg = 0;
  CommonUtil::ekey_set_node_key(out_edge_cursor, node_id);
  int ret = out_edge_cursor->search(out_edge_cursor);
  if (ret == 0)  // The node exists
  {
    // If we are using read optimization, we can get the in/out degree directly
    if (opts.read_optimize)
    {
      node found = {0};
      found.id = node_id;
      ekey_get_node_value(out_edge_cursor, &found.in_degree, &found.out_degree);
      out_edge_cursor->reset(out_edge_cursor);
      out_deg = found.out_degree;
    }
    else // we have to scan the out-edge table to count the out-degree
    {
      node_id_t src, dst;
      do
      {
        out_edge_cursor->next(out_edge_cursor);  // skip to the out_edges
        //we need to use ekey_get_key because we are checking for
        //OutOfBand_ID_MIN in dst to identify node entries.
        CommonUtil::ekey_get_key(out_edge_cursor, &src, &dst);
        if (src == node_id)
        {
          out_deg++;
        }
        else
        {
          break;
        }
      } while (src == node_id && dst != OutOfBand_ID_MIN);
      out_edge_cursor->reset(out_edge_cursor);
    }
  }
  else
  {
    out_edge_cursor->reset(out_edge_cursor);
    throw GraphException("Node with ID " + std::to_string(node_id) +
                         " does not exist");
  }

  return out_deg;
}

std::vector<edge> SplitEdgeKey::get_out_edges(node_id_t node_id)
{
  std::vector<edge> out_edges;
  CommonUtil::ekey_set_node_key(out_edge_cursor, node_id);

  int temp;
  if (out_edge_cursor->search(out_edge_cursor) == 0)
  {
    while (true)
    {
      edge found;
      if (out_edge_cursor->next(out_edge_cursor) != 0) break;
      CommonUtil::ekey_get_key(out_edge_cursor, &found.src_id, &found.dst_id);
      if ((found.src_id == node_id) & (found.dst_id != OutOfBand_ID_MIN))
      {
        ekey_get_edge_value(out_edge_cursor, &found.edge_weight);
        out_edges.push_back(found);
      }
      else
      {
        break;
      }
    }
  }
  else
  {
    throw GraphException("The node " + to_string(node_id) +
                         " does not exist in the graph");
  }
  out_edge_cursor->reset(out_edge_cursor);
  return out_edges;
}
std::vector<node> SplitEdgeKey::get_out_nodes(node_id_t node_id)
{
  std::vector<node> out_nodes;
  WT_CURSOR *e_cur;  // need new cursor because we will be using the class
                     // cursor for get_node
  if (_get_table_cursor(
          OUT_EDGES, &e_cur, session, false, true, opts.checkpoint_name) != 0)
  {
    throw GraphException("Could not get a cursor to the OutEdge table");
  }

  CommonUtil::ekey_set_node_key(e_cur, node_id);

  if (e_cur->search(e_cur) == 0)
  {
    while (true)
    {
      node found;
      node_id_t src_id, dst_id;
      if (e_cur->next(e_cur) != 0)
      {
        break;
      }
      CommonUtil::ekey_get_key(e_cur, &src_id, &dst_id);
      // if (src_id == MAKE_EKEY(node_id))
      if (src_id == node_id)
      {
        // found = get_node(OG_KEY(dst_id));
        found = get_node(dst_id);
        out_nodes.push_back(found);
        // CommonUtil::dump_node(found);
      }
      else
      {
        break;
      }
    }
  }
  else
  {
    throw GraphException("The node " + to_string(node_id) +
                         " does not exist in the graph");
  }
  e_cur->close(e_cur);
  return out_nodes;
}
std::vector<node_id_t> SplitEdgeKey::get_out_nodes_id(node_id_t node_id)
{
  std::vector<node_id_t> out_nodes_id;
  // Use the pre-opened rd_out_cursor instead of opening a new cursor per call.
  // reset() at the end releases the page pin and returns the cursor to a clean
  // state for the next caller, without closing and re-opening from the cache.
  WT_CURSOR *e_cur = rd_out_cursor;
  CommonUtil::ekey_set_node_key(e_cur, node_id);
  if (e_cur->search(e_cur) == 0)
  {
    while (true)
    {
      node_id_t src_id, dst_id;
      if (e_cur->next(e_cur) != 0)
      {
        break;
      }
      CommonUtil::ekey_get_key(e_cur, &src_id, &dst_id);

      if (src_id == node_id)
      {
        out_nodes_id.push_back(dst_id);
      }
      else
      {
        break;
      }
    }
  }
  else
  {
    e_cur->reset(e_cur);
    throw GraphException("The node " + to_string(node_id) +
                         " does not exist in the graph");
  }
  e_cur->reset(e_cur);
  return out_nodes_id;
}
std::vector<edge> SplitEdgeKey::get_in_edges(node_id_t node_id)
{
  if (!has_node(node_id))
  {
    throw GraphException("The node " + to_string(node_id) +
                         " does not exist in the graph");
  }
  std::vector<edge> in_edges;
  CommonUtil::ekey_set_node_key(in_edge_cursor, node_id);

  int temp;
  int search_exact;
  in_edge_cursor->search_near(in_edge_cursor, &search_exact);
  if (search_exact < 0)
  {
    // we should call next and see if the next edge is relevant
    in_edge_cursor->next(in_edge_cursor);
    node_id_t src, dst;
    CommonUtil::ekey_get_key(in_edge_cursor, &dst, &src);
    if (dst != node_id)
    {
      in_edge_cursor->reset(in_edge_cursor);
      return in_edges;  // no in-edges for this node.
    }
  }
  if (search_exact == 0)
  {
    in_edge_cursor->next(in_edge_cursor);  // to position to the next edge
  }

  do
  {
    edge found;
    CommonUtil::ekey_get_key(in_edge_cursor, &found.dst_id, &found.src_id);
    if ((found.dst_id == node_id) & (found.src_id != OutOfBand_ID_MIN))
    {
      ekey_get_edge_value(in_edge_cursor, &found.edge_weight);
      in_edges.push_back(found);
    }
    else
    {
      break;
    }
  } while (in_edge_cursor->next(in_edge_cursor) == 0);

  in_edge_cursor->reset(in_edge_cursor);
  return in_edges;
}
std::vector<node> SplitEdgeKey::get_in_nodes(node_id_t node_id)
{
  if (!has_node(node_id))
  {
    throw GraphException("The node " + to_string(node_id) +
                         " does not exist in the graph");
  }
  std::vector<node> in_nodes;
  WT_CURSOR *in_cur;  // need new cursor because we will be using the class
                      // cursor for get_node
  int ret;
  opts.is_directed
      ? (ret = _get_table_cursor(
             IN_EDGES, &in_cur, session, false, true, opts.checkpoint_name))
      : (ret = _get_table_cursor(
             OUT_EDGES, &in_cur, session, false, true, opts.checkpoint_name));
  if (ret != 0)
  {
    LOG_MSG("Failed to get a cursor to the {} table",
            (opts.is_directed ? "InEdges" : "OutEdges"));
    throw GraphException("Could not get a cursor in get_in_nodes");
  }
  int search_exact;
  CommonUtil::ekey_set_node_key(in_cur, node_id);
  in_cur->search_near(in_cur, &search_exact);
  if (search_exact <= 0)
  {
    in_cur->next(in_cur);  // position to the first in-edge of the node
  }

  do
  {
    node found;
    node_id_t src_id, dst_id;
    CommonUtil::ekey_get_key(in_cur, &dst_id, &src_id);
    #ifdef DEBUG
    std::cout << "src_id: " << src_id << " dst_id: " << dst_id << std::endl;
    #endif
    if (dst_id == node_id)
    {
      found = get_node(src_id);
      in_nodes.push_back(found);
      CommonUtil::dump_node(found);
    }
    else
    {
      break;
    }
  } while (in_cur->next(in_cur) == 0);
  in_cur->close(in_cur);
  return in_nodes;
}

std::vector<node_id_t> SplitEdgeKey::get_in_nodes_id(node_id_t node_id)
{
  if (!has_node(node_id))
  {
    throw GraphException("The node " + to_string(node_id) +
                         " does not exist in the graph");
  }
  std::vector<node_id_t> in_nodes_id;
  // Use the pre-opened rd_in_cursor (IN_EDGES if directed, OUT_EDGES if
  // undirected) instead of opening a new cursor per call.  reset() at the end
  // releases the page pin without the overhead of close+reopen from cache.
  WT_CURSOR *in_cur = rd_in_cursor;
  int search_exact;
  CommonUtil::ekey_set_node_key(in_cur, node_id);
  in_cur->search_near(in_cur, &search_exact);
  if (search_exact <= 0)
  {
    in_cur->next(in_cur);  // position to the first in-edge of the node
  }

  do
  {
    node_id_t src_id, dst_id;
    CommonUtil::ekey_get_key(in_cur, &dst_id, &src_id);

    if (dst_id == node_id)
    {
      in_nodes_id.push_back(src_id);
    }
    else
    {
      break;
    }
  } while (in_cur->next(in_cur) == 0);

  in_cur->reset(in_cur);
  return in_nodes_id;
}

/**
 * @brief This function accepts a node_id and two integers, in_change and
 * out_change and updates the in and out degree of the node in the in_Edge and
 * out_Edge tables. This function must be called within a running transaction,
 * and does not commit the transaction.
 *
 * OPTIMIZATION: Uses cached degree_cursor instead of creating a new cursor
 * each time. This significantly reduces cursor creation overhead.
 *
 * @param node_id the node to be updated in both in and out edge tables
 * @param in_change the +/- value of the change in the node's in-degree
 * @param out_change the +/- value of the change in the node's out-degree
 * @return 1 on error, 0 on success
 */
int SplitEdgeKey::update_node_degree(node_id_t node_id,
                                     int in_change,
                                     int out_change)
{
  degree_t in, out;
  int ret = 0;

  // Use cached cursor instead of creating new one each time
  if (degree_cursor == nullptr)
  {
    degree_cursor = get_new_out_cursor();
  }
  degree_cursor->reset(degree_cursor);

  CommonUtil::ekey_set_node_key(degree_cursor, node_id);
  if (!(ret = degree_cursor->search(degree_cursor)))
  {
    ekey_get_node_value(degree_cursor, &in, &out);
    ekey_set_node_value(degree_cursor, in + in_change, out + out_change);
    ret = degree_cursor->update(degree_cursor);
  }
  else
  {
    LOG_MSG("Failed to update the node degree: Node {} does not exist",
            node_id,
            wiredtiger_strerror(ret));
  }
  // Don't close - cursor is cached for reuse
  return error_check_insert_txn(ret, false);
}

OutCursor *SplitEdgeKey::get_outnbd_iter()
{
  OutCursor *toReturn = new SplitEKeyOutCursor(get_new_out_cursor(), session);
  return toReturn;
}
InCursor *SplitEdgeKey::get_innbd_iter()
{
  InCursor *toReturn = new SplitEkeyInCursor(
      get_new_in_cursor(), session, opts.is_directed, opts.read_optimize);

  return toReturn;
}
NodeCursor *SplitEdgeKey::get_node_iter()
{
  NodeCursor *toReturn = new SplitEKeyNodeCursor(get_new_out_cursor(), session);
  return toReturn;
}
EdgeCursor *SplitEdgeKey::get_edge_iter()
{
  EdgeCursor *toReturn = new SplitEKeyEdgeCursor(get_new_out_cursor(), session);
  return toReturn;
}
void SplitEdgeKey::get_random_node_ids(vector<node_id_t> &randoms,
                                       int num_nodes)
{
  int count = 0, waste = 0;
  int ret = 0;
  if (random_node_cursor == nullptr)
  {
    ret = _get_table_cursor(EDGE_TABLE,
                            &random_node_cursor,
                            session,
                            true,
                            true,
                            opts.checkpoint_name);
  }
  if (ret != 0)
  {
    throw GraphException("Could not get a random cursor to the node table");
  }

  random_node_cursor->next(random_node_cursor);
  do
  {
    node_id_t src, dst;
    uint32_t in_deg, out_deg;
    ret = CommonUtil::ekey_get_key(random_node_cursor, &src, &dst);
    if (ret != 0)
    {
      break;
    }
    if (dst == OutOfBand_ID_MIN)
    {
      #ifdef DEBUG
      std::cout << "Random starting vertex: " << src << std::endl;
      #endif
      // random_node_cursor->get_value(random_node_cursor, &in_deg, &out_deg);
      ekey_get_node_value(random_node_cursor, &in_deg, &out_deg);
      if (out_deg > 0)
      {
        randoms.emplace_back(src);
        count++;
      }

      if (count == num_nodes) break;
    }
    else
    {
      waste++;
    }
    ret = random_node_cursor->next(random_node_cursor);
    if (ret != 0)
    {
      random_node_cursor->reset(random_node_cursor);
      random_node_cursor->next(random_node_cursor);
    }
  } while (count < num_nodes);
  if (ret != 0)
  {
    throw GraphException("Issue with random cursor iteration:" +
                         string(wiredtiger_strerror(ret)));
  }
  
  #ifdef DEBUG
  std::cout << "wasted iterations: " << waste << std::endl;
  std::cout << "ids size: " << randoms.size() << std::endl;
  #endif
}
int SplitEdgeKey::error_check_insert_txn(int return_val,
                                         bool ignore_duplicate_key)
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
        LOG_MSG("Rolling back; duplicate key found");
        session->rollback_transaction(session, nullptr);
      }
      return WT_DUPLICATE_KEY;
    default:
      LOG_MSG("Rolling back in insert_txn: ", wiredtiger_strerror(return_val));
      session->rollback_transaction(session, nullptr);
      return return_val;
  }  //  switch (return_val)
  //  {
  //    case 0:
  //      return 0;
  //    case WT_ROLLBACK:
  //      session->rollback_transaction(session, nullptr);
  //      return WT_ROLLBACK;
  //    case WT_DUPLICATE_KEY:
  //      session->rollback_transaction(session, nullptr);
  //      return WT_DUPLICATE_KEY;
  //    default:
  //      session->rollback_transaction(session, nullptr);
  //      return GRAPH_API_PANIC;
  //  }
}

int SplitEdgeKey::error_check_read_txn(int return_val)
{
  switch (return_val)
  {
    case 0:
      return 0;
    case WT_ROLLBACK:
      session->rollback_transaction(session, nullptr);
      return WT_ROLLBACK;
    case WT_NOTFOUND:
      LOG_MSG("The key was not found", wiredtiger_strerror(return_val));
      session->rollback_transaction(session, nullptr);
      return WT_NOTFOUND;
    default:
      session->rollback_transaction(session, nullptr);
      LOG_MSG("Failed to complete read action : ",
              wiredtiger_strerror(return_val));
      return return_val;
  }
}

node_id_t SplitEdgeKey::get_min_node_id()
{
  WT_CURSOR *cursor = get_new_out_cursor();
  node_id_t src, dst;
  int ret = cursor->next(cursor);
  if (ret == 0)
  {
    CommonUtil::ekey_get_key(cursor, &src, &dst);
    assert(dst == OutOfBand_ID_MIN);
  }
  else
  {
    throw GraphException("Could not get the minimum node ID");
  }
  cursor->close(cursor);
  return src;
}

node_id_t SplitEdgeKey::get_max_node_id()
{
  WT_CURSOR *cursor = get_new_out_cursor();
  node_id_t src, dst;
  int ret = cursor->prev(cursor);
  if (ret == 0)
  {
    CommonUtil::ekey_get_key(cursor, &src, &dst);
  }
  else
  {
    throw GraphException("Could not get the maximum node ID");
  }
  cursor->close(cursor);
  return src;
}

int SplitEdgeKey::delete_node(node_id_t node_id)
{
  int num_edges_to_add = 0;
  session->begin_transaction(session, "isolation=snapshot");

  int ret = delete_node_and_related_edges(node_id, &num_edges_to_add);
  if (ret != 0)
    return ret;
  else
  {
    session->commit_transaction(session, nullptr);
    GraphBase::increment_edges(num_edges_to_add);
    GraphBase::increment_nodes(-1);
  }
  return 0;
}
WT_CURSOR *SplitEdgeKey::get_metadata_cursor()
{
  if (metadata_cursor == nullptr)
  {
    int ret = _get_table_cursor(
        METADATA, &metadata_cursor, session, false, true, opts.checkpoint_name);
    if (ret != 0)
    {
      throw GraphException("Could not get a metadata cursor");
    }
  }
  return metadata_cursor;
}
int SplitEdgeKey::delete_node_and_related_edges(node_id_t node_id,
                                                int *num_edges_to_add)
{
  int ret;
  node_id_t src, dst;
  WT_CURSOR *incursor = get_new_in_cursor();
  CommonUtil::ekey_set_node_key(out_edge_cursor, node_id);
  if (out_edge_cursor->search(out_edge_cursor) != 0)
  {
    session->rollback_transaction(session, nullptr);
    return 1;
  }

  ret = out_edge_cursor->remove(out_edge_cursor);
  if (ret != 0)
  {
    session->rollback_transaction(session, nullptr);
    return ret;
  }

  while (out_edge_cursor->next(out_edge_cursor) == 0)
  {
    CommonUtil::ekey_get_key(out_edge_cursor, &src, &dst);
    if (src != node_id)
    {
      break;
    }
    // Delete the edge OUTGOING FROM the deleted node
    ret = out_edge_cursor->remove(out_edge_cursor);
    if (ret != 0)
    {
      session->rollback_transaction(session, nullptr);
      return ret;  // panic
    }
    // delete the reverse edge and adjust dst's in-degree
    CommonUtil::ekey_set_key(incursor, dst, src);
    ret = incursor->remove(incursor);
    if (ret != 0)
    {
      session->rollback_transaction(session, nullptr);
      return ret;  // panic
    }
    // We have successfully deleted the edge
    *num_edges_to_add -= 1;  // we have effectively only removed one edge
    (opts.is_directed) ? (ret = update_node_degree(dst, -1, 0))
                       : (ret = update_node_degree(dst, -1, -1));
    if (ret != 0)
    {
      session->rollback_transaction(session, nullptr);
      return ret;
    }
  }
  out_edge_cursor->reset(out_edge_cursor);
  incursor->reset(incursor);

  /**
   * Now we need to delete edges that are incoming to the node being deleted. We
   * need to explicitly handle this case for directed graphs, because sweeping
   * the out_table liek we have done so far will not give us an edge incoming to
   * the node being deleted.
   * SO, we scan the in-table with (node_id, OutOfBand_ID_MIN) as the key, and
   * find the edges incoming to the node. Then we delete those edges from the
   * in-table and the corresponding edges from the out-table.
   */
  if (opts.is_directed)
  {
    // Now we need to remove the edges incoming to the node
    CommonUtil::ekey_set_node_key(incursor, node_id);
    int search_dir = 0;
    incursor->search_near(incursor, &search_dir);
    if (search_dir < 0)
    {
      incursor->next(incursor);
    }
    do
    {
      CommonUtil::ekey_get_key(incursor, &dst, &src);
      if (dst != node_id)
      {
        break;
      }
      // Delete the edge incoming edge to the deleted node (in-edge table)
      ret = incursor->remove(incursor);
      if (ret != 0)
      {
        session->rollback_transaction(session, nullptr);
        return ret;  // panic
      }
      // Delete the corresponding edge from the out-edge table
      CommonUtil::ekey_set_key(out_edge_cursor, src, node_id);
      ret = out_edge_cursor->remove(out_edge_cursor);
      if (ret != 0)
      {
        session->rollback_transaction(session, nullptr);
        return ret;  // panic
      }
      // decrement the node_degree of the src node
      ret = update_node_degree(
          src, 0, -1);  // src node's outdegree in in_edge table
      if (ret != 0)
      {
        session->rollback_transaction(session, nullptr);
        return ret;
      }
      num_edges_to_add -= 1;
    } while (incursor->next(incursor) == 0);
    in_edge_cursor->reset(in_edge_cursor);
    incursor->close(incursor);
  }

  return 0;
}

int SplitEdgeKey::delete_edge(node_id_t src_id, node_id_t dst_id)
{
  int num_edges_to_add = 0;
  int ret;
  session->begin_transaction(session, "isolation=snapshot");

  CommonUtil::ekey_set_key(out_edge_cursor, src_id, dst_id);
  if ((ret = error_check_insert_txn(out_edge_cursor->remove(out_edge_cursor),
                                    false)))
  {
    LOG_MSG("Failed to delete the edge between {} and {}", src_id, dst_id);
    return ret;
  }
  out_edge_cursor->reset(out_edge_cursor);
  // delete the reverse edge.
  CommonUtil::ekey_set_key(in_edge_cursor, dst_id, src_id);
  if ((ret = error_check_insert_txn(in_edge_cursor->remove(in_edge_cursor),
                                    false)))
  {
    LOG_MSG(
        "Failed to delete the reverse edge between {} and {}", dst_id, src_id);
    return ret;
  }
  in_edge_cursor->reset(in_edge_cursor);
  num_edges_to_add -= 1;  // we have effectively only removed one edge

  if (opts.read_optimize)
  {
    opts.is_directed ? (ret = update_node_degree(dst_id, -1, -0))
                     : (ret = update_node_degree(dst_id, -1, -1));
    if (ret)
    {
      LOG_MSG("failed to update node degree");
      return ret;
    }
    opts.is_directed ? (ret = update_node_degree(src_id, 0, -1))
                     : (ret = update_node_degree(src_id, -1, -1));
    if (ret)
    {
      LOG_MSG("failed to update node degree");
      return ret;
    }
  }
  session->commit_transaction(session, nullptr);
  GraphBase::increment_edges(num_edges_to_add);
  return 0;
}

WT_CURSOR *SplitEdgeKey::get_new_out_cursor()
{
  WT_CURSOR *new_out_cursor = nullptr;
  if (_get_table_cursor(OUT_EDGES,
                        &new_out_cursor,
                        session,
                        false,
                        true,
                        opts.checkpoint_name) != 0)
  {
    throw GraphException("Could not get a cursor to the OutEdge table");
  }
  new_out_cursor->reset(new_out_cursor);
  return new_out_cursor;
}

WT_CURSOR *SplitEdgeKey::get_new_in_cursor()
{
  WT_CURSOR *new_in_cursor = nullptr;
  int ret;
  opts.is_directed ? (ret = _get_table_cursor(IN_EDGES,
                                              &new_in_cursor,
                                              session,
                                              false,
                                              true,
                                              opts.checkpoint_name))
                   : (ret = _get_table_cursor(OUT_EDGES,
                                              &new_in_cursor,
                                              session,
                                              false,
                                              true,
                                              opts.checkpoint_name));
  if (ret)
  {
    throw GraphException("Could not get an in edge cursor: " +
                         string(wiredtiger_strerror(ret)));
  }

  return new_in_cursor;
}

void SplitEdgeKey::dump_table(const string &table_name, int num_records)
{
  // ofstream out = ofstream("S_EKey_dump_" + table_name + ".txt");
  in_edge_cursor->reset(in_edge_cursor);
  if (table_name == IN_EDGES)
  {
    std::cout << "Dumping IN_EDGES table:" << std::endl;
    while (in_edge_cursor->next(in_edge_cursor) == 0 && num_records > 0)
    {
      node_id_t src, dst;
      edgeweight_t edge_weight=0.0;
      CommonUtil::ekey_get_key(in_edge_cursor, &dst, &src);
      if(opts.is_weighted)
      {
        
        ekey_get_edge_value(in_edge_cursor, &edge_weight);
      }
      if(src!=0) // skip node entries in this dump.
      {
        std::cout << "(" << dst << ", " << src ;
        if (opts.is_weighted) std::cout << ", " << edge_weight;
        std::cout << ")" << std::endl;
        std::cout << "---------------------------\n";
      }
      num_records--;
    }
  }
  else
  {
    out_edge_cursor->reset(out_edge_cursor);
    std::cout << "Dumping OUT_EDGES table:" << std::endl;
    while (out_edge_cursor->next(out_edge_cursor) == 0 && num_records > 0)
    {
      node_id_t src, dst;
      CommonUtil::ekey_get_key(out_edge_cursor, &src, &dst);
      if (dst == OutOfBand_ID_MIN)
      {
        std::cout << "NODE ID\t" << src << std::endl;
        degree_t d_in, d_out;
        // out_edge_cursor->get_value(out_edge_cursor, &in, &out);
        ekey_get_node_value(out_edge_cursor, &d_in, &d_out);
        std::cout << "In-degree:\t" << d_in << "\nOut-degree:\t" << d_out
                  << std::endl;
      }
      else
      {
        std::cout << "EDGE\t(" << src << ", " << dst;
        if (opts.is_weighted)
        {
          edgeweight_t edge_weight=0.0;
          // out_edge_cursor->get_value(out_edge_cursor, &edge_weight);
          ekey_get_edge_value(out_edge_cursor, &edge_weight);
          std::cout << ", " << edge_weight;
        }
        std::cout << ")" << std::endl;
      }
      std::cout << "---------------------------\n";
      num_records--;
    }
  }
}

