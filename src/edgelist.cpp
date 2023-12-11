#include "edgelist.h"

#include <wiredtiger.h>

#include <string>

#include "common_util.h"

using namespace std;

[[maybe_unused]] const std::string GRAPH_PREFIX = "uo_elist";

UnOrderedEdgeList::UnOrderedEdgeList(graph_opts &opt_params,
                                     WT_CONNECTION *conn)
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

bool UnOrderedEdgeList::has_node(node_id_t node_id)
{
    CommonUtil::set_key(node_cursor, node_id);
    int ret = node_cursor->search(node_cursor);
    node_cursor->reset(node_cursor);
    return (ret == 0);
}
void UnOrderedEdgeList::create_wt_tables(graph_opts &opts, WT_CONNECTION *conn)
{
    // ******** Now set up the Node Table     **************
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
        node_columns.push_back(IN_DEGREE);
        node_columns.push_back(OUT_DEGREE);
        node_value_format = "II";
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
    // Now prepare the edge value format. starts with II for src,dst. Add
    // another I if weighted
    vector<string> edge_columns = {ID, SRC, DST};
    string edge_key_format =
        "u";  // id : uint32_t, big_endian using __builtin_bswap32

    string edge_value_format = "II";  // source, dst are uint32_t
    if (opts.is_weighted)
    {
        edge_columns.push_back(WEIGHT);
        edge_value_format += "i";  // int32_t; signed.
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

    if (!opts.optimize_create)
    {
        create_indices(sess);
    }

    sess->close(sess, nullptr);
}

void UnOrderedEdgeList::init_cursors()
{
    // metadata_cursor initialization
    int ret =
        _get_table_cursor(METADATA, &metadata_cursor, session, false, false);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the metadata table:" +
                             string(wiredtiger_strerror(ret)));
    }
    // node_cursor initialization
    ret = _get_table_cursor(NODE_TABLE, &node_cursor, session, false, true);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the node table:" +
                             string(wiredtiger_strerror(ret)));
    }
    // edge_cursor initialization
    ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, session, false, false);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the edge table:" +
                             string(wiredtiger_strerror(ret)));
    }
    // The edge table needs the same cursors as in std_graph.
    string projection;
    opts.is_weighted
        ? projection = "(" + ID + "," + SRC + "," + DST + "," + WEIGHT + ")"
        : projection = "(" + ID + "," + SRC + "," + DST + "," + "na" + ")";

    ret =
        _get_index_cursor(EDGE_TABLE,
                          SRC_INDEX,
                          projection,
                          &(src_index_cursor));  // Does not throw error in case
                                                 // indices are not created;
                                                 // will print to stderror
    ret = _get_index_cursor(
        EDGE_TABLE, DST_INDEX, projection, &(dst_index_cursor));
    ret = _get_index_cursor(
        EDGE_TABLE, SRC_DST_INDEX, projection, &(src_dst_index_cursor));
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
int UnOrderedEdgeList::add_node(node to_insert)
{
    session->begin_transaction(session, "isolation=snapshot");
    int ret;

    CommonUtil::set_key(node_cursor, to_insert.id);

    if (opts.read_optimize)
    {
        node_cursor->set_value(
            node_cursor, to_insert.in_degree, to_insert.out_degree);
    }
    else
    {
        node_cursor->set_value(node_cursor, "");
    }

    if ((ret = error_check_insert_txn(node_cursor->insert(node_cursor), false)))
    {
        return ret;
    }

    session->commit_transaction(session, nullptr);
    add_to_nnodes(1);
    return ret;
}

int UnOrderedEdgeList::add_node_txn(node to_insert)
{
    CommonUtil::set_key(node_cursor, to_insert.id);
    if (opts.read_optimize)
    {
        node_cursor->set_value(
            node_cursor, to_insert.in_degree, to_insert.out_degree);
    }
    else
    {
        node_cursor->set_value(node_cursor, "");
    }
    return node_cursor->insert(node_cursor);
}

/**
 * @brief Adds an edge to the graph; will attempt to add nodes associated with
 * the edge
 *
 * @param to_insert the edge object to be inserted
 * @param is_bulk_insert indicates whether system is in bulk_insert mode
 * @returns 0 if operation is successful
 * @returns WT_ROLLBACK if there is a concurrent read/write conflict
 * @returns WT_DUPLICATE_KEY if there is a key conflict within the database
 * @returns WT_NOT_FOUND if a component associated with the edge is
 * missing TODO: findout if this is neccessary
 * @throws GraphException for other databse errors
 */

