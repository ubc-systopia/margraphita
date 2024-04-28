#include "edgekey_split.h"

#include <wiredtiger.h>

#include <string>

#include "common_util.h"

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
    // ******** Now set up the Node Table     **************
    WT_SESSION *sess;
    if (CommonUtil::open_session(conn, &sess) != 0)
    {
        throw GraphException("Cannot open session");
    }
    // Set up the out-edge table
    // columns: SRC, DST, ATTR_FIRST, ATTR_SECOND
    vector<string> edge_columns = {SRC, DST, ATTR_FIRST, ATTR_SECOND};
    string edge_key_format = "uu";  // SRC DST
    string edge_value_format =
        "II";  // in/out degree(unit32_t) or weight/uninterpreted
    CommonUtil::set_table(
        sess, OUT_EDGES, edge_columns, edge_key_format, edge_value_format);

    edge_columns.clear();
    // Set up the out-edge table
    // columns: SRC, DST, ATTR_FIRST, ATTR_SECOND
    edge_columns = {DST, SRC, ATTR_FIRST, ATTR_SECOND};
    CommonUtil::set_table(
        sess, IN_EDGES, edge_columns, edge_key_format, edge_value_format);

    if (!opts.optimize_create)
    {
        // Index on (DST,SRC) columns of the edge table
        // Used for adjacency neighbourhood iterators
        std::string idx_name = "index:" + OUT_EDGES + ":" + DST_SRC_INDEX;
        std::string idx_conf = "columns=(" + DST + "," + SRC + ")";
        if (sess->create(sess, idx_name.c_str(), idx_conf.c_str()) != 0)
        {
            throw GraphException(
                "Failed to create DST_SRC_INDEX on the out-edge table");
        }
    }

    sess->close(sess, nullptr);
}

void SplitEdgeKey::init_cursors()
{
    // metadata_cursor initialization
    int ret =
        _get_table_cursor(METADATA, &metadata_cursor, session, false, false);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the metadata table:" +
                             string(wiredtiger_strerror(ret)));
    }

    // out_edge_cursor
    ret = _get_table_cursor(OUT_EDGES, &out_edge_cursor, session, false, true);
    if (ret != 0)
    {
        throw GraphException("Could not get an out edge cursor: " +
                             string(wiredtiger_strerror(ret)));
    }

    // in_edge_cursor
    ret = _get_table_cursor(IN_EDGES, &in_edge_cursor, session, false, true);
    if (ret != 0)
    {
        throw GraphException("Could not get an in edge cursor: " +
                             string(wiredtiger_strerror(ret)));
    }
    // dst_src index
    std::string projection = "(" + ATTR_FIRST + "," + ATTR_SECOND + ")";
    ret = _get_index_cursor(
        OUT_EDGES, DST_SRC_INDEX, projection, &dst_src_idx_cursor);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the dst_src index" +
                             string(wiredtiger_strerror(ret)));
    }
}
int SplitEdgeKey::add_node(node to_insert)
{
    session->begin_transaction(session, "isolation=snapshot");
    CommonUtil::ekey_set_key(out_edge_cursor, to_insert.id, OutOfBand_ID);
    // CommonUtil::ekey_set_key(in_edge_cursor, to_insert.id, OutOfBand_ID);
    if (opts.read_optimize)
    {
        out_edge_cursor->set_value(
            out_edge_cursor, to_insert.in_degree, to_insert.out_degree);
        //        in_edge_cursor->set_value(
        //            in_edge_cursor, to_insert.in_degree,
        //            to_insert.out_degree);
    }
    else
    {
        out_edge_cursor->set_value(out_edge_cursor, 0, 0);
        //        in_edge_cursor->set_value(in_edge_cursor, 0, 0);
    }
    //    auto in_ret =
    //        error_check_insert_txn(in_edge_cursor->insert(in_edge_cursor));
    auto out_ret =
        error_check_insert_txn(out_edge_cursor->insert(out_edge_cursor));
    if (out_ret == 0)  // & in_ret == 0)
    {
        session->commit_transaction(session, nullptr);
        GraphBase::increment_nodes(1);
        return 0;
    }
    else
    {
        // FIXME: THIS NEEDS TO BE CHANGED ASAP
        throw GraphException("Failed to insert a node with ID " +
                             std::to_string(to_insert.id) +
                             " into the edge table");
    }
}

