#include "standard_graph.h"

#include <wiredtiger.h>

#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

#include "common.h"

using namespace std;
const std::string GRAPH_PREFIX = "std";

StandardGraph::StandardGraph(graph_opts &opt_params, WT_CONNECTION *conn)
    : GraphBase(opt_params, conn)
{
    init_cursors();
}

void StandardGraph::create_wt_tables(graph_opts &opts, WT_CONNECTION *conn)
{
    WT_SESSION *sess;
    if (CommonUtil::open_session(conn, &sess) != 0)
    {
        throw GraphException("Cannot open session");
    }
    // Set up the node table
    // The node entry is of the form: <id>,<in_degree><out_degree>
    // If the graph is opts.read_optimized, add columns and format for in/out
    // degrees
    vector<string> node_columns = {ID};  // Always there :)
    string node_value_format;
    string node_key_format = "'>i'";
    if (opts.read_optimize)
    {
        node_columns.push_back(IN_DEGREE);
        node_columns.push_back(OUT_DEGREE);
        node_value_format = "'>II'";
    }
    else
    {
        node_columns.push_back(
            "na");  // have to do this because the column count must match.
        node_value_format = "s";  // 1 byte fixed length char[] to hold ""
    }
    // Now Create the Node Table
    //! What happens when the table is not read-optimized? I store "" <-ok?

    CommonUtil::set_table(
        sess, NODE_TABLE, node_columns, node_key_format, node_value_format);

    // ******** Now set up the Edge Table     **************
    // Edge Column Format : <src><dst><weight>
    // Now prepare the edge value format. starts with II for src,dst. Add
    // another I if weighted
    vector<string> edge_columns = {SRC, DST};
    string edge_key_format = "ii";
    string edge_value_format = "";  // I if weighted or b if unweighted.

    if (opts.is_weighted)
    {
        edge_columns.push_back(WEIGHT);
        edge_value_format += "i";
    }
    else
    {
        edge_columns.push_back("na");
        edge_value_format += 'b';  // One byte to store ""
    }

    // Create edge table
    CommonUtil::set_table(
        sess, EDGE_TABLE, edge_columns, edge_key_format, edge_value_format);

    if (opts.optimize_create == false)
    {
        /**
         *  Create indices with graph.
         * NOTE: If user opts to optimize graph creation, then they're
         * responsible for calling create_indices() AFTER all the
         * nodes/edges have been added to graph.
         * This is done to improve performance of bulk-loading.
         */
        create_indices(sess);
    }
    sess->close(sess, NULL);
}

void StandardGraph::init_cursors()
{
    int ret =
        _get_table_cursor(METADATA, &metadata_cursor, session, false, false);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the metadata table: " +
                             string(wiredtiger_strerror(ret)));
    }
    ret = _get_table_cursor(NODE_TABLE, &node_cursor, session, false, true);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the node table: " +
                             string(wiredtiger_strerror(ret)));
    }
    ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, session, false, false);
    if (ret != 0)
    {
        throw GraphException("Could not get an edge cursor: " +
                             string(wiredtiger_strerror(ret)));
    }
    string projection;
    opts.is_weighted ? projection = "(" + SRC + "," + DST + "," + WEIGHT + ")"
                     : projection = "(" + SRC + "," + DST + "," + "na" + ")";

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
 * @brief Creates the indices not required for adding nodes/edges.
 * Used for optimized bulk loading: User should call drop_indices() before
 * bulk loading nodes/edges. Once nodes/edges are added, user must
 * call create_indices() for the graph API to work.
 *
 * This method requires exclusive access to the specified data source(s). If
 * any cursors are open with the specified name(s) or a data source is
 * otherwise in use, the call will fail and return EBUSY.
 */
void StandardGraph::create_indices(WT_SESSION *sess)
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

/**
 * @brief The information that gets persisted to WT is of the form:
 * <node_id>,<attr1>..<attrN>,data0,data1,in_degree,out_degree
 * attrs are persisted if has_node_attrs is true
 * data0,data1 are strings. use std::to_string for others.
 * in_degree and out_degree are persisted if opts.read_optimize is true. ints
 *
 *
 * @param to_insert
 */
