#include "edgekey.h"

#include <wiredtiger.h>

#include <algorithm>
#include <cassert>
#include <string>

#include "common_util.h"
using namespace std;
[[maybe_unused]] const std::string GRAPH_PREFIX = "edgekey";

EdgeKey::EdgeKey(graph_opts &opt_params, WT_CONNECTION *conn)
    : GraphBase(opt_params, conn)
{
    init_cursors();
}

/**
 * @brief Create a new graph object
 * The edge table has : <src> <dst> <values>
 * if this is a node: <node_id> <-1> <in_degree, out_degree>
 * if this is an edge : <src><dst><edge_weight>
 */
void EdgeKey::create_wt_tables(graph_opts &opts, WT_CONNECTION *conn)
{
    WT_SESSION *sess;
    if (CommonUtil::open_session(conn, &sess) != 0)
    {
        throw GraphException("Cannot open session");
    }
    // Set up the edge table
    // Edge Columns: <src> <dst> <weight/in_degree> <out_degree>
    // Edge Column Format: iiS
    vector<string> edge_columns = {SRC, DST, ATTR_FIRST, ATTR_SECOND};
    string edge_key_format = "uu";    // SRC DST
    string edge_value_format = "II";  // in/out degree(unit32_t)
    CommonUtil::set_table(
        sess, EDGE_TABLE, edge_columns, edge_key_format, edge_value_format);
    if (!opts.optimize_create)
    {
        create_indices(sess);
    }
    sess->close(sess, nullptr);
}

/**
 * @brief Creates the indices on the SRC and the DST column of the edge
 * table. These are not (and should not be) used for inserting data into the
 * edge table if write_optimize is on. Once the data has been inserted, the
 * create_indices function must be called separately.
 *
 * This function requires exclusive access to the table on which it
 * operates. If there are any open cursors, or if the table has any other
 * operation ongoing, WT throws a Resource Busy error.
 *
 */
void EdgeKey::create_indices(WT_SESSION *sess)
{
    std::string idx_name, idx_conf;
    // Index on (DST,SRC) columns of the edge table
    // Used for adjacency neighbourhood iterators
    idx_name = "index:" + EDGE_TABLE + ":" + DST_SRC_INDEX;
    idx_conf = "columns=(" + DST + "," + SRC + ")";
    if (sess->create(sess, idx_name.c_str(), idx_conf.c_str()) != 0)
    {
        throw GraphException(
            "Failed to create DST_SRC_INDEX on the edge table");
    }
}

void EdgeKey::init_cursors()
{
    int ret =
        _get_table_cursor(METADATA, &metadata_cursor, session, false, true);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the metadata table:" +
                             string(wiredtiger_strerror(ret)));
    }

    // edge_cursor
    ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, session, false, true);
    if (ret != 0)
    {
        throw GraphException("Could not get an edge cursor: " +
                             string(wiredtiger_strerror(ret)));
    }
    // dst_src_cursor

    std::string projection = "(" + ATTR_FIRST + "," + ATTR_SECOND + ")";
    ret = _get_index_cursor(
        EDGE_TABLE, DST_SRC_INDEX, projection, &dst_src_idx_cursor);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the dst_src index" +
                             string(wiredtiger_strerror(ret)));
    }
}

/**
 * @brief get the node identified by node_id
 *
 * @param node_id the Node ID
 * @return node the node struct containing the node
 */