int UnOrderedEdgeList::add_edge(edge to_insert, bool is_bulk_insert)
{
    int num_nodes_to_add = 0;
    int num_edges_to_add = 0;
    int ret = 0;

    session->begin_transaction(session, "isolation=snapshot");
    bool src_exists = has_node(to_insert.src_id);
    bool dst_exists = has_node(to_insert.dst_id);
    // Ensure src and dst nodes exist
    if (!is_bulk_insert)
    {
        if (!src_exists)
        {
            node src = {0};
            if (opts.is_directed)
            {
                src = {.id = to_insert.src_id, .in_degree = 0, .out_degree = 1};
            }
            else
            {
                src = {.id = to_insert.src_id, .in_degree = 1, .out_degree = 1};
            }
            ret = error_check(add_node_txn(src), __FILE__, __LINE__);
            if (ret != 0)
            {
                return ret;  // ret == 1 means rollback, ret == -1 means
                             // duplicate key
            }
            num_nodes_to_add += 1;
        }
        node_cursor->reset(node_cursor);
        if (!dst_exists)
        {
            node dst = {0};
            if (opts.is_directed)
            {
                dst = {.id = to_insert.dst_id, .in_degree = 1, .out_degree = 0};
            }
            else
            {
                dst = {.id = to_insert.dst_id, .in_degree = 1, .out_degree = 1};
            }
            ret = error_check(add_node_txn(dst), __FILE__, __LINE__);
            if (ret != 0)
            {
                return ret;
            }
            num_nodes_to_add += 1;
        }
        /*At this point the src and dst nodes exists in the context of the
        transaction, but have not yet been .committed to the database. If the
        transaction is rolled back, the nodes will be removed. In the following
        steps, if we try to edit the node (to increment the degree), we will get
        a WT_ROLLBACK error. We need to handle this case by rolling back the
        transaction and starting over.
        */

        CommonUtil::set_key(edge_cursor, to_insert.id);

        if (opts.is_weighted)
        {
            edge_cursor->set_value(edge_cursor,
                                   to_insert.src_id,
                                   to_insert.dst_id,
                                   to_insert.edge_weight);
        }
        else
        {
            edge_cursor->set_value(
                edge_cursor, to_insert.src_id, to_insert.dst_id, 0);
        }

        ret = error_check(edge_cursor->insert(edge_cursor), __FILE__, __LINE__);
        if (!ret)
        {
            num_edges_to_add += 1;
        }
        else
        {
            CommonUtil::log_msg("Failed to insert edge between " +
                                std::to_string(to_insert.src_id) + " and " +
                                std::to_string(to_insert.dst_id) +
                                wiredtiger_strerror(ret), __FILE__, __LINE__);
            return ret;
        }

        if (!opts.is_directed)
        {
            CommonUtil::set_key(edge_cursor, to_insert.id);

            if (opts.is_weighted)
            {
                edge_cursor->set_value(edge_cursor,
                                       to_insert.dst_id,
                                       to_insert.src_id,
                                       to_insert.edge_weight);
            }
            else
            {
                edge_cursor->set_value(
                    edge_cursor, to_insert.dst_id, to_insert.src_id, 0);
            }

            ret = error_check(edge_cursor->insert(edge_cursor), __FILE__, __LINE__);
            if (!ret)
            {
                num_edges_to_add += 1;
            }
            else
            {
                CommonUtil::log_msg("Failed to insert (reverse) edge between " +
                                    std::to_string(to_insert.dst_id) + " and " +
                                    std::to_string(to_insert.src_id) +
                                    wiredtiger_strerror(ret), __FILE__, __LINE__);
                return ret;
            }
        }

        if (!is_bulk_insert)
        {
            // If opts.read_optimized is true, we update in/out degreees in the
            // node table.
            if (this->opts.read_optimize)
            {
                // If the node exists, search and update. Otherwise, insert.
                if (src_exists)
                {
                    CommonUtil::set_key(node_cursor, to_insert.src_id);
                    node_cursor->search(node_cursor);
                    // update in/out degrees for src node in NODE_TABLE
                    node found = {0};
                    CommonUtil::record_to_node(
                        node_cursor, &found, opts.read_optimize);
                    found.id = to_insert.src_id;
                    found.out_degree++;
                    ret = error_check(update_node_degree(node_cursor,
                                                         found.id,
                                                         found.in_degree,
                                                         found.out_degree),
                                      __FILE__, __LINE__);  //! pass the cursor
                                                 //! to this function

                    if (ret != 0)
                    {
                        CommonUtil::log_msg(
                            "Failed to update node degree for node " +
                            std::to_string(to_insert.src_id) +
                            wiredtiger_strerror(ret), __FILE__, __LINE__);
                        return ret;
                    }
                }

                if (dst_exists)
                {
                    // update in/out degrees for the dst node in the NODE_TABLE
                    CommonUtil::set_key(node_cursor, to_insert.dst_id);
                    node_cursor->search(node_cursor);
                    node found = {0};
                    CommonUtil::record_to_node(
                        node_cursor, &found, opts.read_optimize);
                    found.id = to_insert.dst_id;
                    found.in_degree++;
                    ret = error_check(update_node_degree(node_cursor,
                                                         found.id,
                                                         found.in_degree,
                                                         found.out_degree),
                                      __FILE__, __LINE__);
                    if (ret != 0)
                    {
                        CommonUtil::log_msg(
                            "Failed to update node degree for node " +
                            std::to_string(to_insert.dst_id) +
                            wiredtiger_strerror(ret), __FILE__, __LINE__);
                        return ret;
                    }
                }
            }
        }
    }

    session->commit_transaction(session, nullptr);
    add_to_nedges(num_edges_to_add);
    add_to_nnodes(num_nodes_to_add);
    return 0;
}