int StandardGraph::add_node(node to_insert)
{
    int ret = 0;

start_add_node:
    session->begin_transaction(session, "isolation=snapshot");

    node_cursor->set_key(node_cursor, to_insert.id);

    if (opts.read_optimize)
    {
        node_cursor->set_value(
            node_cursor, to_insert.in_degree, to_insert.out_degree);
    }
    else
    {
        node_cursor->set_value(node_cursor, "");
    }

    ret = node_cursor->insert(node_cursor);

    switch (ret)
    {
        case 0:
            break;
        case WT_ROLLBACK:
            session->rollback_transaction(session, NULL);
            return WT_ROLLBACK;
        case WT_DUPLICATE_KEY:
            session->rollback_transaction(session, NULL);
            return WT_DUPLICATE_KEY;
            // return;
        default:
            session->rollback_transaction(session, NULL);
            throw GraphException("Failed to add node_id" +
                                 std::to_string(to_insert.id));
    }

    session->commit_transaction(session, NULL);
    add_to_nnodes(1);
    return 0;
}

int StandardGraph::add_node_txn(node to_insert)
{
    node_cursor->set_key(node_cursor, to_insert.id);
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
 * @brief This function is used to check if a node identified by node_id exists
 * in the node table.
 *
 * @param node_id the node_id to be searched.
 * @return true if the node is found; false otherwise.
 */
bool StandardGraph::has_node(node_id_t node_id)
{
    node_cursor->set_key(node_cursor, node_id);
    int ret = node_cursor->search(node_cursor);
    node_cursor->reset(node_cursor);
    return (ret == 0);
}

/**
 * @brief This function tests if the edge identified by <src_id, dst_id> exists
 *
 * @param src_id source node id
 * @param dst_id destination node_id
 * @return true/false for if the edge exists/does not exist
 */
bool StandardGraph::has_edge(node_id_t src_id, node_id_t dst_id)
{
    src_dst_index_cursor->set_key(src_dst_index_cursor, src_id, dst_id);
    int ret = src_dst_index_cursor->search(src_dst_index_cursor);
    src_dst_index_cursor->reset(src_dst_index_cursor);
    return (ret == 0);
}

edge StandardGraph::get_edge(node_id_t src_id, node_id_t dst_id)
{
    edge found = {-1, -1, -1};
    src_dst_index_cursor->set_key(src_dst_index_cursor, src_id, dst_id);
    int ret = src_dst_index_cursor->search(src_dst_index_cursor);
    if (ret == 0)
    {
        found.src_id = src_id;
        found.dst_id = dst_id;
        if (opts.is_weighted)
        {
            CommonUtil::read_from_edge_idx(src_dst_index_cursor, &found);
        }
    }
    src_dst_index_cursor->reset(src_dst_index_cursor);
    return found;
}

node StandardGraph::get_node(node_id_t node_id)
{
    node found = {-1};
    node_cursor->set_key(node_cursor, node_id);
    int ret = node_cursor->search(node_cursor);
    if (ret == 0)
    {
        CommonUtil::record_to_node(node_cursor, &found, opts.read_optimize);
        found.id = node_id;
    }
    node_cursor->reset(node_cursor);
    return found;
}

node StandardGraph::get_random_node()
{
    node found = {0};
    int ret = 0;

    if (this->random_node_cursor == NULL)
    {
        ret = _get_table_cursor(
            NODE_TABLE, &random_node_cursor, session, true, false);
    }
    ret = this->random_node_cursor->next(random_node_cursor);
    if (ret != 0)
    {
        throw GraphException(wiredtiger_strerror(ret));
    }
    random_node_cursor->get_key(random_node_cursor, &found.id);
    CommonUtil::record_to_node(
        this->random_node_cursor, &found, opts.read_optimize);
    return found;
}

/**
 * @brief This function is used to delete a node and all its edges.
 *
 * @param node_id
 */
//! read again
// TODO(puneet):
int StandardGraph::delete_node(node_id_t node_id)
{
    int num_nodes_to_add = 0;
    int num_edges_to_add = 0;
    int ret = 0;

start_delete_node:
    num_nodes_to_add = 0;
    num_edges_to_add = 0;
    ret = 0;

    session->begin_transaction(session, "isolation=snapshot");

    if (!has_node(node_id))
    {
        session->rollback_transaction(session, NULL);
        return WT_NOTFOUND;
    }

    // Delete incoming and outgoing edges
    vector<edge> incoming_edges = get_in_edges(node_id);
    vector<edge> outgoing_edges = get_out_edges(node_id);

    for (edge e : incoming_edges)
    {
        ret = delete_edge_txn(e.src_id, e.dst_id, &num_edges_to_add);
        switch (ret)
        {
            case 0:
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, NULL);
                return WT_ROLLBACK;
            default:
                session->rollback_transaction(session, NULL);
                throw GraphException("Error msg here");
        }
    }

    for (edge e : outgoing_edges)
    {
        ret = delete_edge_txn(e.src_id, e.dst_id, &num_edges_to_add);
        switch (ret)
        {
            case 0:
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, NULL);
                return WT_ROLLBACK;
            default:
                session->rollback_transaction(session, NULL);
                throw GraphException("Error msg here");
        }
    }

    // Delete the node with the matching node_id
    node_cursor->set_key(node_cursor, node_id);
    ret = node_cursor->remove(node_cursor);
    node_cursor->reset(node_cursor);

    switch (ret)
    {
        case 0:
            num_nodes_to_add -= 1;
            break;
        case WT_ROLLBACK:
            session->rollback_transaction(session, NULL);
            return WT_ROLLBACK;
        default:
            session->rollback_transaction(session, NULL);
            throw GraphException("Could not delete node with ID " +
                                 to_string(node_id));
    }

    session->commit_transaction(session, NULL);
    add_to_nedges(num_edges_to_add);
    add_to_nnodes(num_nodes_to_add);

    return 0;
}

