#include "edgekey.h"

#include <wiredtiger.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "common.h"

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

void EdgeKey::init_cursors()
{
    int ret =
        _get_table_cursor(METADATA, &metadata_cursor, session, false, true);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the metadata table:" +
                             string(wiredtiger_strerror(ret)));
    }
    ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, session, false, true);
    if (ret != 0)
    {
        throw GraphException("Could not get an edge cursor: " +
                             string(wiredtiger_strerror(ret)));
    }
    string projection = "(" + SRC + "," + DST + ")";
    ret = _get_index_cursor(EDGE_TABLE, SRC_INDEX, projection, &src_idx_cursor);
    ret = _get_index_cursor(EDGE_TABLE, DST_INDEX, projection, &dst_idx_cursor);
    projection =
        "(" + DST + "," + SRC + "," + ATTR_FIRST + "," + ATTR_SECOND + ")";
    ret = _get_index_cursor(
        EDGE_TABLE, DST_SRC_INDEX, projection, &dst_src_idx_cursor);
}
/**
 * @brief get the node identified by node_id
 *
 * @param node_id the Node ID
 * @return node the node struct containing the node
 */
node EdgeKey::get_node(node_id_t node_id)
{
    CommonUtil::set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
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
        ret = CommonUtil::get_key(random_cursor, &src, &dst);
        if (ret != 0)
        {
            break;
        }
        if (dst == OutOfBand_ID)
        {
            // random_cur->set_key(random_cur, src, dst);
            rando.id = OG_KEY(src);
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

    CommonUtil::set_key(edge_cursor, MAKE_EKEY(to_insert.id), OutOfBand_ID);
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
    CommonUtil::set_key(edge_cursor, MAKE_EKEY(to_insert.id), OutOfBand_ID);
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
    CommonUtil::set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
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
    CommonUtil::set_key(edge_cursor, MAKE_EKEY(src_id), MAKE_EKEY(dst_id));
    ret = edge_cursor->search(edge_cursor);
    edge_cursor->reset(edge_cursor);
    return (ret == 0);
}

int EdgeKey::delete_node(node_id_t node_id)  // TODO
{
    int num_nodes_to_add = 0;
    int num_edges_to_add = 0;
    int ret = 0;

    session->begin_transaction(session, "isolation=snapshot");

    CommonUtil::set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
    ret = edge_cursor->remove(edge_cursor);
    switch (ret)
    {
        case 0:
            num_nodes_to_add -= 1;
            break;
        case WT_ROLLBACK:
            session->rollback_transaction(session, nullptr);
            return WT_ROLLBACK;
            // goto start_delete_node;
        case WT_NOTFOUND:
            session->rollback_transaction(session, nullptr);
            return WT_NOTFOUND;
            // return;
        default:
            session->rollback_transaction(session, nullptr);
            throw GraphException(
                "Failed to remove node with id " + std::to_string(node_id) +
                " from the edge table" + wiredtiger_strerror(ret));
    }
    edge_cursor->reset(edge_cursor);

    ret = delete_related_edges(
        src_idx_cursor, edge_cursor, node_id, &num_edges_to_add);

    if (ret == 1)
    {
        return WT_ROLLBACK;
    }

    ret = delete_related_edges(
        dst_idx_cursor, edge_cursor, node_id, &num_edges_to_add);

    if (ret == 1)
    {
        return WT_ROLLBACK;
        // goto start_delete_node;
    }
    session->commit_transaction(session, nullptr);
    // add_to_nedges(num_edges_to_add);
    // add_to_nnodes(num_nodes_to_add);
    return 0;
}

// Caller should continue on return value = 0 and retry on return value = 1
int EdgeKey::delete_related_edges(WT_CURSOR *idx_cur,
                                  WT_CURSOR *e_cur,
                                  node_id_t node_id,
                                  int *num_edges_to_add)  // TODO
{
    int ret = 0;
    node_id_t src, dst;

    CommonUtil::set_key(idx_cur, MAKE_EKEY(node_id));
    if (idx_cur->search(idx_cur) != 0)
    {
        return 0;
    }

    do
    {
        CommonUtil::get_val_idx(idx_cur, &src, &dst);
        CommonUtil::set_key(e_cur, src, dst);

        ret = e_cur->remove(e_cur);
        switch (ret)
        {
            case 0:
                *num_edges_to_add -= 1;
                break;
            case WT_ROLLBACK:
                session->rollback_transaction(session, nullptr);
                return 1;
                break;
            case WT_NOTFOUND:
            // WT_NOTFOUND should not occur
            default:
                session->rollback_transaction(session, nullptr);
                throw GraphException("Failed to remove edge between " +
                                     to_string(src) + " and " + to_string(dst) +
                                     wiredtiger_strerror(ret));
        }
        // if readoptimize: Get the node to decrement in/out degree
        if (opts.read_optimize)
        {
            node temp;

            if (src != MAKE_EKEY(node_id))
            {
                temp = get_node(OG_KEY(src));
                temp.out_degree--;
            }
            else
            {
                temp = get_node(OG_KEY(dst));
                temp.in_degree--;
            }

            ret = update_node_degree(temp.id, temp.in_degree, temp.out_degree);
            switch (ret)
            {
                case 0:
                    break;
                case WT_NOTFOUND:
                case WT_ROLLBACK:
                    session->rollback_transaction(session, nullptr);
                    return 1;

                    // WT_NOTFOUND should not occur
                default:
                    session->rollback_transaction(session, nullptr);
                    throw GraphException("Could not update the node with ID" +
                                         std::to_string(temp.id) +
                                         wiredtiger_strerror(ret));
            }
        }

        CommonUtil::set_key(idx_cur, MAKE_EKEY(node_id));
    } while (idx_cur->search(idx_cur) == 0);
    idx_cur->reset(idx_cur);

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
    CommonUtil::set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
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
    int ret = 0;
    // start_add_edge:
    num_nodes_to_add = 0;
    num_edges_to_add = 0;
    ret = 0;
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

    CommonUtil::set_key(
        edge_cursor, MAKE_EKEY(to_insert.src_id), MAKE_EKEY(to_insert.dst_id));

    if (opts.is_weighted)
    {
        edge_cursor->set_value(
            edge_cursor, to_insert.edge_weight, OutOfBand_Val);
    }
    else
    {
        edge_cursor->set_value(edge_cursor, 0, OutOfBand_Val);
    }

    ret = edge_cursor->insert(edge_cursor);

    switch (ret)
    {
        case 0:
            num_edges_to_add += 1;
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

    // insert reverse edge if undirected
    if (!opts.is_directed)
    {
        CommonUtil::set_key(edge_cursor,
                            MAKE_EKEY(to_insert.dst_id),
                            MAKE_EKEY(to_insert.src_id));

        if (opts.is_weighted)
        {
            edge_cursor->set_value(
                edge_cursor, to_insert.edge_weight, OutOfBand_Val);
        }
        else
        {
            edge_cursor->set_value(edge_cursor, 0, OutOfBand_Val);
        }

        ret = edge_cursor->insert(edge_cursor);
        switch (ret)
        {
            case 0:
                num_edges_to_add += 1;
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
                throw GraphException(
                    "Failed to insert the reverse edge between " +
                    std::to_string(to_insert.src_id) + " and " +
                    std::to_string(to_insert.dst_id) +
                    wiredtiger_strerror(ret));
        }
    }

    if (!is_bulk)
    {
        // Update the in/out degrees if opts.read_optimize
        if (opts.read_optimize)
        {
            node src = {0};

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
                case WT_NOTFOUND:
                case WT_ROLLBACK:
                    session->rollback_transaction(session, nullptr);
                    return WT_ROLLBACK;
                    // goto start_add_edge;

                // WT_NOTFOUND should not occur
                default:
                    session->rollback_transaction(session, nullptr);
                    throw GraphException("Could not update the node with ID" +
                                         std::to_string(src.id) +
                                         wiredtiger_strerror(ret));
            }

            node dst = {0};

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
                case WT_NOTFOUND:
                case WT_ROLLBACK:
                    session->rollback_transaction(session, nullptr);
                    return WT_ROLLBACK;
                    // goto start_add_edge;
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
    int num_edges_to_add = 0;
    int ret = 0;
    // start_delete_edge:
    num_edges_to_add = 0;
    ret = 0;
    session->begin_transaction(session, "isolation=snapshot");

    CommonUtil::set_key(edge_cursor, MAKE_EKEY(src_id), MAKE_EKEY(dst_id));

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
        CommonUtil::set_key(edge_cursor, MAKE_EKEY(dst_id), MAKE_EKEY(src_id));

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
            case WT_NOTFOUND:
            case WT_ROLLBACK:
                session->rollback_transaction(session, nullptr);
                return WT_ROLLBACK;
                // goto start_delete_edge;

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
            case WT_NOTFOUND:
            case WT_ROLLBACK:
                session->rollback_transaction(session, nullptr);
                return WT_ROLLBACK;
                // goto start_delete_edge;

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
    edge found = {UINT64_MAX, -1, -1, -1};
    CommonUtil::set_key(edge_cursor, MAKE_EKEY(src_id), MAKE_EKEY(dst_id));
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
 * @return std::vector<node> the list containing all nodes.
 */
std::vector<node> EdgeKey::get_nodes()
{
    std::vector<node> nodes;

    CommonUtil::set_key(dst_idx_cursor, OutOfBand_ID);
    if (dst_idx_cursor->search(dst_idx_cursor) == 0)
    {
        node_id_t node_id, temp;
        CommonUtil::get_val_idx(dst_idx_cursor, &node_id, &temp);
        assert(temp == OutOfBand_ID);  // this should be true
        CommonUtil::set_key(edge_cursor, node_id, OutOfBand_ID);
        if (edge_cursor->search(edge_cursor) == 0)
        {
            node found{.id = OG_KEY(node_id)};
            CommonUtil::record_to_node_ekey(edge_cursor, &found);
            nodes.push_back(found);
        }

        while (dst_idx_cursor->next(dst_idx_cursor) == 0)
        {
            CommonUtil::get_val_idx(dst_idx_cursor, &node_id, &temp);
            if (temp == OutOfBand_ID)
            {
                CommonUtil::set_key(edge_cursor, node_id, OutOfBand_ID);
                if (edge_cursor->search(edge_cursor) == 0)
                {
                    node found{.id = OG_KEY(node_id)};
                    CommonUtil::record_to_node_ekey(edge_cursor, &found);
                    nodes.push_back(found);
                }
            }
            else
            {
                break;
            }
        }
    }
    dst_idx_cursor->reset(dst_idx_cursor);
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
    if (opts.read_optimize)
    {
        CommonUtil::set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
        node found = {0};
        if (edge_cursor->search(edge_cursor) == 0)
        {
            found.id = node_id;
            CommonUtil::record_to_node_ekey(edge_cursor, &found);
        }
        edge_cursor->reset(edge_cursor);
        return found.out_degree;
    }
    else
    {
        degree_t out_deg = 0;
        int ret = 0;
        CommonUtil::set_key(src_idx_cursor, MAKE_EKEY(node_id));
        if (src_idx_cursor->search(src_idx_cursor) == 0)
        {
            while (ret == 0)
            {
                node_id_t src, dst;
                CommonUtil::get_val_idx(src_idx_cursor, &src, &dst);
                if (src == MAKE_EKEY(node_id))
                {
                    if (dst != OutOfBand_ID)
                    {
                        out_deg++;
                    }
                }
                else
                {
                    break;
                }
                ret = src_idx_cursor->next(src_idx_cursor);
            }
        }
        src_idx_cursor->reset(src_idx_cursor);
        return out_deg;
    }
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
        CommonUtil::set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);
        edge_cursor->search(edge_cursor);
        node found = {0};
        CommonUtil::record_to_node_ekey(edge_cursor, &found);
        edge_cursor->reset(edge_cursor);
        return found.in_degree;
    }
    else
    {
        int ret = 0;
        degree_t in_degree = 0;
        CommonUtil::set_key(dst_idx_cursor, MAKE_EKEY(node_id));
        if (dst_idx_cursor->search(dst_idx_cursor) == 0)
        {
            while (ret == 0)
            {
                node_id_t src, dst;
                CommonUtil::get_val_idx(dst_idx_cursor, &src, &dst);
                if (src == MAKE_EKEY(node_id))
                {
                    if (dst != OutOfBand_ID)
                    {
                        in_degree++;
                    }
                }
                else
                {
                    break;
                }
                ret = dst_idx_cursor->next(dst_idx_cursor);
            }
        }
        dst_idx_cursor->reset(dst_idx_cursor);
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
        CommonUtil::get_key(edge_cursor, &found.src_id, &found.dst_id);
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
    CommonUtil::set_key(edge_cursor, MAKE_EKEY(node_id), OutOfBand_ID);

    int temp;
    if (edge_cursor->search(edge_cursor) == 0)
    {
        while (true)
        {
            edge found;
            if (edge_cursor->next(edge_cursor) != 0) break;
            CommonUtil::get_key(edge_cursor, &found.src_id, &found.dst_id);
            found.src_id -= 1;
            found.dst_id -= 1;
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

    CommonUtil::set_key(e_cur, MAKE_EKEY(node_id), OutOfBand_ID);

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
            CommonUtil::get_key(e_cur, &src_id, &dst_id);
            if (src_id == MAKE_EKEY(node_id))
            {
                found = get_node(OG_KEY(dst_id));
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

    CommonUtil::set_key(e_cur, MAKE_EKEY(node_id), OutOfBand_ID);

    if (e_cur->search(e_cur) == 0)
    {
        while (true)
        {
            node_id_t src_id, dst_id;
            if (e_cur->next(e_cur) != 0)
            {
                break;
            }
            CommonUtil::get_key(e_cur, &src_id, &dst_id);
            if (src_id == MAKE_EKEY(node_id))
            {
                out_nodes_id.push_back(OG_KEY(dst_id));
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
    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    CommonUtil::set_key(dst_idx_cursor, MAKE_EKEY(node_id));
    int search_ret = dst_idx_cursor->search(dst_idx_cursor);
    if (search_ret == 0)
    {
        node_id_t src_id, dst_id;
        CommonUtil::get_val_idx(dst_idx_cursor, &src_id, &dst_id);
        //        std::cout << "(src, dst) : " << src_id << " , " << dst_id
        //        <<std::endl;

        do
        {
            edge found;
            found.src_id = OG_KEY(src_id);
            found.dst_id = OG_KEY(dst_id);
            found.edge_weight = 0;
            if (opts.is_weighted)
            {
                CommonUtil::set_key(edge_cursor, src_id, dst_id);
                if (edge_cursor->search(edge_cursor) == 0)
                {
                    CommonUtil::record_to_edge_ekey(edge_cursor, &found);
                    in_edges.push_back(found);
                }
            }
            else
            {
                in_edges.push_back(found);
            }

            dst_idx_cursor->next(dst_idx_cursor);
            CommonUtil::get_val_idx(dst_idx_cursor, &src_id, &dst_id);
        } while (dst_id == MAKE_EKEY(node_id));
    }
    edge_cursor->reset(edge_cursor);
    dst_idx_cursor->reset(dst_idx_cursor);
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
    CommonUtil::set_key(dst_idx_cursor, MAKE_EKEY(node_id));
    int search_ret = dst_idx_cursor->search(dst_idx_cursor);
    if (search_ret == 0)
    {
        node_id_t src_id, dst_id;
        CommonUtil::get_val_idx(dst_idx_cursor, &src_id, &dst_id);
        do
        {
            in_nodes.push_back(get_node(OG_KEY(src_id)));
            if (dst_idx_cursor->next(dst_idx_cursor) == 0)
            {
                CommonUtil::get_val_idx(dst_idx_cursor, &src_id, &dst_id);
            }
            else
            {
                break;
            }

        } while (dst_id == MAKE_EKEY(node_id));
    }
    edge_cursor->reset(edge_cursor);
    dst_idx_cursor->reset(dst_idx_cursor);
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
    CommonUtil::set_key(dst_idx_cursor, MAKE_EKEY(node_id));
    if (dst_idx_cursor->search(dst_idx_cursor) == 0)
    {
        node_id_t src_id, dst_id;
        CommonUtil::get_val_idx(dst_idx_cursor, &src_id, &dst_id);
        while (dst_id == MAKE_EKEY(node_id))
        {
            in_nodes_id.push_back(OG_KEY(src_id));
            if (dst_idx_cursor->next(dst_idx_cursor) != 0)
            {
                break;
            }
            CommonUtil::get_val_idx(dst_idx_cursor, &src_id, &dst_id);
        }
    }
    dst_idx_cursor->reset(dst_idx_cursor);
    return in_nodes_id;
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
    // Index on SRC column of edge table
    string edge_table_idx, edge_table_idx_conf;
    edge_table_idx = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
    edge_table_idx_conf = "columns=(" + SRC + ")";
    if (sess->create(
            sess, edge_table_idx.c_str(), edge_table_idx_conf.c_str()) != 0)
    {
        throw GraphException("Failed to create SRC_INDEX on the edge table");
    }

    // Index on the DST column of the edge table
    edge_table_idx = "index:" + EDGE_TABLE + ":" + DST_INDEX;
    edge_table_idx_conf = "columns=(" + DST + ")";
    if (sess->create(
            sess, edge_table_idx.c_str(), edge_table_idx_conf.c_str()) != 0)
    {
        throw GraphException(
            "Failed to create an index on the DST column of the edge "
            "table");
    }

    // Index on (DST,SRC) columns of the edge table
    // Used for adjacency neighbourhood iterators
    edge_table_idx = "index:" + EDGE_TABLE + ":" + DST_SRC_INDEX;
    edge_table_idx_conf = "columns=(" + DST + "," + SRC + ")";
    if (sess->create(
            sess, edge_table_idx.c_str(), edge_table_idx_conf.c_str()) != 0)
    {
        throw GraphException(
            "Failed to create DST_SRC_INDEX on the edge table");
    }
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
    CommonUtil::close_cursor(src_idx_cursor);
    CommonUtil::close_cursor(dst_idx_cursor);

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

WT_CURSOR *EdgeKey::get_metadata_cursor()
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
WT_CURSOR *EdgeKey::get_src_idx_cursor()
{
    if (src_idx_cursor == nullptr)
    {
        string projection = "(" + SRC + "," + DST + ")";
        if (_get_index_cursor(
                EDGE_TABLE, SRC_INDEX, projection, &src_idx_cursor) != 0)
        {
            throw GraphException("Could not get a cursor to the SRC_INDEX ");
        }
    }

    return src_idx_cursor;
}

WT_CURSOR *EdgeKey::get_new_src_idx_cursor()
{
    WT_CURSOR *new_src_idx_cursor = nullptr;
    string projection = "(" + SRC + "," + DST + ")";
    if (_get_index_cursor(
            EDGE_TABLE, SRC_INDEX, projection, &new_src_idx_cursor) != 0)
    {
        throw GraphException("Could not get a cursor to the SRC_INDEX ");
    }

    return new_src_idx_cursor;
}

WT_CURSOR *EdgeKey::get_dst_idx_cursor()
{
    if (dst_idx_cursor == nullptr)
    {
        string projection = "(" + SRC + "," + DST + ")";
        if (_get_index_cursor(
                EDGE_TABLE, DST_INDEX, projection, &dst_idx_cursor) != 0)
        {
            throw GraphException("Could not get a cursor to DST_INDEX");
        }
    }

    return dst_idx_cursor;
}

WT_CURSOR *EdgeKey::get_new_dst_idx_cursor()
{
    WT_CURSOR *new_dst_idx_cursor = nullptr;
    string projection = "(" + SRC + "," + DST + ")";
    if (_get_index_cursor(
            EDGE_TABLE, DST_INDEX, projection, &new_dst_idx_cursor) != 0)
    {
        throw GraphException("Could not get a cursor to the DST_INDEX ");
    }

    return new_dst_idx_cursor;
}

WT_CURSOR *EdgeKey::get_dst_src_idx_cursor()
{
    if (dst_src_idx_cursor == nullptr)
    {
        string projection =
            "(" + DST + "," + SRC + "," + ATTR_FIRST + "," + ATTR_SECOND + ")";
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
    string projection =
        "(" + DST + "," + SRC + "," + ATTR_FIRST + "," + ATTR_SECOND + ")";
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
    uint64_t num_nodes = this->get_num_nodes();
    OutCursor *toReturn = new EkeyOutCursor(get_new_src_idx_cursor(), session);
    toReturn->set_num_nodes(num_nodes);
    toReturn->set_key_range(
        {node{0}, node{static_cast<node_id_t>((int64_t)num_nodes)}});
    return toReturn;
}

InCursor *EdgeKey::get_innbd_iter()
{
    uint64_t num_nodes = this->get_num_nodes();
    InCursor *toReturn = new EkeyInCursor(get_new_dst_idx_cursor(), session);
    toReturn->set_num_nodes(num_nodes);
    toReturn->set_key_range(
        {node{0}, node{static_cast<node_id_t>((int64_t)num_nodes)}});

    return toReturn;
}

NodeCursor *EdgeKey::get_node_iter()
{
    return new EkeyNodeCursor(get_new_dst_src_idx_cursor(), session);
}

EdgeCursor *EdgeKey::get_edge_iter()
{
    return new EkeyEdgeCursor(get_new_edge_cursor(), session, opts.is_weighted);
}

WT_CURSOR *EdgeKey::get_node_cursor() { return get_dst_src_idx_cursor(); }

WT_CURSOR *EdgeKey::get_new_node_cursor() { return get_new_dst_idx_cursor(); }

node EdgeKey::get_next_node(WT_CURSOR *dst_cur)
{
    CommonUtil::set_key(dst_cur, OutOfBand_ID);
    WT_CURSOR *e_cur = get_edge_cursor();
    node found = {-1};
    if (dst_cur->search(dst_cur) == 0)
    {
        node_id_t node_id, temp;
        dst_cur->get_value(dst_cur, &node_id, &temp);
        assert(temp == OutOfBand_ID);  // this should be true
        CommonUtil::set_key(e_cur, MAKE_EKEY(node_id), OutOfBand_ID);
        if (e_cur->search(e_cur) == 0)
        {
            found.id = node_id;
            CommonUtil::record_to_node_ekey(e_cur, &found);
        }
    }
    return found;
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

edge EdgeKey::get_next_edge(WT_CURSOR *e_cur)
{
    edge found = {UINT64_MAX, -1, -1, -1};
    while (e_cur->next(e_cur) == 0)
    {
        CommonUtil::get_key(e_cur, &found.src_id, &found.dst_id);
        if (found.dst_id != OutOfBand_ID)
        {
            if (opts.is_weighted)
            {
                CommonUtil::record_to_edge_ekey(e_cur, &found);
            }
            break;
        }
    }
    return found;
}

void EdgeKey::close_all_cursors()
{
    edge_cursor->close(edge_cursor);
    metadata_cursor->close(metadata_cursor);
    src_idx_cursor->close(src_idx_cursor);
    dst_idx_cursor->close(dst_idx_cursor);
}
