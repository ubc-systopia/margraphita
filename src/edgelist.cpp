#include "edgelist.h"

#include <wiredtiger.h>

#include <cstring>
#include <string>

#include "common.h"

using namespace std;

[[maybe_unused]] const std::string GRAPH_PREFIX = "uo_elist";

UnOrderedEdgeList::UnOrderedEdgeList(graph_opts &opt_params,
                                     WT_CONNECTION *conn,
                                     GraphEngine *graph_engine)
    : GraphBase(opt_params, conn)

{
    init_cursors();
    this->graph_engine = graph_engine;
    this->insert_range = graph_engine->request_insert_range();
    this->insert_edge_id = insert_range.first;
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
    // CommonUtil::set_key(node_cursor, node_id);
    // int ret = node_cursor->search(node_cursor);
    // node_cursor->reset(node_cursor);
    // return (ret == 0);
    return false;
}
void UnOrderedEdgeList::create_wt_tables(graph_opts &opts, WT_CONNECTION *conn)
{
    // ******** Now set up the Node Table     **************
    WT_SESSION *sess;
    if (CommonUtil::open_session(conn, &sess) != 0)
    {
        throw GraphException("Cannot open session");
    }

    // ******** Now set up the Edge Table     **************
    // Edge Column Format : <src><dst><weight>
    // Now prepare the edge value format. starts with II for src,dst. Add
    // another I if weighted
    vector<string> edge_columns = {ID, SRC, DST};
    string edge_key_format =
        "QII";  // id : uint32_t, big_endian using __builtin_bswap32
    string edge_value_format = "";
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
    // edge_cursor initialization
    ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, session, false, false);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the edge table:" +
                             string(wiredtiger_strerror(ret)));
    }
    string projection = "(" + ID + ")";
    ret = _get_index_cursor(
        EDGE_TABLE, SRC_DST_INDEX, projection, &src_dst_index_cursor);
    if (ret != 0)
    {
        // throw GraphException(
        //     "Could not get an src_dst index cursor to the edge table:" +
        //     string(wiredtiger_strerror(ret)));
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
int UnOrderedEdgeList::add_node(node to_insert) { return -1; }

int UnOrderedEdgeList::add_node_txn(node to_insert) { return -1; }

int UnOrderedEdgeList::delete_edge(node_id_t src_id, node_id_t dst_id)
{
    int ret = 0;
    session->begin_transaction(session, "isolation=snapshot");
    src_dst_index_cursor->set_key(src_dst_index_cursor, src_id, dst_id);
    ret = src_dst_index_cursor->search(src_dst_index_cursor);
    switch (ret)
    {
        case 0:
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
    uint64_t id;
    src_dst_index_cursor->get_value(src_dst_index_cursor, &id);
    edge_cursor->set_key(edge_cursor, id, src_id, dst_id);
    ret = edge_cursor->remove(edge_cursor);

    switch (ret)
    {
        case 0:
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

    src_dst_index_cursor->reset(src_dst_index_cursor);
    edge_cursor->reset(edge_cursor);
    session->commit_transaction(session, nullptr);
    return ret;
}

/**
 * @brief Adds an edge to the graph; will attempt to add nodes associated
 * with the edge
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
    if (insert_edge_id > insert_range.second - 2)
    {
        insert_range = graph_engine->request_insert_range();
        insert_edge_id = insert_range.first;
    }
    session->begin_transaction(session, "isolation=snapshot");
    if (!is_bulk_insert)
    {
        edge_cursor->set_key(
            edge_cursor, insert_edge_id, to_insert.src_id, to_insert.dst_id);

        if (opts.is_weighted)
        {
            edge_cursor->set_value(edge_cursor, to_insert.edge_weight);
        }
        else
        {
            edge_cursor->set_value(edge_cursor, 0);
        }

        int ret = error_check(edge_cursor->insert(edge_cursor), SRC_LOC);
        if (!ret)
        {
            insert_edge_id += 1;
        }
        else
        {
            CommonUtil::log_msg("Failed to insert edge between " +
                                std::to_string(to_insert.src_id) + " and " +
                                std::to_string(to_insert.dst_id) +
                                wiredtiger_strerror(ret));
            return ret;
        }

        if (!opts.is_directed)
        {
            edge_cursor->set_key(edge_cursor,
                                 insert_edge_id,
                                 to_insert.dst_id,
                                 to_insert.src_id);

            if (opts.is_weighted)
            {
                edge_cursor->set_key(edge_cursor, to_insert.edge_weight);
            }
            else
            {
                edge_cursor->set_value(edge_cursor, 0);
            }

            ret = error_check(edge_cursor->insert(edge_cursor), SRC_LOC);
            if (!ret)
            {
                insert_edge_id += 1;
            }
            else
            {
                CommonUtil::log_msg("Failed to insert (reverse) edge between " +
                                    std::to_string(to_insert.dst_id) + " and " +
                                    std::to_string(to_insert.src_id) +
                                    wiredtiger_strerror(ret));
                return ret;
            }
        }
    }

    session->commit_transaction(session, NULL);
    return 0;
}

int UnOrderedEdgeList::update_node_degree(WT_CURSOR *cursor,
                                          node_id_t node_id,
                                          degree_t in_degree,
                                          degree_t out_degree)
{
    return -1;
}

node UnOrderedEdgeList::get_random_node() { return node{-1}; }

degree_t UnOrderedEdgeList::get_in_degree(node_id_t node_id) { return 0; }

degree_t UnOrderedEdgeList::get_out_degree(node_id_t node_id) { return 0; }

/**
 * @brief Get all nodes in the graph
 *
 * @return std::vector<node> vector of all nodes
 */
std::vector<node> UnOrderedEdgeList::get_nodes()
{
    std::vector<node> nodelist;
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
        edge found = {UINT64_MAX, -1, -1, -1};
        WT_ITEM id;
        edge_cursor->get_key(
            edge_cursor, &found.id, &found.src_id, &found.dst_id);
        edge_cursor->get_value(edge_cursor, &found.edge_weight);
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
    edge found = {UINT64_MAX, -1, -1, -1};
    src_dst_index_cursor->set_key(src_dst_index_cursor, src_id, dst_id);
    int ret = src_dst_index_cursor->search(src_dst_index_cursor);
    if (ret == 0)
    {
        src_dst_index_cursor->get_value(src_dst_index_cursor, &found.id);
        found.src_id = src_id;
        found.dst_id = dst_id;
        // src_dst_index_cursor->get_value(src_dst_index_cursor,
        // &found.edge_weight);
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
    src_dst_index_cursor->set_key(src_dst_index_cursor, src_id, dst_id);
    int ret = src_dst_index_cursor->search(src_dst_index_cursor);
    src_dst_index_cursor->reset(src_dst_index_cursor);
    return (ret == 0);
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
                // session->rollback_transaction(session,
                // nullptr);
                /*Rolling back the transaction
                 * here is not necessary if the
                 * cursor has been opened with
                 * overwrite=false. A plain
                 * warning is enough. */
                CommonUtil::log_msg(
                    "WT_DUPLICATE_KEY error in "
                    "insert_txn",
                    SRC_LOC);
            }
            return WT_DUPLICATE_KEY;
        case WT_NOTFOUND:
            //! Should we roll back the
            //! transaction here? This should
            //! not happen.
            CommonUtil::log_msg(
                "WT_NOTFOUND error in "
                "insert_txn",
                SRC_LOC);
            return WT_NOTFOUND;
        default:
            session->rollback_transaction(session, nullptr);
            throw GraphException(
                "Failed to complete insert "
                "action" +
                std::string(wiredtiger_strerror(return_val)));
    }
}

int UnOrderedEdgeList::error_check_update_txn(int return_val)
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
            throw GraphException(
                "Failed to complete update "
                "action" +
                std::string(wiredtiger_strerror(return_val)));
    }
}