degree_t StandardGraph::get_in_degree(node_id_t node_id)
{
    if (this->opts.read_optimize)
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
        degree_t count = 0;
        dst_index_cursor->set_key(dst_index_cursor, node_id);

        if (dst_index_cursor->search(dst_index_cursor) == 0)
        {
            count++;
            while (dst_index_cursor->next(dst_index_cursor) == 0)
            {
                node_id_t src, dst;
                dst_index_cursor->get_value(dst_index_cursor, &src, &dst);
                if (dst == node_id)
                {
                    count++;
                }
                else
                {
                    break;
                }
            }
        }
        dst_index_cursor->reset(dst_index_cursor);
        return count;
    }
}

degree_t StandardGraph::get_out_degree(node_id_t node_id)
{
    if (opts.read_optimize)
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
        degree_t count = 0;

        src_index_cursor->set_key(src_index_cursor, node_id);
        if (src_index_cursor->search(src_index_cursor) == 0)
        {
            count++;
            while (src_index_cursor->next(src_index_cursor) == 0)
            {
                node_id_t src, dst;
                src_index_cursor->get_value(src_index_cursor, &src, &dst);
                if (src == node_id)
                {
                    count++;
                }
                else
                {
                    break;
                }
            }
        }
        src_index_cursor->reset(src_index_cursor);
        return count;
    }
}

/**
 * @brief get all nodes in the nodes table
 *
 * @return std::vector<node> the vector of all nodes in the node table
 */
std::vector<node> StandardGraph::get_nodes()
{
    vector<node> nodes;
    int ret;
    while ((ret = node_cursor->next(node_cursor) == 0))
    {
        node item{0};
        CommonUtil::record_to_node(node_cursor, &item, opts.read_optimize);
        node_cursor->get_key(node_cursor, &item.id);
        nodes.push_back(item);
    }
    node_cursor->reset(node_cursor);
    return nodes;
}

/**
 * @brief get all nodes in the nodes table
 */
void StandardGraph::get_nodes(vector<node> &nodes)
{
    int ret;
    while ((ret = node_cursor->next(node_cursor) == 0))
    {
        node item{0};
        CommonUtil::record_to_node(node_cursor, &item, opts.read_optimize);
        node_cursor->get_key(node_cursor, &item.id);
        nodes.push_back(item);
    }
    node_cursor->reset(node_cursor);
}

int StandardGraph::error_check_add_edge(int ret)
{
    switch (ret)
    {
        case 0:
            return 0;
        case WT_DUPLICATE_KEY:
            return -1;
        case WT_ROLLBACK:
            session->rollback_transaction(session, NULL);
            return 1;
        default:
            session->rollback_transaction(session, NULL);
            throw GraphException(wiredtiger_strerror(ret));
    }
}

/**
 * @brief Insert an edge into the edge table
 *
 * @param to_insert The edge struct containing the edge to be inserted.
 */