int SplitEdgeKey::add_node_txn(node to_insert)
{
    CommonUtil::ekey_set_key(out_edge_cursor, to_insert.id, OutOfBand_ID);
    if (opts.read_optimize)
    {
        out_edge_cursor->set_value(
            out_edge_cursor, to_insert.in_degree, to_insert.out_degree);
    }
    else
    {
        out_edge_cursor->set_value(out_edge_cursor, 0, OutOfBand_Val);
    }
    int ret = out_edge_cursor->insert(out_edge_cursor);
    if (ret != 0)
    {
        return ret;
    }

    //    // UPDATE IN_EDGE TABLE
    //    CommonUtil::ekey_set_key(in_edge_cursor, to_insert.id, OutOfBand_ID);
    //    if (opts.read_optimize)
    //    {
    //        in_edge_cursor->set_value(
    //            in_edge_cursor, to_insert.in_degree, to_insert.out_degree);
    //    }
    //    else
    //    {
    //        in_edge_cursor->set_value(in_edge_cursor, 0, OutOfBand_Val);
    //    }
    //    return in_edge_cursor->insert(
    //        in_edge_cursor);  // no need to check -- done in caller.
}

bool SplitEdgeKey::has_node(node_id_t node_id)
{
    int ret;
    CommonUtil::ekey_set_key(out_edge_cursor, node_id, OutOfBand_ID);
    ret = out_edge_cursor->search(out_edge_cursor);
    out_edge_cursor->reset(out_edge_cursor);
    return (ret == 0);
}

int SplitEdgeKey::add_edge(edge to_insert, bool is_bulk)
{
    int num_nodes_to_add = 0;
    int num_edges_to_add = 0;
    int ret;
    session->begin_transaction(session, "isolation=snapshot");
    if (!has_node(to_insert.src_id))
    {
        node src;
        // create with to-be out degree
        if (opts.is_directed)
            src = {to_insert.src_id, 0, 1};
        else
            src = {to_insert.src_id, 1, 1};
        ret = error_check_insert_txn(add_node_txn(src));
        if (ret != 0)
        {
            return ret;
        }
        else
        {
            num_nodes_to_add++;
        }
    }
    else  // update the node degree
    {
        ret =
            error_check_insert_txn(update_node_degree(to_insert.src_id, 0, 1));
        if (ret != 0)
        {
            return ret;  // the session has been already rolled back.
        }
    }
    if (!has_node(to_insert.dst_id))
    {
        node dst;
        // create with to-be in degree
        if (opts.is_directed)
            dst = {to_insert.dst_id, 1, 0};
        else
            dst = {to_insert.dst_id, 1, 1};
        ret = error_check_insert_txn(add_node_txn(dst));
        if (ret != 0)
        {
            return ret;
        }
        else
        {
            num_nodes_to_add++;
        }
    }
    else
    {
        ret =
            error_check_insert_txn(update_node_degree(to_insert.dst_id, 1, 0));
        if (ret != 0)
        {
            return ret;  // the session has been already rolled back.
        }
    }
    if (add_edge_only(to_insert) == 0)
    {
        num_edges_to_add++;
    }
    else
    {
        throw GraphException("Failed to insert edge between " +
                             std::to_string(to_insert.src_id) + " and " +
                             std::to_string(to_insert.dst_id) +
                             ". Session rolled back.");
    }
    session->commit_transaction(session, nullptr);
    GraphBase::increment_nodes(num_nodes_to_add);
    GraphBase::increment_edges(num_edges_to_add);
    return 0;
}