node EdgeKey::get_node(node_id_t node_id)
{
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
    CommonUtil::ekey_set_key(edge_cursor, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////

    node found = {0};
    if (edge_cursor->search(edge_cursor) == 0)
    {
        found.id = node_id;
        CommonUtil::record_to_node_ekey(edge_cursor, &found);
    }
    edge_cursor->reset(edge_cursor);
    return found;
}

/**
 * @brief Returns a random node entry in the edge table. A random cursor to
 * the edge table will return a random record which could be an edge or a
 * node. since the number of nodes is much smaller than the number of edges,
 * it is unlikely that calling next() on the cursor will yield a node. It is
 * much more efficient to return the node record of the src node of the edge
 * we encounter.
 *
 * @return node random node we find
 */
node EdgeKey::get_random_node()
{
    node rando = {0};
    int ret = 0;
    if (random_cursor == nullptr)
    {
        ret =
            _get_table_cursor(EDGE_TABLE, &random_cursor, session, true, true);
    }
    if (ret != 0)
    {
        throw GraphException("Could not get a random cursor to the node table");
    }
    while ((ret = random_cursor->next(random_cursor)) == 0)
    {
        node_id_t src, dst;
        ret = CommonUtil::ekey_get_key(random_cursor, &src, &dst);
        if (ret != 0)
        {
            break;
        }
        if (dst == OutOfBand_ID)
        {
            // random_cur->ekey_set_key(random_cur, src, dst);
            /////////////////////////////////////////////////////////////////////////
            // rando.id = OG_KEY(src);
            rando.id = src;
            /////////////////////////////////////////////////////////////////////////
            CommonUtil::record_to_node_ekey(random_cursor, &rando);
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

/**
 * @brief This function adds a node to the edge table.
 *
 * @param to_insert the node to be inserted into the edge table
 */
int EdgeKey::add_node(node to_insert)
{
    // start_add_node:
    session->begin_transaction(session, "isolation=snapshot");

    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(to_insert.id),
    // OutOfBand_ID);
    CommonUtil::ekey_set_key(edge_cursor, to_insert.id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////

    if (opts.read_optimize)
    {
        edge_cursor->set_value(
            edge_cursor, to_insert.in_degree, to_insert.out_degree);
    }
    else
    {
        edge_cursor->set_value(edge_cursor, 0, 0);
    }

    switch (edge_cursor->insert(edge_cursor))
    {
        case 0:
            break;
        case WT_ROLLBACK:
            session->rollback_transaction(session, nullptr);
            return WT_ROLLBACK;
            // goto start_add_node;
        case WT_DUPLICATE_KEY:
            session->rollback_transaction(session, nullptr);
            return WT_DUPLICATE_KEY;
            // return;
        default:
            session->rollback_transaction(session, nullptr);
            throw GraphException("Failed to insert a node with ID " +
                                 std::to_string(to_insert.id) +
                                 " into the edge table");
    }
    session->commit_transaction(session, nullptr);
    // add_to_nnodes(1);
    return 0;
}

int EdgeKey::add_node_txn(node to_insert)
{
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(to_insert.id),
    // OutOfBand_ID);
    CommonUtil::ekey_set_key(edge_cursor, to_insert.id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////

    if (opts.read_optimize)
    {
        edge_cursor->set_value(
            edge_cursor, to_insert.in_degree, to_insert.out_degree);
    }
    else
    {
        edge_cursor->set_value(edge_cursor, 0, OutOfBand_Val);
    }
    return edge_cursor->insert(edge_cursor);
}

/**
 * @brief returns true if a node with id node_id exists in the edge table
 *
 * @param node_id the ID to look for
 * @return true / false depending on if node_id is found or not
 */
bool EdgeKey::has_node(node_id_t node_id)
{
    int ret;
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
    CommonUtil::ekey_set_key(edge_cursor, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////

    ret = edge_cursor->search(edge_cursor);
    edge_cursor->reset(edge_cursor);
    return (ret == 0);
}

/**
 * @brief returns true if there is an edge found between src_id and dst_id
 * nodes
 *
 * @param src_id source node for the edge to search
 * @param dst_id destination node for the edge to search
 * @return true/false id edge was found or not
 */
bool EdgeKey::has_edge(node_id_t src_id, node_id_t dst_id)
{
    int ret;
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(src_id),
    // MAKE_EKEY(dst_id));
    CommonUtil::ekey_set_key(edge_cursor, src_id, dst_id);
    /////////////////////////////////////////////////////////////////////////

    ret = edge_cursor->search(edge_cursor);
    edge_cursor->reset(edge_cursor);
    return (ret == 0);
}

int EdgeKey::delete_node(node_id_t node_id)
{
    int num_edges_to_add = 0;
    session->begin_transaction(session, "isolation=snapshot");

    int ret = delete_node_and_related_edges(node_id, &num_edges_to_add);

    session->commit_transaction(session, nullptr);
    if (ret != 0) return ret;

    // add_to_nedges(num_edges_to_add);
    // add_to_nnodes(-1);
    return 0;
}

int EdgeKey::delete_node_and_related_edges(node_id_t node_id,
                                           int *num_edges_to_add)  // TODO
{
    int ret;
    node_id_t src, dst;
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
    CommonUtil::ekey_set_key(edge_cursor, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////
    if (edge_cursor->search(edge_cursor) != 0)
    {
        return 0;
    }
    // We have now successfully found the node in the edge table
    // Now we duplicate the cursor and iterate over the edges till we hit the
    // next node (or the end of the table) such that the found node id >
    // provided node id.
    WT_CURSOR *del_cursor;
    CommonUtil::dup_cursor(session, edge_cursor, &del_cursor);
    while (del_cursor->next(del_cursor) == 0)
    {
        /////////////////////////////////////////////////////////////////////////
        // CommonUtil::get_key(del_cursor, &src, &dst);
        CommonUtil::ekey_get_key(del_cursor, &src, &dst);
        // if (src != MAKE_EKEY(node_id))  // We have reached the end of the out
        // edges for the node
        if (src != node_id)
        /////////////////////////////////////////////////////////////////////////
        {
            break;
        }
        // Delete the edge FROM the deleted node
        ret = del_cursor->remove(del_cursor);
        if (ret != 0)
        {
            session->rollback_transaction(session, nullptr);
            throw GraphException("Failed to remove edge between " +
                                 to_string(src) + " and " + to_string(dst) +
                                 wiredtiger_strerror(ret));
        }
        else
        {  // We have successfully deleted the edge
            *num_edges_to_add -= 1;
        }

        // if readoptimize: Get the node to decrement in/out degree
        if (opts.read_optimize)
        {
            /////////////////////////////////////////////////////////////////////////
            node temp = get_node(dst);
            // node temp = get_node(OG_KEY(dst));
            /////////////////////////////////////////////////////////////////////////
            ret = update_node_degree(
                temp.id, temp.in_degree - 1, temp.out_degree);
            if (ret != 0)
            {
                session->rollback_transaction(session, nullptr);
                throw GraphException("Could not update the node with ID" +
                                     std::to_string(temp.id) +
                                     wiredtiger_strerror(ret));
            }
        }
    }
    del_cursor->close(del_cursor);

    // Now decrement the in degrees of nodes that had an edge from the deleted
    // node.
    if (opts.read_optimize)
    {
        dst_src_idx_cursor->reset(dst_src_idx_cursor);
        /////////////////////////////////////////////////////////////////////////
        // CommonUtil::ekey_set_key(dst_src_idx_cursor, MAKE_EKEY(node_id),
        // OutOfBand_ID);
        CommonUtil::ekey_set_key(dst_src_idx_cursor, node_id, OutOfBand_ID);
        /////////////////////////////////////////////////////////////////////////
        int exact_match;
        ret = dst_src_idx_cursor->search_near(dst_src_idx_cursor, &exact_match);
        if (exact_match < 0 && ret != WT_NOTFOUND)
        {
            // There is no outgoing edge from the deleted node
            goto done;
        }
        else if (exact_match >
                 0) /* We cannot have an exact match on the dst,src
                     index. All the nodes are at the beginning of the
                     index. (see the dump of the index if in doubt)*/
        {
            node_id_t src_, dst_;
            do
            {
                /////////////////////////////////////////////////////////////////////////
                // CommonUtil::get_key(dst_src_idx_cursor, &dst_, &src_);
                // if (dst_ != MAKE_EKEY(node_id)) break;
                CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst_, &src_);
                if (dst_ != node_id) break;
                // node temp = get_node(OG_KEY(src_));
                node temp = get_node(src_);
                /////////////////////////////////////////////////////////////////////////

                /*  we are decrementing in-deg
                for the node that had
                an incoming edge from the deleted node*/
                ret = update_node_degree(
                    temp.id, temp.in_degree, temp.out_degree - 1);
                if (ret != 0)
                {
                    session->rollback_transaction(session, nullptr);
                    throw GraphException("Could not update the node with ID" +
                                         std::to_string(temp.id) +
                                         wiredtiger_strerror(ret));
                }
                // Now delete the edge TO the deleted node
                /////////////////////////////////////////////////////////////////////////
                // CommonUtil::ekey_set_key(edge_cursor, src_, dst_);
                CommonUtil::ekey_set_key(edge_cursor, src_, dst_);
                /////////////////////////////////////////////////////////////////////////

                ret = edge_cursor->remove(edge_cursor);
                if (ret != 0)
                {
                    session->rollback_transaction(session, nullptr);
                    throw GraphException(
                        "Failed to remove edge between " + to_string(src_) +
                        " and " + to_string(dst_) + wiredtiger_strerror(ret));
                }
                else
                {
                    *num_edges_to_add -= 1;
                }
            } while (dst_src_idx_cursor->next(dst_src_idx_cursor) == 0 &&
                     dst_ == MAKE_EKEY(node_id));
        }
    done:
        dst_src_idx_cursor->reset(dst_src_idx_cursor);
    }

    ////////////////////////////////////////////////////////////////////////////
    //  remove the original node in the edge table
    //  CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
    CommonUtil::ekey_set_key(edge_cursor, node_id, OutOfBand_ID);
    ////////////////////////////////////////////////////////////////////////////

    ret = edge_cursor->remove(edge_cursor);
    if (ret != 0)
    {
        session->rollback_transaction(session, nullptr);
        throw GraphException("Failed to remove node with ID " +
                             std::to_string(node_id) +
                             wiredtiger_strerror(ret));
    }
    edge_cursor->reset(edge_cursor);
    return 0;
}

/**
 * @brief update the in and out degrees for the node identified by node_id
 * Does nothing if not opts.read_optimize
 * @param node_id The ID of the node to be updated
 * @param indeg new in_degree
 * @param outdeg new out_degree
 */
int EdgeKey::update_node_degree(node_id_t node_id,
                                degree_t indeg,
                                degree_t outdeg)
{
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
    CommonUtil::ekey_set_key(edge_cursor, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////
    edge_cursor->set_value(edge_cursor, indeg, outdeg);
    int ret = edge_cursor->update(edge_cursor);
    edge_cursor->reset(edge_cursor);
    return ret;
}

// Caller should continue on return value = 0 and retry on return value = 1
int EdgeKey::error_check_add_edge(int ret)
{
    switch (ret)
    {
        case 0:
            return 0;
        case WT_DUPLICATE_KEY:
            return -1;
        case WT_ROLLBACK:
            session->rollback_transaction(session, nullptr);
            return 1;
        default:
            throw GraphException(wiredtiger_strerror(ret));
    }
}

int EdgeKey::add_edge_only(edge to_insert)
{
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(to_insert.src_id),
    // MAKE_EKEY(to_insert.dst_id));
    CommonUtil::ekey_set_key(edge_cursor, to_insert.src_id, to_insert.dst_id);
    /////////////////////////////////////////////////////////////////////////

    if (opts.is_weighted)
    {
        edge_cursor->set_value(
            edge_cursor, to_insert.edge_weight, OutOfBand_Val);
    }
    else
    {
        edge_cursor->set_value(edge_cursor, 0, OutOfBand_Val);
    }

    int ret = edge_cursor->insert(edge_cursor);
    switch (ret)
    {
        case 0:
            break;
        case WT_ROLLBACK:
            session->rollback_transaction(session, nullptr);
            return WT_ROLLBACK;
            // goto start_add_edge;
        case WT_DUPLICATE_KEY:
            session->rollback_transaction(session, nullptr);
            return WT_DUPLICATE_KEY;
            // return;
        default:
            session->rollback_transaction(session, nullptr);
            throw GraphException("Failed to insert edge between " +
                                 std::to_string(to_insert.src_id) + " and " +
                                 std::to_string(to_insert.dst_id) +
                                 wiredtiger_strerror(ret));
    }
    return 0;
}

/**
 * @brief This function adds the edge passed as param
 *
 * @param to_insert the edge struct containing info about the edge to
 * insert.
 */
int EdgeKey::add_edge(edge to_insert, bool is_bulk)
{
    int num_nodes_to_add = 0;
    int num_edges_to_add = 0;
    int ret;
    // start_add_edge:
    session->begin_transaction(session, "isolation=snapshot");

    // Ensure src and dst nodes exist
    if (!is_bulk)
    {
        if (!has_node(to_insert.src_id))
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
        }
        if (!has_node(to_insert.dst_id))
        {
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
    }

    // insert edge
    if (add_edge_only(to_insert) == 0)
    {
        num_edges_to_add += 1;
    }
    else
    {
        throw GraphException("Failed to insert edge between " +
                             std::to_string(to_insert.src_id) + " and " +
                             std::to_string(to_insert.dst_id) +
                             ". Session rolled back.");
    }

    // insert reverse edge if undirected
    if (!opts.is_directed)
    {
        edge reverse_edge = {.src_id = to_insert.dst_id,
                             .dst_id = to_insert.src_id,
                             .edge_weight = to_insert.edge_weight};
        if (add_edge_only(reverse_edge) == 0)
        {
            num_edges_to_add += 1;
        }
        else
        {
            throw GraphException("Failed to insert edge between " +
                                 std::to_string(to_insert.dst_id) + " and " +
                                 std::to_string(to_insert.src_id) +
                                 ". Session rolled back.");
        }
    }

    if (!is_bulk)
    {
        // Update the in/out degrees if opts.read_optimize
        if (opts.read_optimize)
        {
            node src;

            src = get_node(to_insert.src_id);
            src.out_degree++;
            if (!opts.is_directed)
            {
                src.in_degree++;
            }

            ret = update_node_degree(src.id, src.in_degree, src.out_degree);
            switch (ret)
            {
                case 0:
                    break;
                case WT_ROLLBACK:
                    session->rollback_transaction(session, nullptr);
                    return WT_ROLLBACK;
                    // goto start_add_edge;
                case WT_NOTFOUND:
                // WT_NOTFOUND should not occur
                default:
                    session->rollback_transaction(session, nullptr);
                    throw GraphException("Could not update the node with ID" +
                                         std::to_string(src.id) +
                                         wiredtiger_strerror(ret));
            }

            node dst;

            dst = get_node(to_insert.dst_id);
            dst.in_degree++;
            if (!opts.is_directed)
            {
                dst.out_degree++;
            }

            ret = update_node_degree(dst.id, dst.in_degree, dst.out_degree);
            switch (ret)
            {
                case 0:
                    break;
                case WT_ROLLBACK:
                    session->rollback_transaction(session, nullptr);
                    return WT_ROLLBACK;
                    // goto start_add_edge;
                case WT_NOTFOUND:
                    // WT_NOTFOUND should not occur
                default:
                    session->rollback_transaction(session, nullptr);
                    throw GraphException("Could not update the node with ID" +
                                         std::to_string(dst.id) +
                                         wiredtiger_strerror(ret));
            }
        }
    }

    session->commit_transaction(session, nullptr);
    // add_to_nedges(num_edges_to_add);
    // add_to_nnodes(num_nodes_to_add);
    return 0;
}

/**
 * @brief Delete the edge identified by (src_id, dst_id)
 *
 * @param src_id Source node ID for the edge to be deleted
 * @param dst_id Dst node ID for the edge to be deleted
 */
int EdgeKey::delete_edge(node_id_t src_id, node_id_t dst_id)  // TODO
{
    int num_edges_to_add;
    int ret;
    // start_delete_edge:
    num_edges_to_add = 0;
    session->begin_transaction(session, "isolation=snapshot");

    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(src_id),
    // MAKE_EKEY(dst_id));
    CommonUtil::ekey_set_key(edge_cursor, src_id, dst_id);
    /////////////////////////////////////////////////////////////////////////
    ret = edge_cursor->remove(edge_cursor);

    switch (ret)
    {
        case 0:
            num_edges_to_add -= 1;
            break;
        case WT_ROLLBACK:
            session->rollback_transaction(session, nullptr);
            return WT_ROLLBACK;
            // goto start_delete_edge;
        case WT_NOTFOUND:
            session->rollback_transaction(session, nullptr);
            return WT_NOTFOUND;
            // return;
        default:
            session->rollback_transaction(session, nullptr);
            throw GraphException(
                "Failed to delete the edge between " + std::to_string(src_id) +
                " and " + std::to_string(dst_id) + wiredtiger_strerror(ret));
    }

    // delete reverse edge
    if (!opts.is_directed)
    {
        /////////////////////////////////////////////////////////////////////////
        // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(dst_id),
        // MAKE_EKEY(src_id));
        CommonUtil::ekey_set_key(edge_cursor, dst_id, src_id);
        /////////////////////////////////////////////////////////////////////////
        ret = edge_cursor->remove(edge_cursor);

        switch (ret)
        {
            case 0:
                num_edges_to_add -= 1;
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, nullptr);
                return WT_ROLLBACK;
                // goto start_delete_edge;
            case WT_NOTFOUND:
                session->rollback_transaction(session, nullptr);
                return WT_NOTFOUND;
                // return;
            default:
                session->rollback_transaction(session, nullptr);
                throw GraphException(
                    "Failed to delete the reverse edge between " +
                    std::to_string(src_id) + " and " + std::to_string(dst_id) +
                    wiredtiger_strerror(ret));
        }
    }
    edge_cursor->reset(edge_cursor);

    // update node degrees
    if (opts.read_optimize)
    {
        node src = get_node(src_id);

        if (src.out_degree > 0)
        {
            src.out_degree--;
        }

        if (!opts.is_directed && src.in_degree > 0)
        {
            src.in_degree--;
        }

        ret = update_node_degree(src.id, src.in_degree, src.out_degree);

        switch (ret)
        {
            case 0:
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, nullptr);
                return WT_ROLLBACK;
                // goto start_delete_edge;
            case WT_NOTFOUND:
            // WT_NOTFOUND should not occur
            default:
                session->rollback_transaction(session, nullptr);
                throw GraphException("Could not update the node with ID" +
                                     std::to_string(src.id) +
                                     wiredtiger_strerror(ret));
        }

        node dst = get_node(dst_id);

        if (dst.in_degree > 0)
        {
            dst.in_degree--;
        }
        if (!opts.is_directed && dst.out_degree > 0)
        {
            dst.out_degree--;
        }

        ret = update_node_degree(dst_id, dst.in_degree, dst.out_degree);

        switch (ret)
        {
            case 0:
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, nullptr);
                return WT_ROLLBACK;
                // goto start_delete_edge;
            case WT_NOTFOUND:
            // WT_NOTFOUND should not occur
            default:
                session->rollback_transaction(session, nullptr);
                throw GraphException("Could not update the node with ID" +
                                     std::to_string(src.id) +
                                     wiredtiger_strerror(ret));
        }
    }
    session->commit_transaction(session, nullptr);
    // add_to_nedges(num_edges_to_add);
    return 0;
}

/**
 * @brief Get the edge identified by (src_id, dst_id)
 *
 * @param src_id source node_id
 * @param dst_id destination node_id
 * @return edge the edge object found
 */
edge EdgeKey::get_edge(node_id_t src_id, node_id_t dst_id)
{
    edge found = {};
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(src_id),
    // MAKE_EKEY(dst_id));
    CommonUtil::ekey_set_key(edge_cursor, src_id, dst_id);
    /////////////////////////////////////////////////////////////////////////
    if (edge_cursor->search(edge_cursor) == 0)
    {
        found.src_id = src_id;
        found.dst_id = dst_id;
        if (opts.is_weighted)
            CommonUtil::record_to_edge_ekey(edge_cursor, &found);
    }
    return found;
}

/**
 * @brief Get all nodes in the table
 *
 * @return std::vector<node> the list containing all nodes. If the graph is not
 * read_optimized, the nodes will have no in/out degree set
 */
std::vector<node> EdgeKey::get_nodes()
{
    std::vector<node> nodes;

    int search_exact;
    /////////////////////////////////////////////////////////////////////////
    CommonUtil::ekey_set_key(dst_src_idx_cursor, OutOfBand_ID, OutOfBand_ID);
    //! NO NEED TO CHANGE THIS
    /////////////////////////////////////////////////////////////////////////
    dst_src_idx_cursor->search_near(dst_src_idx_cursor, &search_exact);
    if (search_exact <= 0)
    {
        throw GraphException(
            "get_nodes failed to position cursor at beginning");
        // this literally cannot  happen.
    }
    // the cursor is positioned at the first node in the table.
    node_id_t src, dst;
    do
    {
        node n;
        /////////////////////////////////////////////////////////////////////////
        // CommonUtil::get_key(dst_src_idx_cursor, &dst, &src);
        // if (dst != OutOfBand_ID) break;
        // n.id = OG_KEY(src);
        CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
        if (dst != OutOfBand_ID) break;
        n.id = src;
        /////////////////////////////////////////////////////////////////////////
        dst_src_idx_cursor->get_value(
            dst_src_idx_cursor, &n.in_degree, &n.out_degree);
        nodes.push_back(n);
    } while (dst_src_idx_cursor->next(dst_src_idx_cursor) == 0);

    edge_cursor->reset(edge_cursor);
    return nodes;
}

/**
 * @brief get the out_degree of the node that has ID = node_id
 *
 * @param node_id ID of the node whose out degree is sought
 * @return int outdegree of node with ID node_id
 */
degree_t EdgeKey::get_out_degree(node_id_t node_id)
{
    degree_t out_deg = 0;
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
    CommonUtil::ekey_set_key(edge_cursor, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////
    int ret = edge_cursor->search(edge_cursor);
    if (ret == 0)  // The node exists
    {
        if (opts.read_optimize)
        {
            node found = {0};
            found.id = node_id;
            CommonUtil::record_to_node_ekey(edge_cursor, &found);
            edge_cursor->reset(edge_cursor);
            out_deg = found.out_degree;
        }
        else
        {
            node_id_t src, dst;
            do
            {
                edge_cursor->next(edge_cursor);  // skip to the out_edges
                /////////////////////////////////////////////////////////////////////////
                // CommonUtil::get_key(edge_cursor, &src, &dst);
                CommonUtil::ekey_get_key(edge_cursor, &src, &dst);
                // if (src == MAKE_EKEY(node_id))
                if (src == node_id)
                {
                    out_deg++;
                }
                else
                {
                    break;
                }
                //} while (src == MAKE_EKEY(node_id) && dst != OutOfBand_ID);
            } while (src == node_id && dst != OutOfBand_ID);
            /////////////////////////////////////////////////////////////////////////
            edge_cursor->reset(edge_cursor);
        }
    }
    else
    {
        edge_cursor->reset(edge_cursor);
        throw GraphException("Node with ID " + std::to_string(node_id) +
                             " does not exist");
    }

    return out_deg;
}

/**
 * @brief Returns the in_degree for the node with ID node_id
 *
 * @param node_id the ID of the node whose in_degree is sought
 * @return int the indegree found/computed.
 */
degree_t EdgeKey::get_in_degree(node_id_t node_id)
{
    if (opts.read_optimize)
    {
        /////////////////////////////////////////////////////////////////////////
        // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(node_id),
        // OutOfBand_ID);
        CommonUtil::ekey_set_key(edge_cursor, node_id, OutOfBand_ID);
        /////////////////////////////////////////////////////////////////////////
        edge_cursor->search(edge_cursor);
        node found = {0};
        CommonUtil::record_to_node_ekey(edge_cursor, &found);
        edge_cursor->reset(edge_cursor);
        return found.in_degree;
    }
    else
    {
        degree_t in_degree = 0;
        /////////////////////////////////////////////////////////////////////////
        // CommonUtil::ekey_set_key(dst_src_idx_cursor, MAKE_EKEY(node_id),
        // OutOfBand_ID);
        CommonUtil::ekey_set_key(dst_src_idx_cursor, node_id, OutOfBand_ID);
        /////////////////////////////////////////////////////////////////////////
        int search_exact;
        node_id_t src, dst;
        dst_src_idx_cursor->search_near(dst_src_idx_cursor, &search_exact);
        if (search_exact <= 0)
        {
            dst_src_idx_cursor->next(dst_src_idx_cursor);
            /////////////////////////////////////////////////////////////////////////
            // CommonUtil::get_key(dst_src_idx_cursor, &dst, &src);
            CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
            // assert(OG_KEY(dst) == node_id);
            assert(dst == node_id);
            assert(src > OutOfBand_ID);
            /////////////////////////////////////////////////////////////////////////
            // the cursor points to edges so the src value
            // here is definitely > out of band value
        }

        do
        {
            /////////////////////////////////////////////////////////////////////////
            // CommonUtil::get_key(dst_src_idx_cursor, &dst, &src);
            CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
            // if (dst == MAKE_EKEY(node_id))
            if (dst == node_id)
            /////////////////////////////////////////////////////////////////////////
            {
                if (src != OutOfBand_ID)
                {
                    in_degree++;
                }
            }
            else
            {
                break;
            }
        } while (dst_src_idx_cursor->next(dst_src_idx_cursor) == 0);

        dst_src_idx_cursor->reset(dst_src_idx_cursor);
        return in_degree;
    }
}

/**
 * @brief Get a list of all edges in the table
 *
 * @return std::vector<edge> the list of all edges
 */
std::vector<edge> EdgeKey::get_edges()
{
    std::vector<edge> edges;

    while (edge_cursor->next(edge_cursor) == 0)
    {
        edge found = {0};
        /////////////////////////////////////////////////////////////////////////
        // CommonUtil::get_key(edge_cursor, &found.src_id, &found.dst_id);
        CommonUtil::ekey_get_key(edge_cursor, &found.src_id, &found.dst_id);
        /////////////////////////////////////////////////////////////////////////
        if (found.dst_id != OutOfBand_ID)
        {
            if (opts.is_weighted)
            {
                CommonUtil::record_to_edge_ekey(edge_cursor, &found);
            }
            edges.push_back(found);
        }
    }
    edge_cursor->reset(edge_cursor);
    return edges;
}

/**
 * @brief Get all edges that have node_id
 *
 * @param node_id
 * @return std::vector<edge>
 */
std::vector<edge> EdgeKey::get_out_edges(node_id_t node_id)
{
    std::vector<edge> out_edges;
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
    CommonUtil::ekey_set_key(edge_cursor, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////

    int temp;
    if (edge_cursor->search(edge_cursor) == 0)
    {
        while (true)
        {
            edge found;
            if (edge_cursor->next(edge_cursor) != 0) break;
            /////////////////////////////////////////////////////////////////////////
            // CommonUtil::get_key(edge_cursor, &found.src_id, &found.dst_id);
            CommonUtil::ekey_get_key(edge_cursor, &found.src_id, &found.dst_id);
            // found.src_id -= 1;
            // found.dst_id -= 1;
            /////////////////////////////////////////////////////////////////////////

            if ((found.src_id == node_id) & (found.dst_id != OutOfBand_ID))
            {
                edge_cursor->get_value(edge_cursor, &found.edge_weight, &temp);
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
    edge_cursor->reset(edge_cursor);
    return out_edges;
}

/**
 * @brief Returns a list containing all nodes that have an incoming edge
 * from node_id
 *
 * @param node_id Node ID of the node who's out nodes are sought.
 * @return std::vector<node>
 */
std::vector<node> EdgeKey::get_out_nodes(node_id_t node_id)
{
    std::vector<node> out_nodes;
    WT_CURSOR *e_cur;
    if (_get_table_cursor(EDGE_TABLE, &e_cur, session, false, true) != 0)
    {
        throw GraphException("Could not get a cursor to the Edge table");
    }

    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(e_cur, MAKE_EKEY(node_id), OutOfBand_ID);
    CommonUtil::ekey_set_key(e_cur, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////

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
            //////////////////////////////////////////////////////////////////////
            // CommonUtil::get_key(e_cur, &src_id, &dst_id);
            CommonUtil::ekey_get_key(e_cur, &src_id, &dst_id);
            // if (src_id == MAKE_EKEY(node_id))
            if (src_id == node_id)
            //////////////////////////////////////////////////////////////////////
            {
                // found = get_node(OG_KEY(dst_id));
                found = get_node(dst_id);
                out_nodes.push_back(found);
                CommonUtil::dump_node(found);
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

/**
 * @brief Returns a list containing ids of all nodes that have an incoming
 * edge from node_id; uses one less cursor search than get_out_nodes
 *
 * @param node_id Node ID of the node who's out nodes are sought.
 * @return std::vector<node>
 */
std::vector<node_id_t> EdgeKey::get_out_nodes_id(node_id_t node_id)
{
    std::vector<node_id_t> out_nodes_id;
    WT_CURSOR *e_cur;
    if (_get_table_cursor(EDGE_TABLE, &e_cur, session, false, true) != 0)
    {
        throw GraphException("Could not get a cursor to the Edge table");
    }

    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(e_cur, MAKE_EKEY(node_id), OutOfBand_ID);
    CommonUtil::ekey_set_key(e_cur, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////

    if (e_cur->search(e_cur) == 0)
    {
        while (true)
        {
            node_id_t src_id, dst_id;
            if (e_cur->next(e_cur) != 0)
            {
                break;
            }
            /////////////////////////////////////////////////////////////////////////
            // CommonUtil::get_key(e_cur, &src_id, &dst_id);
            CommonUtil::ekey_get_key(e_cur, &src_id, &dst_id);
            // if (src_id == MAKE_EKEY(node_id))
            if (src_id == node_id)
            {
                out_nodes_id.push_back(dst_id);
                // out_nodes_id.push_back(OG_KEY(dst_id));
            }
            /////////////////////////////////////////////////////////////////////////

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
    return out_nodes_id;
}

/**
 * @brief Get a list of all edges that have node_id as the destination
 *
 * @param node_id the id of the node whose in_edges are sought
 * @return std::vector<edge>
 */
std::vector<edge> EdgeKey::get_in_edges(node_id_t node_id)
{
    std::vector<edge> in_edges;
    edge found;
    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(dst_src_idx_cursor, MAKE_EKEY(node_id),
    // OutOfBand_ID);
    CommonUtil::ekey_set_key(dst_src_idx_cursor, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////
    int search_exact;
    node_id_t src, dst;
    dst_src_idx_cursor->search_near(dst_src_idx_cursor, &search_exact);
    if (search_exact <= 0)
    {
        dst_src_idx_cursor->next(dst_src_idx_cursor);
        /////////////////////////////////////////////////////////////////////////
        // CommonUtil::get_key(dst_src_idx_cursor, &dst, &src);
        CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
        assert(dst == node_id);
        // assert(OG_KEY(dst) == node_id);
        assert(src > OutOfBand_ID);
        /////////////////////////////////////////////////////////////////////////

        // the cursor points to edges so the src value
        // here is definitely > out of band value
    }

    do
    {
        // CommonUtil::get_key(dst_src_idx_cursor, &dst, &src);
        CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
        // if (dst == MAKE_EKEY(node_id))
        if (dst == node_id)
        {
            if (src != OutOfBand_ID)
            {
                // found.dst_id = OG_KEY(dst);
                // found.src_id = OG_KEY(src);
                found.dst_id = dst;
                found.src_id = src;
                if (opts.is_weighted)
                {
                    int32_t tmp;
                    dst_src_idx_cursor->get_value(
                        dst_src_idx_cursor, &found.edge_weight, &tmp);
                }
                in_edges.push_back(found);
            }
        }
        else
        {
            break;
        }
    } while (dst_src_idx_cursor->next(dst_src_idx_cursor) == 0);

    dst_src_idx_cursor->reset(dst_src_idx_cursor);

    return in_edges;
}

/**
 * @brief Get a list of all nodes that have an outgoing edge to node_id
 *
 * @param node_id the node whose in_nodes are sought.
 * @return std::vector<node>
 */
// Should we delete the debug info here?
std::vector<node> EdgeKey::get_in_nodes(node_id_t node_id)
{
    std::vector<node> in_nodes;
    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }

    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(dst_src_idx_cursor, MAKE_EKEY(node_id),
    // OutOfBand_ID);
    CommonUtil::ekey_set_key(dst_src_idx_cursor, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////
    int search_exact;
    node_id_t src, dst;
    dst_src_idx_cursor->search_near(dst_src_idx_cursor, &search_exact);
    if (search_exact <= 0)
    {
        dst_src_idx_cursor->next(dst_src_idx_cursor);
        CommonUtil::get_key(dst_src_idx_cursor, &dst, &src);
        // CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
        assert(dst == node_id);
        // assert(OG_KEY(dst) == node_id);
        assert(src > OutOfBand_ID);
        // the cursor points to edges so the src value
        // here is definitely > out of band value
    }

    do
    {
        // CommonUtil::get_key(dst_src_idx_cursor, &dst, &src);
        CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
        //        if (dst == MAKE_EKEY(node_id))
        if (dst == node_id) in_nodes.push_back(get_node(src));
        // in_nodes.push_back(get_node(OG_KEY(src)));
        else
            break;
    } while (dst_src_idx_cursor->next(dst_src_idx_cursor) == 0);

    dst_src_idx_cursor->reset(dst_src_idx_cursor);
    return in_nodes;
}

/**
 * @brief Get a list of ids of all nodes that have an outgoing edge to
 * node_id; uses one less cursor search than get_in_nodes
 *
 * @param node_id the node whose in_nodes are sought.
 * @return std::vector<node>
 */
// Should we delete the debug info here?
std::vector<node_id_t> EdgeKey::get_in_nodes_id(node_id_t node_id)
{
    std::vector<node_id_t> in_nodes_id;
    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }

    /////////////////////////////////////////////////////////////////////////
    // CommonUtil::ekey_set_key(dst_src_idx_cursor, MAKE_EKEY(node_id),
    // OutOfBand_ID);
    CommonUtil::ekey_set_key(dst_src_idx_cursor, node_id, OutOfBand_ID);
    /////////////////////////////////////////////////////////////////////////
    int search_exact;
    node_id_t src, dst;
    dst_src_idx_cursor->search_near(dst_src_idx_cursor, &search_exact);
    if (search_exact <= 0)
    {
        dst_src_idx_cursor->next(dst_src_idx_cursor);
        // CommonUtil::get_key(dst_src_idx_cursor, &dst, &src);
        CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
        assert(dst == node_id);
        // assert(OG_KEY(dst) == node_id);
        assert(src > OutOfBand_ID);
        // the cursor points to edges so the src value
        // here is definitely > out of band value
    }

    do
    {
        // CommonUtil::get_key(dst_src_idx_cursor, &dst, &src);
        CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
        // if (dst == MAKE_EKEY(node_id))
        if (dst == node_id) in_nodes_id.push_back(src);
        // in_nodes_id.push_back(OG_KEY(src));
        else
            break;
    } while (dst_src_idx_cursor->next(dst_src_idx_cursor) == 0);

    dst_src_idx_cursor->reset(dst_src_idx_cursor);
    return in_nodes_id;
}

/**
 * @brief Drops the indices not required for adding edges.
 * Used for optimized bulk loading: User should call drop_indices() before
 * bulk loading nodes/edges. Once nodes/edges are added, user must
 * call create_indices() for the graph API to work.
 *
 * This method requires exclusive access to the specified data source(s). If
 * any cursors are open with the specified name(s) or a data source is
 * otherwise in use, the call will fail and return EBUSY.
 */
[[maybe_unused]] void EdgeKey::drop_indices()
{
    CommonUtil::close_cursor(edge_cursor);

    // drop SRC index
    string uri = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
    session->drop(session, uri.c_str(), nullptr);

    // drop the DST index
    uri = "index:" + EDGE_TABLE + ":" + DST_INDEX;
    session->drop(session, uri.c_str(), nullptr);
}

// Session/Connection/Cursor operations

/*
Get cursors
*/

[[maybe_unused]] WT_CURSOR *EdgeKey::get_metadata_cursor()
{
    if (metadata_cursor == nullptr)
    {
        int ret =
            _get_table_cursor(METADATA, &metadata_cursor, session, false, true);
        if (ret != 0)
        {
            throw GraphException("Could not get a metadata cursor");
        }
    }
    return metadata_cursor;
}

WT_CURSOR *EdgeKey::get_new_random_node_cursor()
{
    WT_CURSOR *random_node_cursor;

    if (_get_table_cursor(
            EDGE_TABLE, &random_node_cursor, session, true, true) != 0)
    {
        throw GraphException("Could not get a random cursor to the Edge table");
    }

    return random_node_cursor;
}

WT_CURSOR *EdgeKey::get_dst_src_idx_cursor()
{
    if (dst_src_idx_cursor == nullptr)
    {
        string projection = "(" + ATTR_FIRST + "," + ATTR_SECOND + ")";
        if (_get_index_cursor(
                EDGE_TABLE, DST_SRC_INDEX, projection, &dst_src_idx_cursor) !=
            0)
        {
            throw GraphException("Could not get a cursor to DST_SRC_INDEX");
        }
    }

    return dst_src_idx_cursor;
}

WT_CURSOR *EdgeKey::get_new_dst_src_idx_cursor()
{
    WT_CURSOR *new_dst_src_idx_cursor = nullptr;
    string projection = "(" + ATTR_FIRST + "," + ATTR_SECOND + ")";
    if (_get_index_cursor(
            EDGE_TABLE, DST_SRC_INDEX, projection, &new_dst_src_idx_cursor) !=
        0)
    {
        throw GraphException("Could not get a cursor to DST_SRC_INDEX");
    }

    return new_dst_src_idx_cursor;
}

OutCursor *EdgeKey::get_outnbd_iter()
{
    OutCursor *toReturn = new EkeyOutCursor(get_new_edge_cursor(), session);
    toReturn->set_key_range({OutOfBand_ID, OutOfBand_ID});
    return toReturn;
}

InCursor *EdgeKey::get_innbd_iter()
{
    //    uint64_t num_nodes = this->get_num_nodes();
    InCursor *toReturn =
        new EkeyInCursor(get_new_dst_src_idx_cursor(), session);
    //toReturn->set_key_range({0, 0});

    return toReturn;
}

NodeCursor *EdgeKey::get_node_iter()
{
    NodeCursor *toReturn =
        new EkeyNodeCursor(get_new_dst_src_idx_cursor(), session);
    toReturn->set_key_range({OutOfBand_ID, OutOfBand_ID});
    return toReturn;
}

EdgeCursor *EdgeKey::get_edge_iter()
{
    EdgeCursor *toReturn = new EkeyEdgeCursor(get_new_edge_cursor(), session);
    edge_range rng;
    toReturn->set_key_range(rng);
    return toReturn;
}

[[maybe_unused]] WT_CURSOR *EdgeKey::get_node_cursor()
{
    return get_dst_src_idx_cursor();
}

[[maybe_unused]] [[maybe_unused]] WT_CURSOR *EdgeKey::get_new_node_cursor()
{
    return get_new_dst_src_idx_cursor();
}

WT_CURSOR *EdgeKey::get_edge_cursor()
{
    if (edge_cursor == nullptr)
    {
        if (_get_table_cursor(EDGE_TABLE, &edge_cursor, session, false, true) !=
            0)
        {
            throw GraphException("Could not get a cursor to the Edge table");
        }
    }

    return edge_cursor;
}

WT_CURSOR *EdgeKey::get_new_edge_cursor()
{
    WT_CURSOR *new_edge_cursor = nullptr;
    if (_get_table_cursor(EDGE_TABLE, &new_edge_cursor, session, false, true) !=
        0)
    {
        throw GraphException("Could not get a cursor to the Edge table");
    }

    return new_edge_cursor;
}