int StandardGraph::add_edge(edge to_insert, bool is_bulk)
{
    int num_nodes_to_add = 0;
    int num_edges_to_add = 0;
    int ret = 0;
start_add_edge:
    num_nodes_to_add = 0;
    num_edges_to_add = 0;
    ret = 0;
    session->begin_transaction(session, "isolation=snapshot");
    // Ensure src and dst nodes exist
    if (!is_bulk)
    {
        node src{.id = to_insert.src_id};
        ret = error_check_add_edge(add_node_txn(src));
        if (ret == 1)
        {
            return WT_ROLLBACK;
            // goto start_add_edge;
        }
        else if (ret == 0)
        {
            num_nodes_to_add += 1;
        }

        node dst{.id = to_insert.dst_id};
        ret = error_check_add_edge(add_node_txn(dst));
        if (ret == 1)
        {
            return WT_ROLLBACK;
            // goto start_add_edge;
        }
        else if (ret == 0)
        {
            num_nodes_to_add += 1;
        }
    }

    edge_cursor->set_key(edge_cursor, to_insert.src_id, to_insert.dst_id);

    if (opts.is_weighted)
    {
        edge_cursor->set_value(edge_cursor, to_insert.edge_weight);
    }
    else
    {
        edge_cursor->set_value(edge_cursor, 0);
    }

    ret = edge_cursor->insert(edge_cursor);

    switch (ret)
    {
        case 0:
            num_edges_to_add += 1;
            break;
        case WT_ROLLBACK:
            session->rollback_transaction(session, NULL);
            return WT_ROLLBACK;
            // goto start_add_edge;
        case WT_DUPLICATE_KEY:
            session->rollback_transaction(session, NULL);
            return WT_DUPLICATE_KEY;
            // return;
        default:
            session->rollback_transaction(session, NULL);
            throw GraphException("Failed to insert edge between " +
                                 std::to_string(to_insert.src_id) + " and " +
                                 std::to_string(to_insert.dst_id) +
                                 wiredtiger_strerror(ret));
    }

    if (!opts.is_directed)
    {
        edge_cursor->set_key(edge_cursor, to_insert.dst_id, to_insert.src_id);

        if (opts.is_weighted)
        {
            edge_cursor->set_value(edge_cursor, to_insert.edge_weight);
        }
        else
        {
            edge_cursor->set_value(edge_cursor, 0);
        }

        ret = edge_cursor->insert(edge_cursor);

        switch (ret)
        {
            case 0:
                num_edges_to_add += 1;
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, NULL);
                return WT_ROLLBACK;
                // goto start_add_edge;
            case WT_DUPLICATE_KEY:
                session->rollback_transaction(session, NULL);
                return WT_DUPLICATE_KEY;
                // return;
            default:
                session->rollback_transaction(session, NULL);
                throw GraphException("Failed to insert edge between " +
                                     std::to_string(to_insert.src_id) +
                                     " and " +
                                     std::to_string(to_insert.dst_id) +
                                     wiredtiger_strerror(ret));
        }
    }

    if (!is_bulk)
    {
        // If opts.read_optimized is true, we update in/out degreees in the node
        // table.
        if (this->opts.read_optimize)
        {
            node_cursor->set_key(node_cursor, to_insert.src_id);
            node_cursor->search(node_cursor);
            // update in/out degrees for src node in NODE_TABLE
            node found = {0};
            CommonUtil::record_to_node(node_cursor, &found, opts.read_optimize);
            found.id = to_insert.src_id;
            found.out_degree++;
            ret = update_node_degree(
                node_cursor,
                found.id,
                found.in_degree,
                found.out_degree);  //! pass the cursor to this function

            switch (ret)
            {
                case 0:
                    break;
                case WT_ROLLBACK:
                    session->rollback_transaction(session, NULL);
                    return WT_ROLLBACK;
                    // goto start_add_edge;
                case WT_NOTFOUND:
                // WT_NOTFOUND should not occur
                default:
                    session->rollback_transaction(session, NULL);
                    throw GraphException("Could not update the node with ID" +
                                         std::to_string(to_insert.src_id) +
                                         wiredtiger_strerror(ret));
            }

            // update in/out degrees for the dst node in the NODE_TABLE
            node_cursor->set_key(node_cursor, to_insert.dst_id);
            node_cursor->search(node_cursor);

            CommonUtil::record_to_node(node_cursor, &found, opts.read_optimize);
            found.id = to_insert.dst_id;
            found.in_degree++;
            ret = update_node_degree(
                node_cursor, found.id, found.in_degree, found.out_degree);
            switch (ret)
            {
                case 0:
                    break;
                case WT_ROLLBACK:
                    session->rollback_transaction(session, NULL);
                    return WT_ROLLBACK;
                    // goto start_add_edge;
                case WT_NOTFOUND:
                    // WT_NOTFOUND should not occur
                default:
                    session->rollback_transaction(session, NULL);
                    throw GraphException("Could not update the node with ID" +
                                         std::to_string(to_insert.dst_id) +
                                         wiredtiger_strerror(ret));
            }
        }
    }

    session->commit_transaction(session, NULL);
    add_to_nedges(num_edges_to_add);
    add_to_nnodes(num_nodes_to_add);
    return 0;
}