edge SplitEdgeKey::get_edge(node_id_t src_id, node_id_t dst_id)
{
    edge found = {};

    CommonUtil::ekey_set_key(out_edge_cursor, src_id, dst_id);
    if (out_edge_cursor->search(out_edge_cursor) == 0)
    {
        found.src_id = src_id;
        found.dst_id = dst_id;
        if (opts.is_weighted)
            CommonUtil::record_to_edge_ekey(out_edge_cursor, &found);
    }
    return found;
}

std::vector<node> SplitEdgeKey::get_nodes()
{
    std::vector<node> nodes;

    int search_exact;
    CommonUtil::ekey_set_key(dst_src_idx_cursor, OutOfBand_ID, OutOfBand_ID);
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
        CommonUtil::ekey_get_key(dst_src_idx_cursor, &dst, &src);
        if (dst != OutOfBand_ID) break;
        n.id = src;
        dst_src_idx_cursor->get_value(
            dst_src_idx_cursor, &n.in_degree, &n.out_degree);
        nodes.push_back(n);
    } while (dst_src_idx_cursor->next(dst_src_idx_cursor) == 0);

    dst_src_idx_cursor->reset(dst_src_idx_cursor);
    return nodes;
}

std::vector<edge> SplitEdgeKey::get_edges()
{
    std::vector<edge> edges;

    while (out_edge_cursor->next(out_edge_cursor) == 0)
    {
        edge found = {0};
        CommonUtil::ekey_get_key(out_edge_cursor, &found.src_id, &found.dst_id);
        if (found.dst_id != OutOfBand_ID)
        {
            if (opts.is_weighted)
            {
                CommonUtil::record_to_edge_ekey(out_edge_cursor, &found);
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

    CommonUtil::ekey_set_key(out_edge_cursor, src_id, dst_id);
    ret = out_edge_cursor->search(out_edge_cursor);
    out_edge_cursor->reset(out_edge_cursor);
    return (ret == 0);
}

node SplitEdgeKey::get_node(node_id_t node_id)
{
    CommonUtil::ekey_set_key(out_edge_cursor, node_id, OutOfBand_ID);
    node found = {0};
    if (out_edge_cursor->search(out_edge_cursor) == 0)
    {
        found.id = node_id;
        CommonUtil::record_to_node_ekey(out_edge_cursor, &found);
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
        ret = _get_table_cursor(
            OUT_EDGES, &random_node_cursor, session, true, true);
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
        if (dst == OutOfBand_ID)
        {
            rando.id = src;
            CommonUtil::record_to_node_ekey(random_node_cursor, &rando);
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
    if (opts.read_optimize)
    {
        CommonUtil::ekey_set_key(out_edge_cursor, node_id, OutOfBand_ID);
        if (out_edge_cursor->search(out_edge_cursor) != 0)
        {
            out_edge_cursor->reset(out_edge_cursor);
            throw GraphException("Node with ID " + std::to_string(node_id) +
                                 " does not exist");
        }
        node found = {0};
        CommonUtil::record_to_node_ekey(out_edge_cursor, &found);
        out_edge_cursor->reset(out_edge_cursor);
        return found.in_degree;
    }
    else
    {
        CommonUtil::ekey_set_key(in_edge_cursor, node_id, OutOfBand_ID);
        int search_exact;  // this can not be 0 because we are not creating
                           // node_id, 0 entires for nodes in this table.
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
                return 0;  // no in-edges for this node.
            }
        }
        if (search_exact == 0)
        {
            in_edge_cursor->next(
                in_edge_cursor);  // to position to the next edge
        }
        // Now the case where search_exact >0
        do
        {
            node_id_t src, dst;
            CommonUtil::ekey_get_key(in_edge_cursor, &dst, &src);
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
    CommonUtil::ekey_set_key(out_edge_cursor, node_id, OutOfBand_ID);
    int ret = out_edge_cursor->search(out_edge_cursor);
    if (ret == 0)  // The node exists
    {
        if (opts.read_optimize)
        {
            node found = {0};
            found.id = node_id;
            CommonUtil::record_to_node_ekey(out_edge_cursor, &found);
            out_edge_cursor->reset(out_edge_cursor);
            out_deg = found.out_degree;
        }
        else
        {
            node_id_t src, dst;
            do
            {
                out_edge_cursor->next(
                    out_edge_cursor);  // skip to the out_edges
                CommonUtil::ekey_get_key(out_edge_cursor, &src, &dst);
                if (src == node_id)
                {
                    out_deg++;
                }
                else
                {
                    break;
                }
            } while (src == node_id && dst != OutOfBand_ID);
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
    CommonUtil::ekey_set_key(out_edge_cursor, node_id, OutOfBand_ID);

    int temp;
    if (out_edge_cursor->search(out_edge_cursor) == 0)
    {
        while (true)
        {
            edge found;
            if (out_edge_cursor->next(out_edge_cursor) != 0) break;
            CommonUtil::ekey_get_key(
                out_edge_cursor, &found.src_id, &found.dst_id);
            if ((found.src_id == node_id) & (found.dst_id != OutOfBand_ID))
            {
                out_edge_cursor->get_value(
                    out_edge_cursor, &found.edge_weight, &temp);
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
    if (_get_table_cursor(OUT_EDGES, &e_cur, session, false, true) != 0)
    {
        throw GraphException("Could not get a cursor to the OutEdge table");
    }

    CommonUtil::ekey_set_key(e_cur, node_id, OutOfBand_ID);

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
    WT_CURSOR *e_cur;
    if (_get_table_cursor(OUT_EDGES, &e_cur, session, false, true) != 0)
    {
        throw GraphException("Could not get a cursor to the OutEdge table");
    }
    CommonUtil::ekey_set_key(e_cur, node_id, OutOfBand_ID);
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
        throw GraphException("The node " + to_string(node_id) +
                             " does not exist in the graph");
    }
    e_cur->close(e_cur);
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
    CommonUtil::ekey_set_key(in_edge_cursor, node_id, OutOfBand_ID);

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
        if ((found.dst_id == node_id) & (found.src_id != OutOfBand_ID))
        {
            in_edge_cursor->get_value(
                in_edge_cursor, &found.edge_weight, &temp);
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
    if (_get_table_cursor(IN_EDGES, &in_cur, session, false, true) != 0)
    {
        throw GraphException("Could not get a cursor to the OutEdge table");
    }
    int search_exact;
    CommonUtil::ekey_set_key(in_cur, node_id, OutOfBand_ID);
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
        std::cout << "src_id: " << src_id << " dst_id: " << dst_id << std::endl;
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
    WT_CURSOR *in_cur;
    if (_get_table_cursor(IN_EDGES, &in_cur, session, false, true) != 0)
    {
        throw GraphException("Could not get a cursor to the OutEdge table");
    }
    int search_exact;
    CommonUtil::ekey_set_key(in_cur, node_id, OutOfBand_ID);
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

    in_cur->close(in_cur);
    return in_nodes_id;
}
/**
 * @brief This function accepts a node_id and two integers, in_change and
 * out_change and updates the in and out degree of the node in the in_Edge and
 * out_Edge tables. This function must be called within a running transaction,
 * and does not commit the transaction.
 * @param node_id the node to be updated in both in and out edge tables
 * @param in_change the +/- value of the change in the node's in-degree
 * @param out_change the +/- value of the change in the node's out-degree
 * @return 1 on error, 0 on success
 */
int SplitEdgeKey::update_node_degree(node_id_t node_id,
                                     int in_change,
                                     int out_change)
{
    // remember to do both the in and out edge tables
    // no transaction needed
    degree_t in, out;
    //    WT_CURSOR *in_cursor = get_new_in_cursor();
    WT_CURSOR *out_cursor = get_new_out_cursor();
    CommonUtil::ekey_set_key(out_cursor, node_id, OutOfBand_ID);
    if (out_cursor->search(out_cursor) == 0)
    {
        out_cursor->get_value(out_cursor, &in, &out);
        out_cursor->set_value(out_cursor, in + in_change, out + out_change);
        out_cursor->update(out_cursor);
    }
    else
    {
        return 1;
    }
    out_cursor->close(out_cursor);
    return 0;
}
OutCursor *SplitEdgeKey::get_outnbd_iter()
{
    OutCursor *toReturn = new SplitEKeyOutCursor(get_new_out_cursor(), session);
    return toReturn;
}
InCursor *SplitEdgeKey::get_innbd_iter()
{
    InCursor *toReturn = new SplitEkeyInCursor(get_new_in_cursor(), session);

    return toReturn;
}
NodeCursor *SplitEdgeKey::get_node_iter()
{
    NodeCursor *toReturn =
        new SplitEKeyNodeCursor(get_new_node_index_cursor(), session);
    return toReturn;
}
EdgeCursor *SplitEdgeKey::get_edge_iter()
{
    EdgeCursor *toReturn =
        new SplitEKeyEdgeCursor(get_new_out_cursor(), session);
    return toReturn;
}
void SplitEdgeKey::get_random_node_ids(vector<node_id_t> &randoms,
                                       int num_nodes)
{
    int count = 0, waste = 0;
    int ret = 0;
    if (random_node_cursor == nullptr)
    {
        ret = _get_table_cursor(
            EDGE_TABLE, &random_node_cursor, session, true, true);
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
        if (dst == OutOfBand_ID)
        {
            std::cout << "Random starting vertex: " << src << std::endl;
            random_node_cursor->get_value(
                random_node_cursor, &in_deg, &out_deg);
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
    std::cout << "wasted iterations: " << waste << std::endl;
    std::cout << "ids size: " << randoms.size() << std::endl;
}
int SplitEdgeKey::error_check_insert_txn(int return_val)
{
    switch (return_val)
    {
        case 0:
            return 0;
        case WT_ROLLBACK:
            session->rollback_transaction(session, nullptr);
            return WT_ROLLBACK;
        case WT_DUPLICATE_KEY:
            session->rollback_transaction(session, nullptr);
            return WT_DUPLICATE_KEY;
        default:
            session->rollback_transaction(session, nullptr);
            return GRAPH_API_PANIC;
    }
}

node_id_t SplitEdgeKey::get_min_node_id() { return 0; }
node_id_t SplitEdgeKey::get_max_node_id() { return 0; }

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
WT_CURSOR *SplitEdgeKey::get_metadata_cursor() { return nullptr; }
int SplitEdgeKey::delete_node_and_related_edges(node_id_t node_id,
                                                int *num_edges_to_add)
{
    int ret;
    node_id_t src, dst;

    CommonUtil::ekey_set_key(out_edge_cursor, node_id, OutOfBand_ID);
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
        std::cout << "@877 src: " << src << " dst: " << dst << std::endl;
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

        // We have successfully deleted the edge
        *num_edges_to_add -= 1;  // we have effectively only removed one edge
        ret = update_node_degree(
            dst, -1, 0);  // dst node's indegree in out_edge table
        if (ret != 0)
        {
            session->rollback_transaction(session, nullptr);
            return ret;
        }
    }
    out_edge_cursor->reset(out_edge_cursor);

    // we now do the same for in-edges table
    CommonUtil::ekey_set_key(in_edge_cursor, node_id, OutOfBand_ID);
    if (in_edge_cursor->search(in_edge_cursor) == 0)
    {
        ret = in_edge_cursor->remove(in_edge_cursor);
        if (ret != 0)
        {
            session->rollback_transaction(session, nullptr);
            return ret;
        }
    }

    while (in_edge_cursor->next(in_edge_cursor) == 0)
    {
        CommonUtil::ekey_get_key(in_edge_cursor, &dst, &src);
        std::cout << "@917 dst: " << dst << " src: " << src << std::endl;
        if (dst != node_id)
        {
            break;
        }
        // Delete the edge OUTGOING FROM the deleted node
        ret = in_edge_cursor->remove(in_edge_cursor);
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
    }
    in_edge_cursor->reset(in_edge_cursor);

    // We now need to remove the src->node_id edges from the out_edge table,
    // using an index
    WT_CURSOR *idx_cursor = get_node_index_cursor();
    CommonUtil::ekey_set_key(idx_cursor, node_id, OutOfBand_ID);
    int search_near;
    idx_cursor->search_near(idx_cursor, &search_near);
    if (search_near < 0)
    {
        idx_cursor->next(idx_cursor);
    }
    do
    {
        CommonUtil::ekey_get_key(idx_cursor, &dst, &src);
        if (dst != node_id)
        {
            break;
        }
        // Delete the edge OUTGOING FROM the deleted node
        CommonUtil::ekey_set_key(out_edge_cursor, src, node_id);
        ret = out_edge_cursor->remove(out_edge_cursor);
        if (ret != 0)
        {
            session->rollback_transaction(session, nullptr);
            return ret;  // panic
        }
    } while (idx_cursor->next(idx_cursor) == 0);

    return 0;
}

int SplitEdgeKey::add_edge_only(edge to_insert)
{
    // insert into out-edges table
    CommonUtil::ekey_set_key(
        out_edge_cursor, to_insert.src_id, to_insert.dst_id);
    if (opts.is_weighted)
    {
        out_edge_cursor->set_value(
            out_edge_cursor, to_insert.edge_weight, OutOfBand_Val);
    }
    else
    {
        out_edge_cursor->set_value(out_edge_cursor, 0, OutOfBand_Val);
    }

    int ret = error_check_insert_txn(out_edge_cursor->insert(out_edge_cursor));
    if (ret != 0)
        return ret;  // the session has been rolled back already. The final
                     // commit on success must be called by caller function

    // Insert into the in-edges table
    CommonUtil::ekey_set_key(
        in_edge_cursor, to_insert.dst_id, to_insert.src_id);
    if (opts.is_weighted)
    {
        in_edge_cursor->set_value(
            in_edge_cursor, to_insert.edge_weight, OutOfBand_Val);
    }
    else
    {
        in_edge_cursor->set_value(in_edge_cursor, 0, OutOfBand_Val);
    }
    return error_check_insert_txn(in_edge_cursor->insert(in_edge_cursor));
}
int SplitEdgeKey::error_check_add_edge(int ret) { return 0; }
int SplitEdgeKey::delete_edge(node_id_t src_id, node_id_t dst_id) { return 0; }

WT_CURSOR *SplitEdgeKey::get_new_out_cursor()
{
    WT_CURSOR *new_out_cursor = nullptr;
    if (_get_table_cursor(OUT_EDGES, &new_out_cursor, session, false, true) !=
        0)
    {
        throw GraphException("Could not get a cursor to the OutEdge table");
    }
    return new_out_cursor;
}
WT_CURSOR *SplitEdgeKey::get_new_in_cursor()
{
    WT_CURSOR *new_in_cursor = nullptr;
    if (_get_table_cursor(IN_EDGES, &new_in_cursor, session, false, true) != 0)
    {
        throw GraphException("Could not get a cursor to the In-Edge table");
    }

    return new_in_cursor;
}

WT_CURSOR *SplitEdgeKey::get_new_node_index_cursor()
{
    WT_CURSOR *new_dst_src_idx_cursor = nullptr;
    string projection = "(" + ATTR_FIRST + "," + ATTR_SECOND + ")";
    if (_get_index_cursor(
            OUT_EDGES, DST_SRC_INDEX, projection, &new_dst_src_idx_cursor) != 0)
    {
        throw GraphException("Could not get a cursor to DST_SRC_INDEX");
    }

    return new_dst_src_idx_cursor;
}
