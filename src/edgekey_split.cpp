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

    //    // Set up the node table
    //    // columns : ID, ATTR_FIRST, ATTR_SECOND
    //    edge_columns.clear();
    //    edge_columns = {ID, ATTR_FIRST, ATTR_SECOND};
    //    CommonUtil::set_table(sess, NODE_TABLE, edge_columns, "u", "II");

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

    //    // nodes
    //    ret = _get_table_cursor(NODE_TABLE, &node_cursor, session, false,
    //    true); if (ret != 0)
    //    {
    //        throw GraphException("Could not get a node cursor: " +
    //                             string(wiredtiger_strerror(ret)));
    //    }

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
}
int SplitEdgeKey::add_node(node to_insert)
{
    session->begin_transaction(session, "isolation=snapshot");
    CommonUtil::ekey_set_key(out_edge_cursor, to_insert.id, OutOfBand_ID);
    CommonUtil::ekey_set_key(in_edge_cursor, to_insert.id, OutOfBand_ID);
    if (opts.read_optimize)
    {
        out_edge_cursor->set_value(
            out_edge_cursor, to_insert.in_degree, to_insert.out_degree);
        in_edge_cursor->set_value(
            in_edge_cursor, to_insert.in_degree, to_insert.out_degree);
    }
    else
    {
        out_edge_cursor->set_value(out_edge_cursor, 0, 0);
        in_edge_cursor->set_value(in_edge_cursor, 0, 0);
    }
    auto in_ret =
        error_check_insert_txn(in_edge_cursor->insert(in_edge_cursor));
    auto out_ret =
        error_check_insert_txn(out_edge_cursor->insert(out_edge_cursor));
    if (in_ret == 0 & out_ret == 0)
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

bool SplitEdgeKey::has_node(node_id_t node_id)
{
    int ret;
    CommonUtil::ekey_set_key(out_edge_cursor, node_id, OutOfBand_ID);
    ret = out_edge_cursor->search(out_edge_cursor);
    out_edge_cursor->reset(out_edge_cursor);
    return (ret == 0);
}

int SplitEdgeKey::add_edge(edge to_insert, bool is_bulk) { return 0; }
edge SplitEdgeKey::get_edge(node_id_t src_id, node_id_t dst_id)
{
    return edge();
}
std::vector<node> SplitEdgeKey::get_nodes() { return std::vector<node>(); }
std::vector<edge> SplitEdgeKey::get_edges() { return std::vector<edge>(); }

bool SplitEdgeKey::has_edge(node_id_t src_id, node_id_t dst_id)
{
    int ret;

    CommonUtil::ekey_set_key(out_edge_cursor, src_id, dst_id);
    ret = out_edge_cursor->search(out_edge_cursor);
    out_edge_cursor->reset(out_edge_cursor);
    return (ret == 0);
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

    // UPDATE IN_EDGE TABLE
    CommonUtil::ekey_set_key(in_edge_cursor, to_insert.id, OutOfBand_ID);
    if (opts.read_optimize)
    {
        in_edge_cursor->set_value(
            in_edge_cursor, to_insert.in_degree, to_insert.out_degree);
    }
    else
    {
        in_edge_cursor->set_value(in_edge_cursor, 0, OutOfBand_Val);
    }
    return out_edge_cursor->insert(
        out_edge_cursor);  // no need to check -- done in caller.
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
degree_t SplitEdgeKey::get_in_degree(node_id_t node_id) { return 0; }
degree_t SplitEdgeKey::get_out_degree(node_id_t node_id) { return 0; }
std::vector<edge> SplitEdgeKey::get_out_edges(node_id_t node_id)
{
    return std::vector<edge>();
}
std::vector<node> SplitEdgeKey::get_out_nodes(node_id_t node_id)
{
    return std::vector<node>();
}
std::vector<node_id_t> SplitEdgeKey::get_out_nodes_id(node_id_t node_id)
{
    return std::vector<node_id_t>();
}
std::vector<edge> SplitEdgeKey::get_in_edges(node_id_t node_id)
{
    return std::vector<edge>();
}
std::vector<node_id_t> SplitEdgeKey::get_in_nodes_id(node_id_t node_id)
{
    return std::vector<node_id_t>();
}
std::vector<node> SplitEdgeKey::get_in_nodes(node_id_t node_id)
{
    return std::vector<node>();
}
int SplitEdgeKey::update_node_degree(node_id_t node_id,
                                     degree_t in_degree,
                                     degree_t out_degree)
{
    return 0;
}
OutCursor *SplitEdgeKey::get_outnbd_iter() { return nullptr; }
InCursor *SplitEdgeKey::get_innbd_iter() { return nullptr; }
NodeCursor *SplitEdgeKey::get_node_iter() { return nullptr; }
EdgeCursor *SplitEdgeKey::get_edge_iter() { return nullptr; }
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

    session->commit_transaction(session, nullptr);
    if (ret != 0) return ret;

    GraphBase::increment_edges(num_edges_to_add);
    GraphBase::increment_nodes(-1);
    return 0;
}
WT_CURSOR *SplitEdgeKey::get_metadata_cursor() { return nullptr; }
int SplitEdgeKey::delete_node_and_related_edges(node_id_t node_id,
                                                int *num_edges_to_del)
{
    return 0;
}

int SplitEdgeKey::add_edge_only(edge to_insert) { return 0; }
int SplitEdgeKey::error_check_add_edge(int ret) { return 0; }
int SplitEdgeKey::delete_edge(node_id_t src_id, node_id_t dst_id) { return 0; }