int StandardGraph::delete_edge_txn(node_id_t src_id,
                                   node_id_t dst_id,
                                   int *num_edges_to_add_ptr)
{
    node n_found;
    int ret = 0;

    edge_cursor->set_key(edge_cursor, src_id, dst_id);
    ret = edge_cursor->remove(edge_cursor);

    switch (ret)
    {
        case 0:
            *num_edges_to_add_ptr -= 1;
            break;
        case WT_ROLLBACK:
            return WT_ROLLBACK;
        case WT_NOTFOUND:
        default:
            session->rollback_transaction(session, NULL);
            throw GraphException("Failed to delete edge (" + to_string(src_id) +
                                 "," + to_string(dst_id));
    }

    // Delete reverse edge is handled by caller

    // Update in/out degrees for the src and dst nodes if the graph is read
    // optimized

    // Update src node's in/out degrees
    if (opts.read_optimize)
    {
        n_found = {.id = src_id, .in_degree = 0, .out_degree = 0};
        node_cursor->set_key(node_cursor, n_found.id);
        node_cursor->search(node_cursor);
        CommonUtil::record_to_node(node_cursor, &n_found, opts.read_optimize);

        n_found.out_degree = n_found.out_degree - 1;

        ret = update_node_degree(
            node_cursor, n_found.id, n_found.in_degree, n_found.out_degree);

        switch (ret)
        {
            case 0:
                break;
            case WT_ROLLBACK:
                return WT_ROLLBACK;
            case WT_NOTFOUND:
            default:
                session->rollback_transaction(session, NULL);
        }
        // Update dst node's in/out degrees
        n_found = {.id = dst_id, .in_degree = 0, .out_degree = 0};
        node_cursor->set_key(node_cursor, n_found.id);
        node_cursor->search(node_cursor);
        CommonUtil::record_to_node(node_cursor, &n_found, opts.read_optimize);

        n_found.in_degree = n_found.in_degree - 1;

        ret = update_node_degree(
            node_cursor, n_found.id, n_found.in_degree, n_found.out_degree);

        switch (ret)
        {
            case 0:
                break;
            case WT_ROLLBACK:
                return WT_ROLLBACK;
            case WT_NOTFOUND:
            default:
                session->rollback_transaction(session, NULL);
        }
    }
    return 0;
}