int UnOrderedEdgeList::error_check_read_txn(int return_val)
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
            throw GraphException(
                "Failed to complete read "
                "action" +
                std::string(wiredtiger_strerror(return_val)));
    }
}

int UnOrderedEdgeList::error_check_remove_txn(int return_val)
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
    // Index on (DST,SRC) columns of the edge table
    std::string edge_table_idx = "index:" + EDGE_TABLE + ":" + SRC_DST_INDEX;
    std::string edge_table_idx_conf = "columns=(" + SRC + "," + DST + ")";
    if (sess->create(
            sess, edge_table_idx.c_str(), edge_table_idx_conf.c_str()) != 0)
    {
        throw GraphException(
            "Failed to create DST_SRC_INDEX on "
            "the edge table");
    }
}

void UnOrderedEdgeList::drop_indices() {}

int UnOrderedEdgeList::error_check(int ret, std::source_location loc)
{
    switch (ret)
    {
        case 0:
            return 0;
        case WT_DUPLICATE_KEY:
        case WT_ROLLBACK:
        default:
            session->rollback_transaction(session, NULL);
            CommonUtil::log_msg("Error code( " + to_string(ret) +
                                    ") :" + wiredtiger_strerror(ret),
                                loc);
    }
    return ret;
}

std::vector<edge> UnOrderedEdgeList::get_out_edges(node_id_t node_id)
{
    vector<edge> edges(0);
    return edges;
}
std::vector<node> UnOrderedEdgeList::get_out_nodes(node_id_t node_id)
{
    vector<node> nodes(0);
    return nodes;
}
std::vector<node_id_t> UnOrderedEdgeList::get_out_nodes_id(node_id_t node_id)
{
    vector<node_id_t> node_ids(0);
    return node_ids;
}
std::vector<edge> UnOrderedEdgeList::get_in_edges(node_id_t node_id)
{
    vector<edge> edges(0);
    return edges;
}
std::vector<node> UnOrderedEdgeList::get_in_nodes(node_id_t node_id)
{
    vector<node> nodes(0);
    return nodes;
}
std::vector<node_id_t> UnOrderedEdgeList::get_in_nodes_id(node_id_t node_id)
{
    vector<node_id_t> nodes(0);
    return nodes;
}