int UnOrderedEdgeList::update_node_degree(WT_CURSOR *cursor,
                                          node_id_t node_id,
                                          degree_t in_degree,
                                          degree_t out_degree)
{
    CommonUtil::set_key(cursor, node_id);
    int ret = cursor->search(cursor);
    if (ret != 0)
    {
        CommonUtil::check_return(
            ret, "Could not find node with id = " + to_string(node_id));
    }

    node found{.id = node_id, .in_degree = in_degree, .out_degree = out_degree};
    ret = CommonUtil::node_to_record(cursor, found, opts.read_optimize);
    cursor->reset(cursor);
    return ret;
}

node UnOrderedEdgeList::get_random_node()
{
    node found = {0};
    WT_CURSOR *cursor;
    int ret = _get_table_cursor(NODE_TABLE, &cursor, session, true, false);
    if (ret != 0)
    {
        throw GraphException("could not get a random cursor to the node table");
    }
    ret = cursor->next(cursor);
    if (ret != 0)
    {
        throw GraphException("Could not seek a random node in the table");
    }

    CommonUtil::record_to_node(cursor, &found, opts.read_optimize);
    CommonUtil::get_key(cursor, &found.id);
    return found;
}

degree_t UnOrderedEdgeList::get_in_degree(node_id_t node_id)
{
    degree_t count = 0;
    if (this->opts.read_optimize)
    {
        node found = get_node(node_id);
        return found.in_degree;
    }
    else
    {
        if (!has_node(node_id))
        {
            throw GraphException("Node " + std::to_string(node_id) +
                                 "does not exist");
        }
        // We can use raw cursor ops here because the cursor key (dst) is int
        // (not 'u')
        dst_index_cursor->set_key(dst_index_cursor, node_id);
        if (dst_index_cursor->search(dst_index_cursor) == 0)
        {
            count++;
            while (dst_index_cursor->next(dst_index_cursor) == 0)
            {
                edge found;
                WT_ITEM key;
                dst_index_cursor->get_value(dst_index_cursor,
                                            &key,
                                            &found.src_id,
                                            &found.dst_id,
                                            &found.edge_weight);
                if (found.dst_id != node_id)
                {
                    break;
                }
                else
                {
                    count++;
                }
            }
        }

        dst_index_cursor->reset(dst_index_cursor);
        return count;
    }
}

degree_t UnOrderedEdgeList::get_out_degree(node_id_t node_id)
{
    {
        degree_t count = 0;
        if (this->opts.read_optimize)
        {
            node found = get_node(node_id);
            return found.out_degree;
        }
        else
        {
            if (!has_node(node_id))
            {
                throw GraphException("Node " + std::to_string(node_id) +
                                     "does not exist");
            }
            // We can use raw cursor ops here because the cursor key (dst) is
            // int (not 'u')
            src_index_cursor->set_key(src_index_cursor, node_id);
            if (src_index_cursor->search(src_index_cursor) == 0)
            {
                count++;
                while (src_index_cursor->next(src_index_cursor) == 0)
                {
                    edge found;
                    WT_ITEM key;
                    src_index_cursor->get_value(src_index_cursor,
                                                &key,
                                                &found.src_id,
                                                &found.dst_id,
                                                &found.edge_weight);
                    if (found.src_id != node_id)
                    {
                        break;
                    }
                    else
                    {
                        count++;
                    }
                }
            }

            src_index_cursor->reset(src_index_cursor);
            return count;
        }
    }
}