int StandardGraph::delete_edge(node_id_t src_id, node_id_t dst_id)
{
    int num_edges_to_add = 0;
    int ret = 0;
start_delete_edge:
    num_edges_to_add = 0;
    ret = 0;
    session->begin_transaction(session, "isolation=snapshot");

    edge_cursor->set_key(edge_cursor, src_id, dst_id);
    ret = edge_cursor->remove(edge_cursor);

    switch (ret)
    {
        case 0:
            num_edges_to_add -= 1;
            break;
        case WT_ROLLBACK:
            session->rollback_transaction(session, NULL);
            return WT_ROLLBACK;
            // goto start_delete_edge;
        case WT_NOTFOUND:
            session->rollback_transaction(session, NULL);
            return WT_NOTFOUND;
            // return;
        default:
            session->rollback_transaction(session, NULL);
            throw GraphException(
                "Failed to delete the edge between " + std::to_string(src_id) +
                " and " + std::to_string(dst_id) + wiredtiger_strerror(ret));
    }

    // Delete reverse edge if the graph is undirected.
    if (!opts.is_directed)
    {
        edge_cursor->set_key(edge_cursor, dst_id, src_id);
        ret = edge_cursor->remove(edge_cursor);
        switch (ret)
        {
            case 0:
                num_edges_to_add -= 1;
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, NULL);
                return WT_ROLLBACK;
                // goto start_delete_edge;
            case WT_NOTFOUND:
                session->rollback_transaction(session, NULL);
                return WT_NOTFOUND;
                // return;
            default:
                session->rollback_transaction(session, NULL);
                throw GraphException(
                    "Failed to delete the reverse edge between " +
                    std::to_string(src_id) + " and " + std::to_string(dst_id) +
                    wiredtiger_strerror(ret));
        }
    }
    edge_cursor->reset(edge_cursor);
    // Update in/out degrees for the src and dst nodes if the graph is read
    // optimized
    if (opts.read_optimize)
    {
        // Update src node's in/out degrees
        node n_found = {.id = src_id, .in_degree = 0, .out_degree = 0};
        node_cursor->set_key(node_cursor, n_found.id);
        node_cursor->search(node_cursor);
        CommonUtil::record_to_node(node_cursor, &n_found, opts.read_optimize);

        n_found.out_degree--;
        if (!opts.is_directed)
        {
            n_found.in_degree--;
        }
        ret = update_node_degree(
            node_cursor, n_found.id, n_found.in_degree, n_found.out_degree);

        switch (ret)
        {
            case 0:
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, NULL);
                return WT_ROLLBACK;
                // goto start_delete_edge;
            case WT_NOTFOUND:
            // WT_NOTFOUND should not occur
            default:
                session->rollback_transaction(session, NULL);
                throw GraphException("Could not update the node with ID" +
                                     std::to_string(src_id) +
                                     wiredtiger_strerror(ret));
        }
        // Update dst node's in/out degrees
        n_found = {.id = dst_id, .in_degree = 0, .out_degree = 0};
        node_cursor->set_key(node_cursor, n_found.id);
        node_cursor->search(node_cursor);

        CommonUtil::record_to_node(node_cursor, &n_found, opts.read_optimize);

        n_found.in_degree--;
        if (!opts.is_directed)
        {
            n_found.out_degree--;
        }
        ret = update_node_degree(
            node_cursor, n_found.id, n_found.in_degree, n_found.out_degree);

        switch (ret)
        {
            case 0:
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, NULL);
                return WT_ROLLBACK;
                // goto start_delete_edge;
            case WT_NOTFOUND:
            // WT_NOTFOUND should not occur
            default:
                session->rollback_transaction(session, NULL);
                throw GraphException("Could not update the node with ID" +
                                     std::to_string(dst_id) +
                                     wiredtiger_strerror(ret));
        }
    }
    session->commit_transaction(session, NULL);
    add_to_nedges(num_edges_to_add);
    return 0;
}