WT_CURSOR *UnOrderedEdgeList::get_node_cursor() { return nullptr; }

WT_CURSOR *UnOrderedEdgeList::get_new_node_cursor() { return nullptr; }

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

WT_CURSOR *UnOrderedEdgeList::get_src_idx_cursor() { return nullptr; }

WT_CURSOR *UnOrderedEdgeList::get_new_src_idx_cursor() { return nullptr; }

WT_CURSOR *UnOrderedEdgeList::get_dst_idx_cursor() { return nullptr; }

WT_CURSOR *UnOrderedEdgeList::get_new_dst_idx_cursor() { return nullptr; }

WT_CURSOR *UnOrderedEdgeList::get_src_dst_idx_cursor()
{
    if (src_dst_index_cursor == nullptr)
    {
        string projection = "(" + ID + ")";
        if (_get_index_cursor(EDGE_TABLE,
                              SRC_DST_INDEX,
                              projection,
                              &(src_dst_index_cursor)) != 0)
        {
            throw GraphException(
                "Could not get a SRC_DST index "
                "cursor on the edge table");
        }
    }
    return src_dst_index_cursor;
}

WT_CURSOR *UnOrderedEdgeList::get_new_src_dst_idx_cursor()
{
    WT_CURSOR *new_src_dst_index_cursor = nullptr;
    string projection = "(" + ID + ")";
    if (_get_index_cursor(
            EDGE_TABLE, SRC_DST_INDEX, projection, &new_src_dst_index_cursor) !=
        0)
    {
        throw GraphException(
            "Could not get a SRC_DST index "
            "cursor on the edge table");
    }

    return new_src_dst_index_cursor;
}

OutCursor *UnOrderedEdgeList::get_outnbd_iter() { return nullptr; }

InCursor *UnOrderedEdgeList::get_innbd_iter() { return nullptr; }

NodeCursor *UnOrderedEdgeList::get_node_iter() { return nullptr; }

EdgeCursor *UnOrderedEdgeList::get_edge_iter()
{
    return new UOEdgeListEdgeCursor(get_new_edge_cursor(), session);
}