/**
 * @brief Get all nodes in the graph
 *
 * @return std::vector<node> vector of all nodes
 */
std::vector<node> UnOrderedEdgeList::get_nodes()
{
    std::vector<node> nodelist;

    while ((node_cursor->next(node_cursor) == 0))
    {
        node found;
        CommonUtil::record_to_node(node_cursor, &found, opts.read_optimize);
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
node UnOrderedEdgeList::get_node(node_id_t node_id)
{
    node found = {-1};
    CommonUtil::set_key(node_cursor, node_id);
    int ret = node_cursor->search(node_cursor);
    if (ret == 0)
    {
        CommonUtil::record_to_node(node_cursor, &found, opts.read_optimize);
        found.id = node_id;
    }
    node_cursor->reset(node_cursor);
    return found;
}

/**
 * @brief Get a list of all the edges in the graph
 *
 * @return std::vector<edge> Vector containing all the edges in the graph
 */
std::vector<edge> UnOrderedEdgeList::get_edges()
{
    std::vector<edge> edgelist;
    while (edge_cursor->next(edge_cursor) == 0)
    {
        edge found = {-1, -1, -1, -1};
        WT_ITEM id;
        edge_cursor->get_key(edge_cursor, &id);
        edge_cursor->get_value(
            edge_cursor, &found.src_id, &found.dst_id, &found.edge_weight);
        found.id = CommonUtil::extract_flip_endian(id);
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
edge UnOrderedEdgeList::get_edge(node_id_t src_id, node_id_t dst_id)
{
    edge found = {-1, -1, -1, -1};
    src_dst_index_cursor->set_key(src_dst_index_cursor, src_id, dst_id);
    int ret = src_dst_index_cursor->search(src_dst_index_cursor);
    if (ret == 0)
    {
        WT_ITEM id;
        src_dst_index_cursor->get_value(src_dst_index_cursor,
                                        &id,
                                        &found.src_id,
                                        &found.dst_id,
                                        &found.edge_weight);
        found.id = CommonUtil::extract_flip_endian(id);
    }
    src_dst_index_cursor->reset(src_dst_index_cursor);
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
bool UnOrderedEdgeList::has_edge(node_id_t src_id, node_id_t dst_id)
{
    src_dst_index_cursor->reset(src_dst_index_cursor);
    src_dst_index_cursor->set_key(src_dst_index_cursor, src_id, dst_id);
    int ret = src_dst_index_cursor->search(src_dst_index_cursor);
    return ret == 0;
}

int UnOrderedEdgeList::error_check_insert_txn(int return_val,
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
                // session->rollback_transaction(session, nullptr);
                /*Rolling back the transaction here is not necessary if the
                 * cursor has been opened with overwrite=false. A plain
                 * warning is enough. */
                CommonUtil::log_msg("WT_DUPLICATE_KEY error in insert_txn",
                                    __FILE__, __LINE__);
            }
            return WT_DUPLICATE_KEY;
        case WT_NOTFOUND:
            //! Should we roll back the transaction here? This should not
            //! happen.
            CommonUtil::log_msg("WT_NOTFOUND error in insert_txn", __FILE__, __LINE__);
            return WT_NOTFOUND;
        default:
            session->rollback_transaction(session, nullptr);
            throw GraphException("Failed to complete insert action" +
                                 std::string(wiredtiger_strerror(return_val)));
    }
}

[[maybe_unused]] int UnOrderedEdgeList::error_check_update_txn(int return_val)
{
    switch (return_val)
    {
        case 0:
            return 0;
        case WT_ROLLBACK:
            session->rollback_transaction(session, nullptr);
            return WT_ROLLBACK;
        default:
            session->rollback_transaction(session, nullptr);
            throw GraphException("Failed to complete update action" +
                                 std::string(wiredtiger_strerror(return_val)));
    }
}

[[maybe_unused]] int UnOrderedEdgeList::error_check_read_txn(int return_val)
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
            throw GraphException("Failed to complete read action" +
                                 std::string(wiredtiger_strerror(return_val)));
    }
}

[[maybe_unused]] int UnOrderedEdgeList::error_check_remove_txn(int return_val)
{
    switch (return_val)
    {
        case 0:
            return 0;
        case WT_ROLLBACK:
            session->rollback_transaction(session, nullptr);
            return WT_ROLLBACK;
        case WT_NOTFOUND:
        default:
            session->rollback_transaction(session, nullptr);
    }
    return return_val;
}

void UnOrderedEdgeList::create_indices(WT_SESSION *sess)
{
    // Create table index on (src, dst)
    int ret = 0;
    string edge_table_index_str, edge_table_index_conf_str;
    edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_DST_INDEX;
    edge_table_index_conf_str = "columns=(" + SRC + "," + DST + ")";
    // THere is likely a util fucntion for this
    if ((ret = sess->create(sess,
                            edge_table_index_str.c_str(),
                            edge_table_index_conf_str.c_str())) > 0)
    {
        fprintf(stderr,
                "Failed to create index SRC_DST_INDEX in the edge table");
    }
    // Create index on SRC column
    edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
    edge_table_index_conf_str = "columns=(" + SRC + ")";
    if ((ret = sess->create(sess,
                            edge_table_index_str.c_str(),
                            edge_table_index_conf_str.c_str())) > 0)
    {
        fprintf(stderr, "Failed to create index SRC_INDEX in the edge table");
    }
    // Create index on DST column
    edge_table_index_str = "index:" + EDGE_TABLE + ":" + DST_INDEX;
    edge_table_index_conf_str = "columns=(" + DST + ")";
    if ((ret = sess->create(sess,
                            edge_table_index_str.c_str(),
                            edge_table_index_conf_str.c_str())) > 0)
    {
        fprintf(stderr, "Failed to create index DST_INDEX in the edge table");
    }
}

[[maybe_unused]] void UnOrderedEdgeList::drop_indices()
{
    CommonUtil::close_cursor(this->edge_cursor);
    CommonUtil::close_cursor(this->src_dst_index_cursor);
    CommonUtil::close_cursor(this->src_index_cursor);
    CommonUtil::close_cursor(this->dst_index_cursor);

    // Drop (src) index on edge table
    string uri = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
    this->session->drop(this->session, uri.c_str(), nullptr);

    // Drop (dst) index on the edge table
    uri = "index:" + EDGE_TABLE + ":" + DST_INDEX;
    this->session->drop(this->session, uri.c_str(), nullptr);
}

int UnOrderedEdgeList::error_check(int ret, const std::string& line ,int loc)
{
    switch (ret)
    {
        case 0:
            return 0;
        case WT_DUPLICATE_KEY:
        case WT_ROLLBACK:
        default:
            session->rollback_transaction(session, nullptr);
            CommonUtil::log_msg("Error code( " + to_string(ret) +
                                    ") :" + wiredtiger_strerror(ret),
                                line, loc);
    }
    return ret;
}

std::vector<edge> UnOrderedEdgeList::get_out_edges(node_id_t node_id)
{
    vector<edge> edges;
    if (!has_node(node_id))
    {
        throw GraphException("Could not find node with id" +
                             to_string(node_id));
    }

    src_index_cursor->set_key(src_index_cursor, node_id);
    if (src_index_cursor->search(src_index_cursor) == 0)
    {
        edge found = {-1, -1, -1, -1};
        WT_ITEM id;
        src_index_cursor->get_value(src_index_cursor,
                                    &id,
                                    &found.src_id,
                                    &found.dst_id,
                                    &found.edge_weight);
        found.id = CommonUtil::extract_flip_endian(id);
        edges.push_back(found);
        while (src_index_cursor->next(src_index_cursor) == 0)
        {
            src_index_cursor->get_value(src_index_cursor,
                                        &id,
                                        &found.src_id,
                                        &found.dst_id,
                                        &found.edge_weight);
            found.id = CommonUtil::extract_flip_endian(id);
            if (found.src_id == node_id)
            {
                edges.push_back(found);
            }
            else
            {
                break;
            }
        }
    }
    src_index_cursor->reset(src_index_cursor);
    return edges;
}
std::vector<node> UnOrderedEdgeList::get_out_nodes(node_id_t node_id)
{
    vector<edge> out_edges;
    vector<node> nodes;
    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    out_edges = get_out_edges(node_id);
    nodes.reserve(out_edges.size());
    for (edge out_edge : out_edges)
    {
        nodes.push_back(get_node(out_edge.dst_id));
    }
    return nodes;
}
std::vector<node_id_t> UnOrderedEdgeList::get_out_nodes_id(node_id_t node_id)
{
    vector<edge> out_edges;
    vector<node_id_t> node_ids;
    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    out_edges = get_out_edges(node_id);
    node_ids.reserve(out_edges.size());
    for (edge out_edge : out_edges)
    {
        node_ids.push_back(out_edge.dst_id);
    }
    return node_ids;
}
std::vector<edge> UnOrderedEdgeList::get_in_edges(node_id_t node_id)
{
    vector<edge> edges;

    if (!has_node(node_id))
    {
        throw GraphException("Could not find node with id" +
                             to_string(node_id));
    }
    dst_index_cursor->set_key(dst_index_cursor, node_id);
    if (dst_index_cursor->search(dst_index_cursor) == 0)
    {
        edge found = {-1, -1, -1, -1};
        WT_ITEM id;
        dst_index_cursor->get_value(dst_index_cursor,
                                    &id,
                                    &found.src_id,
                                    &found.dst_id,
                                    &found.edge_weight);
        found.id = CommonUtil::extract_flip_endian(id);
        edges.push_back(found);
        while (dst_index_cursor->next(dst_index_cursor) == 0)
        {
            dst_index_cursor->get_value(dst_index_cursor,
                                        &id,
                                        &found.src_id,
                                        &found.dst_id,
                                        &found.edge_weight);
            if (found.dst_id == node_id)
            {
                found.id = CommonUtil::extract_flip_endian(id);
                edges.push_back(found);
            }
            else
            {
                break;
            }
        }
    }
    dst_index_cursor->reset(dst_index_cursor);
    return edges;
}
std::vector<node> UnOrderedEdgeList::get_in_nodes(node_id_t node_id)
{
    vector<node> nodes;
    if (!has_node(node_id))
    {
        throw GraphException("Could not find node with id" +
                             to_string(node_id));
    }

    vector<edge> in_edges = get_in_edges(node_id);
    nodes.reserve(in_edges.size());
    for (edge in_edge : in_edges)
    {
        nodes.push_back(get_node(in_edge.src_id));
    }
    return nodes;
}
std::vector<node_id_t> UnOrderedEdgeList::get_in_nodes_id(node_id_t node_id)
{
    vector<node_id_t> nodes;
    if (!has_node(node_id))
    {
        throw GraphException("Could not find node with id" +
                             to_string(node_id));
    }

    vector<edge> in_edges = get_in_edges(node_id);
    nodes.reserve(in_edges.size());
    for (edge in_edge : in_edges)
    {
        nodes.push_back(in_edge.src_id);
    }
    return nodes;
}

[[maybe_unused]]WT_CURSOR *UnOrderedEdgeList::get_node_cursor()
{
    if (node_cursor == nullptr)
    {
        int ret =
            _get_table_cursor(NODE_TABLE, &node_cursor, session, false, true);
        if (ret != 0)
        {
            throw GraphException("Could not get a node cursor");
        }
    }

    return node_cursor;
}

WT_CURSOR *UnOrderedEdgeList::get_new_node_cursor()
{
    WT_CURSOR *new_node_cursor = nullptr;
    int ret =
        _get_table_cursor(NODE_TABLE, &new_node_cursor, session, false, false);
    if (ret != 0)
    {
        throw GraphException("Could not get a node cursor");
    }

    return new_node_cursor;
}

WT_CURSOR *UnOrderedEdgeList::get_edge_cursor()
{
    if (edge_cursor == nullptr)
    {
        int ret =
            _get_table_cursor(EDGE_TABLE, &edge_cursor, session, false, false);
        if (ret != 0)
        {
            throw GraphException("Could not get an edge cursor");
        }
    }

    return edge_cursor;
}

WT_CURSOR *UnOrderedEdgeList::get_new_edge_cursor()
{
    WT_CURSOR *new_edge_cursor = nullptr;
    int ret =
        _get_table_cursor(EDGE_TABLE, &new_edge_cursor, session, false, false);
    if (ret != 0)
    {
        throw GraphException("Could not get an edge cursor");
    }

    return new_edge_cursor;
}

WT_CURSOR *UnOrderedEdgeList::get_src_idx_cursor()
{
    if (src_index_cursor == nullptr)
    {
        string projection =
            "(" + ID + "," + SRC + "," + DST + "," + WEIGHT + ")";
        if (_get_index_cursor(
                EDGE_TABLE, SRC_INDEX, projection, &(src_index_cursor)) != 0)
        {
            throw GraphException(
                "Could not get a SRC index cursor on the edge table");
        }
    }

    return src_index_cursor;
}

WT_CURSOR *UnOrderedEdgeList::get_new_src_idx_cursor()
{
    WT_CURSOR *new_src_index_cursor = nullptr;
    string projection = "(" + ID + "," + SRC + "," + DST + "," + WEIGHT + ")";
    if (_get_index_cursor(
            EDGE_TABLE, SRC_INDEX, projection, &new_src_index_cursor) != 0)
    {
        throw GraphException(
            "Could not get a SRC index cursor on the edge table");
    }

    return new_src_index_cursor;
}

WT_CURSOR *UnOrderedEdgeList::get_dst_idx_cursor()
{
    if (dst_index_cursor == nullptr)
    {
        string projection =
            "(" + ID + "," + SRC + "," + DST + "," + WEIGHT + ")";
        if (_get_index_cursor(
                EDGE_TABLE, DST_INDEX, projection, &(dst_index_cursor)) != 0)
        {
            throw GraphException(
                "Could not get a DST index cursor on the edge table");
        }
    }

    return dst_index_cursor;
}

WT_CURSOR *UnOrderedEdgeList::get_new_dst_idx_cursor()
{
    WT_CURSOR *new_dst_index_cursor = nullptr;
    string projection = "(" + ID + "," + SRC + "," + DST + "," + WEIGHT + ")";
    if (_get_index_cursor(
            EDGE_TABLE, DST_INDEX, projection, &new_dst_index_cursor) != 0)
    {
        throw GraphException(
            "Could not get a DST index cursor on the edge table");
    }

    return new_dst_index_cursor;
}

[[maybe_unused]]WT_CURSOR *UnOrderedEdgeList::get_src_dst_idx_cursor()
{
    if (src_dst_index_cursor == nullptr)
    {
        string projection =
            "(" + ID + "," + SRC + "," + DST + "," + WEIGHT + ")";
        if (_get_index_cursor(EDGE_TABLE,
                              SRC_DST_INDEX,
                              projection,
                              &(src_dst_index_cursor)) != 0)
        {
            throw GraphException(
                "Could not get a DST index cursor on the edge table");
        }
    }

    return src_dst_index_cursor;
}

[[maybe_unused]]WT_CURSOR *UnOrderedEdgeList::get_new_src_dst_idx_cursor()
{
    WT_CURSOR *new_src_dst_index_cursor = nullptr;
    string projection = "(" + ID + "," + SRC + "," + DST + "," + WEIGHT + ")";
    if (_get_index_cursor(
            EDGE_TABLE, SRC_DST_INDEX, projection, &new_src_dst_index_cursor) !=
        0)
    {
        throw GraphException(
            "Could not get a DST index cursor on the edge table");
    }

    return new_src_dst_index_cursor;
}

OutCursor *UnOrderedEdgeList::get_outnbd_iter()
{
    uint64_t num_nodes = this->get_num_nodes();
    OutCursor *toReturn =
        new UOEdgeListOutCursor(get_new_src_idx_cursor(), session);
    toReturn->set_num_nodes(num_nodes);
    toReturn->set_key_range(
        {-1, static_cast<node_id_t>((int64_t)num_nodes - 1)});
    return toReturn;
}

InCursor *UnOrderedEdgeList::get_innbd_iter()
{
    uint64_t num_nodes = this->get_num_nodes();
    InCursor *toReturn =
        new UOEdgeListInCursor(get_new_dst_idx_cursor(), session);
    toReturn->set_num_nodes(num_nodes);
    toReturn->set_key_range(
        {-1, static_cast<node_id_t>((int64_t)num_nodes - 1)});
    return toReturn;
}

NodeCursor *UnOrderedEdgeList::get_node_iter()
{
    return new UOEdgeListNodeCursor(get_new_node_cursor(), session);
}

EdgeCursor *UnOrderedEdgeList::get_edge_iter()
{
    return new UOEdgeListEdgeCursor(get_new_edge_cursor(), session);
}