int StandardGraph::update_node_degree(WT_CURSOR *cursor,
                                      node_id_t node_id,
                                      degree_t in_degree,
                                      degree_t out_degree)
{
    cursor->set_key(cursor, node_id);
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

/**
 * @brief Get all edges in the edge table
 *
 * @return std::vector<edge> the vector containing all edges in the edge table
 */
std::vector<edge> StandardGraph::get_edges()
{
    int ret;
    vector<edge> edges;
    while ((ret = edge_cursor->next(edge_cursor) == 0))
    {
        edge found = {-1, -1, -1};
        edge_cursor->get_key(edge_cursor, &found.src_id, &found.dst_id);
        if (opts.is_weighted)
        {
            CommonUtil::record_to_edge(edge_cursor, &found);
        }
        edges.push_back(found);
    }
    edge_cursor->reset(edge_cursor);

    return edges;
}

/**
 * @brief This function returns a vector of outgoing edges from node_id
 *
 * @param node_id the ID of the node whose out edges are being searched
 * @return std::vector<edge> vector of out edges found
 */
std::vector<edge> StandardGraph::get_out_edges(node_id_t node_id)
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
        edge found = {-1, -1, -1};
        CommonUtil::read_from_edge_idx(src_index_cursor, &found);
        edges.push_back(found);
        while (src_index_cursor->next(src_index_cursor) == 0)
        {
            CommonUtil::read_from_edge_idx(src_index_cursor, &found);
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

/**
 * @brief This function is used to get all the nodes that have an incoming edge
 * from node_id
 * @param node_id the node for which out edges are being searched
 * @return vector<node> The vector of out nodes
 */
vector<node> StandardGraph::get_out_nodes(node_id_t node_id)
{
    vector<edge> out_edges;
    vector<node> nodes;
    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    out_edges = get_out_edges(node_id);
    for (edge out_edge : out_edges)
    {
        nodes.push_back(get_node(out_edge.dst_id));
    }
    return nodes;
}

/**
 * @brief This function is used to get ids of all the nodes that have an
 * incoming edge from node_id; uses one less cursor search than get_out_nodes
 *
 * @param node_id the node for which out edges are being searched
 * @return vector<node> The vector of out nodes
 */
vector<node_id_t> StandardGraph::get_out_nodes_id(node_id_t node_id)
{
    vector<edge> out_edges;
    vector<node_id_t> node_ids;
    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    out_edges = get_out_edges(node_id);
    for (edge out_edge : out_edges)
    {
        node_ids.push_back(out_edge.dst_id);
    }
    return node_ids;
}

/**
 * @brief This is used to get in edges to the node_id.
 *
 * @param node_id
 * @return vector<edge>
 */
vector<edge> StandardGraph::get_in_edges(node_id_t node_id)
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
        edge found = {-1, -1, -1};
        CommonUtil::read_from_edge_idx(dst_index_cursor, &found);
        edges.push_back(found);
        while (dst_index_cursor->next(dst_index_cursor) == 0)
        {
            CommonUtil::read_from_edge_idx(dst_index_cursor, &found);
            if (found.dst_id == node_id)
            {
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

/**
 * @brief This function is used to get the list of nodes that have edges to
 * node_id
 *
 * @param node_id for identifying the node to search
 * @return vector<node> set of nodes that have edges to node_id
 */
vector<node> StandardGraph::get_in_nodes(node_id_t node_id)
{
    vector<node> nodes;
    if (!has_node(node_id))
    {
        throw GraphException("Could not find node with id" +
                             to_string(node_id));
    }

    vector<edge> in_edges = get_in_edges(node_id);
    for (edge in_edge : in_edges)
    {
        nodes.push_back(get_node(in_edge.src_id));
    }
    return nodes;
}

/**
 * @brief This function is used to get the the ids of list of nodes that have
 * edges to node_id; uses one less search than get_in_nodes
 *
 * @param node_id for identifying the node to search
 * @return vector<node_ids> Id of set of nodes that have edges to node_id
 */
vector<node_id_t> StandardGraph::get_in_nodes_id(node_id_t node_id)
{
    vector<node_id_t> node_ids;
    if (!has_node(node_id))
    {
        throw GraphException("Could not find node with id" +
                             to_string(node_id));
    }

    vector<edge> in_edges = get_in_edges(node_id);
    for (edge in_edge : in_edges)
    {
        node_ids.push_back(in_edge.src_id);
    }
    return node_ids;
}

WT_CURSOR *StandardGraph::get_node_cursor()
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

WT_CURSOR *StandardGraph::get_new_node_cursor()
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

WT_CURSOR *StandardGraph::get_edge_cursor()
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

WT_CURSOR *StandardGraph::get_new_edge_cursor()
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

WT_CURSOR *StandardGraph::get_src_idx_cursor()
{
    if (src_index_cursor == nullptr)
    {
        string projection = "(" + SRC + "," + DST + ")";
        if (_get_index_cursor(
                EDGE_TABLE, SRC_INDEX, projection, &(src_index_cursor)) != 0)
        {
            throw GraphException(
                "Could not get a SRC index cursor on the edge table");
        }
    }

    return src_index_cursor;
}

WT_CURSOR *StandardGraph::get_new_src_idx_cursor()
{
    WT_CURSOR *new_src_index_cursor = nullptr;
    string projection = "(" + SRC + "," + DST + ")";
    if (_get_index_cursor(
            EDGE_TABLE, SRC_INDEX, projection, &new_src_index_cursor) != 0)
    {
        throw GraphException(
            "Could not get a SRC index cursor on the edge table");
    }

    return new_src_index_cursor;
}

WT_CURSOR *StandardGraph::get_dst_idx_cursor()
{
    if (dst_index_cursor == nullptr)
    {
        string projection = "(" + SRC + "," + DST + ")";
        if (_get_index_cursor(
                EDGE_TABLE, DST_INDEX, projection, &(dst_index_cursor)) != 0)
        {
            throw GraphException(
                "Could not get a DST index cursor on the edge table");
        }
    }

    return dst_index_cursor;
}

WT_CURSOR *StandardGraph::get_new_dst_idx_cursor()
{
    WT_CURSOR *new_dst_index_cursor = nullptr;
    string projection = "(" + SRC + "," + DST + ")";
    if (_get_index_cursor(
            EDGE_TABLE, DST_INDEX, projection, &new_dst_index_cursor) != 0)
    {
        throw GraphException(
            "Could not get a DST index cursor on the edge table");
    }

    return new_dst_index_cursor;
}

WT_CURSOR *StandardGraph::get_src_dst_idx_cursor()
{
    if (src_dst_index_cursor == nullptr)
    {
        string projection = "(" + SRC + "," + DST + ")";
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

WT_CURSOR *StandardGraph::get_new_src_dst_idx_cursor()
{
    WT_CURSOR *new_src_dst_index_cursor = nullptr;
    string projection = "(" + SRC + "," + DST + ")";
    if (_get_index_cursor(
            EDGE_TABLE, SRC_DST_INDEX, projection, &new_src_dst_index_cursor) !=
        0)
    {
        throw GraphException(
            "Could not get a DST index cursor on the edge table");
    }

    return new_src_dst_index_cursor;
}

OutCursor *StandardGraph::get_outnbd_iter()
{
    uint64_t num_nodes = this->get_num_nodes();
    OutCursor *toReturn = new StdOutCursor(get_new_src_idx_cursor(), session);
    toReturn->set_num_nodes(num_nodes);
    toReturn->set_key_range({-1, (int64_t)num_nodes - 1});
    return toReturn;
}

InCursor *StandardGraph::get_innbd_iter()
{
    uint64_t num_nodes = this->get_num_nodes();
    InCursor *toReturn = new StdInCursor(get_new_dst_idx_cursor(), session);
    toReturn->set_num_nodes(num_nodes);
    toReturn->set_key_range({-1, (int64_t)num_nodes - 1});
    return toReturn;
}

NodeCursor *StandardGraph::get_node_iter()
{
    return new StdNodeCursor(get_new_node_cursor(), session);
}

EdgeCursor *StandardGraph::get_edge_iter()
{
    return new StdEdgeCursor(get_new_edge_cursor(), session);
}

/**
 * @brief This is a function that I am using to test how search using
 * cursors works. It returns a cursor pointing to the first entry where the
 * condition is true and you can call next() on the cursor to get all
 * records that match the condition -- Break when the condition is not true
 * anymore.
 *
 * @param node_id
 * @return vector<edge>
 */

// WT_CURSOR *StandardGraph::get_node_iter() { return get_node_cursor(); }

node StandardGraph::get_next_node(WT_CURSOR *n_iter)
{
    node found = {-1};
    if (n_iter->next(n_iter) == 0)
    {
        CommonUtil::record_to_node(n_iter, &found, opts.read_optimize);
        n_iter->get_key(n_iter, &found.id);
    }

    return found;
}
// WT_CURSOR *StandardGraph::get_edge_iter() { return get_edge_cursor(); }

edge StandardGraph::get_next_edge(WT_CURSOR *e_iter)
{
    if (e_iter->next(e_iter) == 0)
    {
        edge found;
        CommonUtil::record_to_edge(e_iter, &found);
        e_iter->get_key(e_iter, &found.src_id, &found.dst_id);
        return found;
    }
    else
    {
        edge not_found = {-1};
        return not_found;
    }
}

vector<edge> StandardGraph::test_cursor_iter(node_id_t node_id)
{
    vector<edge> edges;
    if (!has_node(node_id))
    {
        throw GraphException("Could not find node with id" +
                             to_string(node_id));
    }

    WT_CURSOR *cursor = get_src_idx_cursor();

    cursor->set_key(cursor, node_id);
    if (cursor->search(cursor) == 0)
    {
        edge found;
        CommonUtil::read_from_edge_idx(cursor, &found);
        edges.push_back(found);
        while (cursor->next(cursor) == 0)
        {
            CommonUtil::read_from_edge_idx(cursor, &found);
            edges.push_back(found);
        }
    }
    return edges;
}
