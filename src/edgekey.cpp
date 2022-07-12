#include "edgekey.h"

#include <wiredtiger.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "common.h"

using namespace std;
const std::string GRAPH_PREFIX = "edgekey";

EdgeKey::EdgeKey(graph_opts opt_params) : GraphBase(opt_params)
{
    if (!CommonUtil::check_dir_exists(opts.stat_log))
    {
        std::filesystem::create_directories(opts.stat_log);
    }
    if (opts.create_new)
    {
        create_new_graph();
        if (opt_params.optimize_create == false)
        {
            create_indices(session);
        }
    }
    else
    {
        string dirname = opts.db_dir + "/" + opts.db_name;
        if (CommonUtil::check_dir_exists(dirname))
        {
            __restore_from_db(dirname);
        }
        else
        {
            throw GraphException(
                "Could not find the expected WT DB directory - " + opts.db_dir +
                "/" + opts.db_name);
        }
    }
}

EdgeKey::EdgeKey(graph_opts &opt_params, WT_CONNECTION *conn)
    : GraphBase(opt_params, conn)
{
    if (_get_table_cursor(METADATA, &metadata_cursor, session, false) != 0)
    {
        fprintf(stderr, "Failed to create cursor to the metadata table.");
        exit(-1);
    }
}

/**
 * @brief Create a new graph object
 * The edge table has : <src> <dst> <values>
 * if this is a node: <node_id> <-1> <in_degree, out_degree>
 * if this is an edge : <src><dst><edge_weight>
 */
void EdgeKey::create_new_graph()
{
    int ret = 0;
    // Create new directory for WT DB
    std::string dirname = opts.db_dir + "/" + opts.db_name;
    CommonUtil::create_dir(dirname);

    // open connection to WT DB
    ret = CommonUtil::open_connection(const_cast<char *>(dirname.c_str()),
                                      opts.stat_log,
                                      opts.conn_config,
                                      &connection);
    if (ret != 0)
    {
        throw GraphException("Could not open a connection to the DB" +
                             string(wiredtiger_strerror(ret)));
        exit(-1);
    }
    if (CommonUtil::open_session(connection, &session) != 0)
    {
        exit(-1);
    }
    // Set up the edge table
    // Edge Columns: <src> <dst> <weight/in_degree> <out_degree>
    // Edge Column Format: iiS

    CommonUtil::set_table(
        session, EDGE_TABLE, edge_columns, edge_key_format, edge_value_format);

    // Set up the Metadata table
    // This does not change across implementations.
    // value_format:string (S)
    // key_format: string (S)
    string metadata_table_name = "table:" + string(METADATA);
    if ((ret = session->create(session,
                               metadata_table_name.c_str(),
                               "key_format=S,value_format=S")) > 0)
    {
        fprintf(stderr, "Failed to create the metadata table ");
    }

    if ((ret = _get_table_cursor(METADATA, &metadata_cursor, session, false)) !=
        0)
    {
        fprintf(stderr, "Failed to create cursor to the metadata table.");
        exit(-1);
    }

    if ((ret = _get_table_cursor(
             METADATA, &this->metadata_cursor, session, false)) != 0)
    {
        fprintf(stderr, "Failed to create cursor to the metadata table.");
        exit(-1);
    }

    // opts.db_name
    insert_metadata(opts.db_name,
                    const_cast<char *>(opts.db_name.c_str()),
                    this->metadata_cursor);

    // opts.db_dir
    insert_metadata(opts.db_dir,
                    const_cast<char *>(opts.db_dir.c_str()),
                    this->metadata_cursor);

    // opts.read_optimize
    string read_optimized_str = opts.read_optimize ? "true" : "false";
    insert_metadata(READ_OPTIMIZE,
                    const_cast<char *>(read_optimized_str.c_str()),
                    this->metadata_cursor);

    // opts.is_directed
    string is_directed_str = opts.is_directed ? "true" : "false";
    insert_metadata(IS_DIRECTED,
                    const_cast<char *>(is_directed_str.c_str()),
                    this->metadata_cursor);

    // opts.is_weighted
    string is_weighted_str = opts.is_weighted ? "true" : "false";
    insert_metadata(IS_WEIGHTED,
                    const_cast<char *>(is_weighted_str.c_str()),
                    this->metadata_cursor);

    // NUM_NODES = 0
    insert_metadata(node_count,
                    const_cast<char *>(std::to_string(0).c_str()),
                    this->metadata_cursor);

    // NUM_EDGES = 0
    insert_metadata(edge_count,
                    const_cast<char *>(std::to_string(0).c_str()),
                    this->metadata_cursor);
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
    vector<string> edge_columns = {SRC, DST, ATTR};
    string edge_key_format = "qq";   // SRC DST
    string edge_value_format = "S";  // Packed binary
    CommonUtil::set_table(
        sess, EDGE_TABLE, edge_columns, edge_key_format, edge_value_format);
    if (opts.optimize_create == false)
    {
        create_indices(sess);
    }
    sess->close(sess, NULL);
}

/**
 * @brief get the node identified by node_id
 *
 * @param node_id the Node ID
 * @return node the node struct containing the node
 */
node EdgeKey::get_node(node_id_t node_id)
{
    WT_CURSOR *cursor = get_edge_cursor();
    cursor->set_key(cursor, node_id, OutOfBand_ID);
    node found = {0};
    if (cursor->search(cursor) == 0)
    {
        found.id = node_id;
        CommonUtil::__record_to_node_ekey(cursor, &found);
    }
    return found;
}

/**
 * @brief Returns a random node entry in the edge table. A random cursor to the
 * edge table will return a random record which could be an edge or a node.
 * since the number of nodes is much smaller than the number of edges, it is
 * unlikely that calling next() on the cursor will yield a node. It is much more
 * efficient to return the node record of the src node of the edge we encounter.
 *
 * @return node random node we find
 */
node EdgeKey::get_random_node()
{
    node rando = {0};
    WT_CURSOR *random_cur;
    int ret = _get_table_cursor(EDGE_TABLE, &random_cur, session, true);
    if (ret != 0)
    {
        throw GraphException("could not get a random cursor to the node table");
    }
    node_id_t src, dst;
    ret = random_cur->next(random_cur);
    if (ret != 0)
    {
        throw GraphException("could not get a random cursor on the edge table");
    }
    do
    {
        ret = random_cur->get_key(random_cur, &src, &dst);
        if (ret != 0)
        {
            throw GraphException("here");
        }
        if (dst == OutOfBand_ID)
        {
            // random_cur->set_key(random_cur, src, dst);
            CommonUtil::__record_to_node_ekey(random_cur, &rando);
            rando.id = src;
            break;
        }
        else
        {
            ret = random_cur->next(random_cur);
            if (ret != 0)
            {
                throw GraphException("here here");
            }
            ret = random_cur->get_key(random_cur, &src, &dst);
            if (ret != 0)
            {
                throw GraphException("hereherehere");
            }
            if (dst == OutOfBand_ID)
            {
                // random_cur->set_key(random_cur, src, dst);
                CommonUtil::__record_to_node_ekey(random_cur, &rando);
                rando.id = src;
                break;
            }
        }
    } while (dst != OutOfBand_ID);
    return rando;
}

/**
 * @brief This function adds a node to the edge table.
 *
 * @param to_insert the node to be inserted into the edge table
 */
void EdgeKey::add_node(node to_insert)
{
    int ret = 0;
    session->begin_transaction(session, "isolation=snapshot");
    if (has_node(to_insert.id))
    {
        session->rollback_transaction(session, NULL);
        return;
    }

    WT_CURSOR *edge_cursor = get_edge_cursor();

    edge_cursor->set_key(edge_cursor, to_insert.id, OutOfBand_ID);
    if (opts.read_optimize)
    {
        string packed = CommonUtil::pack_int_to_str(to_insert.in_degree,
                                                    to_insert.out_degree);
        edge_cursor->set_value(edge_cursor, packed.c_str());
    }
    else
    {
        edge_cursor->set_value(edge_cursor, "");
    }

    switch (edge_cursor->insert(edge_cursor))
    {
        case 0:
            session->commit_transaction(session, NULL);
            add_to_nnodes(1);
            break;

        case WT_ROLLBACK:
            session->rollback_transaction(session, NULL);
            break;

        default:
            session->rollback_transaction(session, NULL);
            throw GraphException("Failed to insert a node with ID " +
                                 std::to_string(to_insert.id) +
                                 " into the edge table");
    }
}

/**
 * @brief returns true if a node with id node_id exists in the edge table
 *
 * @param node_id the ID to look for
 * @return true / false depending on if node_id is found or not
 */
bool EdgeKey::has_node(node_id_t node_id)
{
    WT_CURSOR *e_cur = get_edge_cursor();
    e_cur->set_key(e_cur, node_id, OutOfBand_ID);
    int ret = e_cur->search(e_cur);
    e_cur->reset(e_cur);
    if (ret == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief returns true if there is an edge found between src_id and dst_id nodes
 *
 * @param src_id source node for the edge to search
 * @param dst_id destinatino node for the edge to search
 * @return true/false id edge was found or not
 */
bool EdgeKey::has_edge(node_id_t src_id, node_id_t dst_id)
{
    bool found = false;
    WT_CURSOR *e_cur = get_edge_cursor();
    e_cur->set_key(e_cur, src_id, dst_id);
    if (e_cur->search(e_cur) == 0)
    {
        found = true;
    }
    e_cur->reset(e_cur);
    return found;
}

void EdgeKey::delete_node(node_id_t node_id)  // TODO
{
    WT_CURSOR *e_cur = get_edge_cursor();
    WT_CURSOR *src_cur = get_src_idx_cursor();
    WT_CURSOR *dst_cur = get_dst_idx_cursor();

    if (_get_table_cursor(EDGE_TABLE, &e_cur, session, false) != 0)
    {
        throw GraphException("Failed to get a cursro to the edge table");
    }
    e_cur->set_key(e_cur, node_id, OutOfBand_ID);
    if (e_cur->remove(e_cur) != 0)
    {
        throw GraphException("Failed to remove node with id " +
                             std::to_string(node_id) + " from the edge table");
    }
    delete_related_edges(src_cur, e_cur, node_id);
    delete_related_edges(dst_cur, e_cur, node_id);
    if (locks != nullptr)
    {
        omp_set_lock(locks->get_node_num_lock());
        set_num_nodes(get_num_nodes() - 1, this->metadata_cursor);
        omp_unset_lock(locks->get_node_num_lock());
    }
    else
    {
        set_num_nodes(get_num_nodes() - 1, this->metadata_cursor);
    }
}

void EdgeKey::delete_related_edges(WT_CURSOR *idx_cur,
                                   WT_CURSOR *e_cur,
                                   node_id_t node_id)  // TODO
{
    int ret = 0;
    e_cur->reset(e_cur);

    idx_cur->set_key(idx_cur, node_id);
    if (idx_cur->search(idx_cur) == 0)
    {
        node_id_t src, dst;
        idx_cur->get_value(idx_cur, &src, &dst);
        e_cur->set_key(e_cur, src, dst);
        ret = e_cur->remove(e_cur);
        if (ret != 0)
        {
            throw GraphException("Failed to remove edge between " +
                                 to_string(src) + " and " + to_string(dst));
        }
        // if readoptimize: Get the node to decrement in/out degree
        if (opts.read_optimize)
        {
            node temp;
            if (locks != nullptr)
            {
                omp_set_lock(locks->get_node_degree_lock());
            }
            if (src != node_id)
            {
                temp = get_node(src);
                temp.out_degree--;
                update_node_degree(temp.id, temp.in_degree, temp.out_degree);
            }
            else
            {
                temp = get_node(dst);
                temp.in_degree--;
                update_node_degree(temp.id, temp.in_degree, temp.out_degree);
            }
            if (locks != nullptr)
            {
                omp_unset_lock(locks->get_node_degree_lock());
            }
        }
        while (idx_cur->next(idx_cur) == 0 &&
               idx_cur->get_key(idx_cur) == node_id)
        {
            node_id_t src, dst;
            idx_cur->get_value(idx_cur, &src, &dst);
            e_cur->set_key(e_cur, src, dst);
            ret = e_cur->remove(e_cur);
            if (ret != 0)
            {
                throw GraphException("Failed to remove edge between " +
                                     to_string(src) + " and " + to_string(dst));
            }
            if (opts.read_optimize)
            {
                node temp;
                if (locks != nullptr)
                {
                    omp_set_lock(locks->get_node_degree_lock());
                }
                if (src != node_id)
                {
                    temp = get_node(src);
                    temp.out_degree--;
                    update_node_degree(
                        temp.id, temp.in_degree, temp.out_degree);
                }
                else
                {
                    temp = get_node(dst);
                    temp.in_degree--;
                    update_node_degree(
                        temp.id, temp.in_degree, temp.out_degree);
                }
                if (locks != nullptr)
                {
                    omp_unset_lock(locks->get_node_degree_lock());
                }
            }
        }
    }
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
    if (opts.read_optimize)
    {
        WT_CURSOR *e_cur = get_edge_cursor();

        e_cur->set_key(e_cur, node_id, OutOfBand_ID);
        string val = CommonUtil::pack_int_to_str(indeg, outdeg);
        e_cur->set_value(e_cur, val.c_str());
        return e_cur->insert(e_cur);
    }
}

/**
 * @brief This function adds the edge passed as param
 *
 * @param to_insert the edge struct containing info about the edge to insert.
 */
void EdgeKey::add_edge(edge to_insert, bool is_bulk)
{
    if (!is_bulk)
    {
        // Checks for existence and concurrency control within add_node(-)
        node src{.id = to_insert.src_id};
        add_node(src);

        node dst{.id = to_insert.dst_id};
        add_node(dst);
    }

start:
    // Insert the edge
    session->begin_transaction(session, "isolation=snapshot");
    WT_CURSOR *e_cur = get_edge_cursor();
    e_cur->set_key(e_cur, to_insert.src_id, to_insert.dst_id);

    if (opts.is_weighted)
    {
        e_cur->set_value(e_cur, std::to_string(to_insert.edge_weight).c_str());
    }
    else
    {
        e_cur->set_value(e_cur, "");
    }

    switch (e_cur->insert(e_cur))
    {
        case 0:
            session->commit_transaction(session, NULL);
            break;

        case WT_ROLLBACK:
            session->rollback_transaction(session, NULL);
            goto start;
            break;

        default:
            session->rollback_transaction(session, NULL);

            throw GraphException("Failed to insert edge between " +
                                 std::to_string(to_insert.src_id) + " and " +
                                 std::to_string(to_insert.dst_id));
    }

    // insert reverse edge if undirected
    if (!opts.is_directed)
    {
    start_rev:
        session->begin_transaction(session, "isolation=snapshot");
        e_cur->set_key(e_cur, to_insert.dst_id, to_insert.src_id);
        if (opts.is_weighted)
        {
            e_cur->set_value(e_cur,
                             std::to_string(to_insert.edge_weight).c_str());
        }
        else
        {
            e_cur->set_value(e_cur, "");
        }

        switch (e_cur->insert(e_cur))
        {
            case 0:
                session->commit_transaction(session, NULL);
                break;

            case WT_ROLLBACK:
                session->rollback_transaction(session, NULL);
                goto start_rev;
                break;

            default:
                session->rollback_transaction(session, NULL);

                throw GraphException(
                    "Failed to insert the reverse edge between " +
                    std::to_string(to_insert.src_id) + " and " +
                    std::to_string(to_insert.dst_id));
        }
    }
    if (!opts.is_directed)
    {
        add_to_nedges(2);
    }
    else
    {
        add_to_nedges(1);
    }

    if (!is_bulk)
    {
        // Update the in/out degrees if opts.read_optimize
        if (opts.read_optimize)
        {
        retry_src:
            session->begin_transaction(session, "isolation=snapshot");

            node src = get_node(to_insert.src_id);
            src.out_degree++;
            if (!opts.is_directed)
            {
                src.in_degree++;
            }

            switch (update_node_degree(src.id, src.in_degree, src.out_degree))
            {
                case 0:
                    session->commit_transaction(session, NULL);
                    break;
                case WT_ROLLBACK:
                    session->rollback_transaction(session, NULL);
                    goto retry_src;
                    break;
                default:
                    session->rollback_transaction(session, NULL);
                    throw GraphException("Could not update the node with ID" +
                                         std::to_string(src.id));
            }

        retry_dst:
            session->begin_transaction(session, "isolation=snapshot");

            node dst = get_node(to_insert.dst_id);
            dst.in_degree++;
            if (!opts.is_directed)
            {
                dst.out_degree++;
            }

            switch (update_node_degree(dst.id, dst.in_degree, dst.out_degree))
            {
                case 0:
                    session->commit_transaction(session, NULL);
                    break;
                case WT_ROLLBACK:
                    session->rollback_transaction(session, NULL);
                    goto retry_dst;
                    break;
                default:
                    session->rollback_transaction(session, NULL);
                    throw GraphException("Could not update the node with ID" +
                                         std::to_string(dst.id));
            }
        }
    }
}

/**
 * @brief Delete the edge identified by (src_id, dst_id)
 *
 * @param src_id Source node ID for the edge to be deleted
 * @param dst_id Dst node ID for the edge to be deleted
 */
void EdgeKey::delete_edge(node_id_t src_id, node_id_t dst_id)  // TODO
{
    // delete edge
    WT_CURSOR *e_cur = get_edge_cursor();
    e_cur->set_key(e_cur, src_id, dst_id);
    if (e_cur->remove(e_cur) != 0)
    {
        throw GraphException("Failed to delete the edge between " +
                             std::to_string(src_id) + " and " +
                             std::to_string(dst_id));
    }
    if (locks != nullptr)
    {
        omp_set_lock(locks->get_edge_num_lock());
        set_num_edges(get_num_edges() - 1, metadata_cursor);
        omp_unset_lock(locks->get_edge_num_lock());
    }
    else
    {
        set_num_edges(get_num_edges() - 1, metadata_cursor);
    }
    // delete reverse edge
    if (!opts.is_directed)
    {
        e_cur->set_key(e_cur, dst_id, src_id);
        if (e_cur->remove(e_cur) != 0)
        {
            throw GraphException("Failed to remove the reverse  edge between " +
                                 std::to_string(src_id) + " and " +
                                 to_string(dst_id));
        }
        if (locks != nullptr)
        {
            omp_set_lock(locks->get_edge_num_lock());
            set_num_edges(get_num_edges() - 1, metadata_cursor);
            omp_unset_lock(locks->get_edge_num_lock());
        }
        else
        {
            set_num_edges(get_num_edges() - 1, metadata_cursor);
        }
    }

    // update node degrees
    if (opts.read_optimize)
    {
        if (locks != nullptr)
        {
            omp_set_lock(locks->get_node_degree_lock());
        }

        node src = get_node(src_id);

        if (src.out_degree > 0)
        {
            src.out_degree--;
        }

        if (!opts.is_directed && src.in_degree > 0)
        {
            src.in_degree--;
        }
        update_node_degree(src.id, src.in_degree, src.out_degree);

        node dst = get_node(dst_id);
        if (dst.in_degree > 0)
        {
            dst.in_degree--;
        }
        if (!opts.is_directed && dst.out_degree > 0)
        {
            dst.out_degree--;
        }
        update_node_degree(dst_id, dst.in_degree, dst.out_degree);

        if (locks != nullptr)
        {
            omp_unset_lock(locks->get_node_degree_lock());
        }
    }
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
    edge found = {-1, -1, -1};
    WT_CURSOR *e_cur = get_edge_cursor();
    e_cur->set_key(e_cur, src_id, dst_id);
    if (e_cur->search(e_cur) == 0)
    {
        found.src_id = src_id;
        found.dst_id = dst_id;
        CommonUtil::__record_to_edge_ekey(e_cur, &found);
        return found;
    }
    else
    {
        return found;
    }
}

/**
 * @brief Get all nodes in the table
 *
 * @return std::vector<node> the list containing all nodes.
 */
std::vector<node> EdgeKey::get_nodes()
{
    std::vector<node> nodes;

    WT_CURSOR *dst_cur = get_dst_idx_cursor();
    WT_CURSOR *e_cur = get_edge_cursor();

    dst_cur->set_key(dst_cur, OutOfBand_ID);
    if (dst_cur->search(dst_cur) == 0)
    {
        node_id_t node_id, temp;
        dst_cur->get_value(dst_cur, &node_id, &temp);
        assert(temp == OutOfBand_ID);  // this should be true
        e_cur->set_key(e_cur, node_id, OutOfBand_ID);
        if (e_cur->search(e_cur) == 0)
        {
            node found{.id = node_id};
            CommonUtil::__record_to_node_ekey(e_cur, &found);
            nodes.push_back(found);
        }

        while (dst_cur->next(dst_cur) == 0)
        {
            dst_cur->get_value(dst_cur, &node_id, &temp);
            if (temp == OutOfBand_ID)
            {
                e_cur->set_key(e_cur, node_id, OutOfBand_ID);
                if (e_cur->search(e_cur) == 0)
                {
                    node found{.id = node_id};
                    CommonUtil::__record_to_node_ekey(e_cur, &found);
                    nodes.push_back(found);
                }
            }
            else
            {
                break;
            }
        }
    }
    dst_cur->reset(dst_cur);
    e_cur->reset(e_cur);
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
        WT_CURSOR *e_cur = get_edge_cursor();
        e_cur->set_key(e_cur, node_id, OutOfBand_ID);
        node found = {0};
        if (e_cur->search(e_cur) == 0)
        {
            found.id = node_id;
            CommonUtil::__record_to_node_ekey(e_cur, &found);
        }
        e_cur->reset(e_cur);
        return found.out_degree;
    }
    else
    {
        degree_t out_deg = 0;
        WT_CURSOR *src_cur = get_src_idx_cursor();
        src_cur->set_key(src_cur, node_id);
        if (src_cur->search(src_cur) == 0)
        {
            node_id_t src, dst;
            src_cur->get_value(src_cur, &src, &dst);

            if (src == node_id && dst != OutOfBand_ID)
            {
                out_deg++;
            }
            while (src_cur->next(src_cur) == 0)
            {
                src_cur->get_value(src_cur, &src, &dst);
                if (src == node_id)
                {
                    if (dst != OutOfBand_ID)
                    {
                        out_deg++;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        src_cur->reset(src_cur);
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
        WT_CURSOR *e_cur = get_edge_cursor();
        e_cur->set_key(e_cur, node_id, OutOfBand_ID);
        e_cur->search(e_cur);
        node found;
        CommonUtil::__record_to_node_ekey(e_cur, &found);
        e_cur->reset(e_cur);
        return found.in_degree;
    }
    else
    {
        degree_t in_degree = 0;
        WT_CURSOR *dst_cur = get_dst_idx_cursor();
        dst_cur->set_key(dst_cur, node_id);
        if (dst_cur->search(dst_cur) == 0)
        {
            node_id_t src, dst;
            dst_cur->get_value(dst_cur, &src, &dst);
            if (dst == node_id)
            {
                in_degree++;
            }
            while (dst_cur->next(dst_cur) == 0)
            {
                dst_cur->get_value(dst_cur, src, dst);
                if (dst == node_id)
                {
                    in_degree++;
                }
                else
                {
                    break;
                }
            }
        }
        dst_cur->reset(dst_cur);
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
    WT_CURSOR *e_cur = get_edge_cursor();

    while (e_cur->next(e_cur) == 0)
    {
        edge found;
        e_cur->get_key(e_cur, &found.src_id, &found.dst_id);
        if (found.dst_id != OutOfBand_ID)
        {
            if (opts.is_weighted)
            {
                CommonUtil::__record_to_edge_ekey(e_cur, &found);
            }
            edges.push_back(found);
        }
    }
    e_cur->reset(e_cur);
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
    WT_CURSOR *e_cur = get_edge_cursor();
    WT_CURSOR *src_cur = get_src_idx_cursor();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    src_cur->set_key(src_cur, node_id);
    int search_ret = src_cur->search(src_cur);
    int flag = 0;
    if (search_ret == 0)
    {
        node_id_t src_id, dst_id;

        do
        {
            src_cur->get_value(src_cur, &src_id, &dst_id);
            if (src_id != node_id)
            {
                break;
            }
            if (src_id == node_id && dst_id == OutOfBand_ID)
            {
                flag++;
            }
            else
            {
                e_cur->set_key(e_cur, src_id, dst_id);
                if (e_cur->search(e_cur) == 0)
                {
                    edge found{.src_id = src_id, .dst_id = dst_id};
                    CommonUtil::__record_to_edge_ekey(e_cur, &found);
                    out_edges.push_back(found);
                }
            }
        } while (src_id == node_id && src_cur->next(src_cur) == 0 &&
                 flag <= 1);  // flag check ensures that if the first entry we
                              // hit was (node_id, -1), we try to look for the
                              // next entry the cursor points to to see if it
                              // has a (node_id, <dst>) entry.
    }
    e_cur->reset(e_cur);
    src_cur->reset(src_cur);
    return out_edges;
}

/**
 * @brief Returns a list containing all nodes that have an incoming edge from
 * node_id
 *
 * @param node_id Node ID of the node who's out nodes are sought.
 * @return std::vector<node>
 */
std::vector<node> EdgeKey::get_out_nodes(node_id_t node_id)
{
    std::vector<node> out_nodes;
    WT_CURSOR *src_cur = get_src_idx_cursor();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    src_cur->set_key(src_cur, node_id);
    int search_ret = src_cur->search(src_cur);
    int flag = 0;
    if (search_ret == 0)
    {
        node_id_t src, dst;
        do
        {
            src_cur->get_value(src_cur, &src, &dst);
            // don't need to check if src == node_id because search_ret was 0
            if (src != node_id)
            {
                break;
            }
            if (src == node_id && dst == OutOfBand_ID)
            {
                flag++;
            }
            else
            {
                out_nodes.push_back(get_node(dst));
            }
        } while (src == node_id && src_cur->next(src_cur) == 0 &&
                 flag <= 1);  // flag check ensures that if the first entry we
                              // hit was (node_id, -1), we try to look for the
                              // next entry the cursor points to to see if it
                              // has a (node_id, <dst>) entry.
    }
    src_cur->reset(src_cur);
    return out_nodes;
}

/**
 * @brief Returns a list containing ids of all nodes that have an incoming edge
 * from node_id; uses one less cursor search than get_out_nodes
 *
 * @param node_id Node ID of the node who's out nodes are sought.
 * @return std::vector<node>
 */
std::vector<node_id_t> EdgeKey::get_out_nodes_id(node_id_t node_id)
{
    std::vector<node_id_t> out_nodes_id;
    WT_CURSOR *src_cur = get_src_idx_cursor();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    src_cur->set_key(src_cur, node_id);
    int search_ret = src_cur->search(src_cur);
    int flag = 0;
    if (search_ret == 0)
    {
        node_id_t src, dst;
        do
        {
            src_cur->get_value(src_cur, &src, &dst);
            // don't need to check if src == node_id because search_ret was 0
            if (src != node_id)
            {
                break;
            }
            if (src == node_id && dst == OutOfBand_ID)
            {
                flag++;
            }
            else
            {
                out_nodes_id.push_back(dst);
            }
        } while (src == node_id && src_cur->next(src_cur) == 0 &&
                 flag <= 1);  // flag check ensures that if the first entry we
                              // hit was (node_id, -1), we try to look for the
                              // next entry the cursor points to to see if it
                              // has a (node_id, <dst>) entry.
    }
    src_cur->reset(src_cur);
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
    WT_CURSOR *e_cur = get_edge_cursor();
    WT_CURSOR *dst_cur = get_dst_idx_cursor();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    dst_cur->set_key(dst_cur, node_id);
    int search_ret = dst_cur->search(dst_cur);
    if (search_ret == 0)
    {
        node_id_t src_id, dst_id;
        dst_cur->get_value(dst_cur, &src_id, &dst_id);

        do
        {
            e_cur->set_key(e_cur, src_id, dst_id);
            if (e_cur->search(e_cur) == 0)
            {
                edge found{.src_id = src_id, .dst_id = dst_id};
                CommonUtil::__record_to_edge_ekey(e_cur, &found);
                in_edges.push_back(found);
            }
            dst_cur->next(dst_cur);
            dst_cur->get_value(dst_cur, &src_id, &dst_id);
        } while (dst_id == node_id);
    }
    e_cur->reset(e_cur);
    dst_cur->reset(dst_cur);
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
    WT_CURSOR *dst_cur = get_dst_idx_cursor();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    dst_cur->set_key(dst_cur, node_id);
    if (dst_cur->search(dst_cur) == 0)
    {
        node_id_t src_id, dst_id;
        if (dst_cur->get_value(dst_cur, &src_id, &dst_id) != 0)
        {
            throw GraphException("here" + __LINE__);
        }

        do
        {
            in_nodes.push_back(get_node(src_id));
            if (dst_cur->next(dst_cur) == 0)
            {
                if (dst_cur->get_value(dst_cur, &src_id, &dst_id) != 0)
                {
                    throw GraphException("here" + __LINE__);
                }
            }
            else
            {
                break;
            }

        } while (dst_id == node_id);
    }
    dst_cur->reset(dst_cur);
    return in_nodes;
}

/**
 * @brief Get a list of ids of all nodes that have an outgoing edge to node_id;
 * uses one less cursor search than get_in_nodes
 *
 * @param node_id the node whose in_nodes are sought.
 * @return std::vector<node>
 */
// Should we delete the debug info here?
std::vector<node_id_t> EdgeKey::get_in_nodes_id(node_id_t node_id)
{
    std::vector<node_id_t> in_nodes_id;
    WT_CURSOR *dst_cur = get_dst_idx_cursor();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    dst_cur->set_key(dst_cur, node_id);
    if (dst_cur->search(dst_cur) == 0)
    {
        node_id_t src_id, dst_id;
        dst_cur->get_value(dst_cur, &src_id, &dst_id);
        while (dst_id == node_id)
        {
            in_nodes_id.push_back(src_id);
            if (!dst_cur->next(dst_cur) == 0)
            {
                break;
            }
            dst_cur->get_value(dst_cur, &src_id, &dst_id);
        }
    }
    dst_cur->reset(dst_cur);
    return in_nodes_id;
}

void EdgeKey::make_indexes()
{
    if (edge_cursor != nullptr)
    {
        CommonUtil::close_cursor(edge_cursor);
    }
    if (src_idx_cursor != nullptr)
    {
        CommonUtil::close_cursor(src_idx_cursor);
    }
    if (dst_idx_cursor != nullptr)
    {
        CommonUtil::close_cursor(dst_idx_cursor);
    }
    create_indices(session);
}
/**
 * @brief Creates the indices on the SRC and the DST column of the edge table.
 * These are not (and should not be) used for inserting data into the edge
 * table if write_optimize is on. Once the data has been inserted, the
 * create_indices function must be called separately.
 *
 * This function requires exclusive access to the table on which it operates.
 * If there are any open cursors, or if the table has any other operation
 * ongoing, WT throws a Resource Busy error.
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
            "Failed to create an index on the DST column of the edge table");
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
void EdgeKey::drop_indices()
{
    CommonUtil::close_cursor(edge_cursor);
    CommonUtil::close_cursor(src_idx_cursor);
    CommonUtil::close_cursor(dst_idx_cursor);

    // drop SRC index
    string uri = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
    session->drop(session, uri.c_str(), NULL);

    // drop the DST index
    uri = "index:" + EDGE_TABLE + ":" + DST_INDEX;
    session->drop(session, uri.c_str(), NULL);
}

// Session/Connection/Cursor operations

/*
Get cursors
*/

WT_CURSOR *EdgeKey::get_metadata_cursor()
{
    if (metadata_cursor == nullptr)
    {
        int ret = _get_table_cursor(METADATA, &metadata_cursor, session, false);
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
        string projection = "(" + DST + "," + SRC + "," + ATTR + ")";
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
    string projection = "(" + DST + "," + SRC + "," + ATTR + ")";
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
    toReturn->set_key_range({-1, (int64_t)num_nodes - 1});
    return toReturn;
}

InCursor *EdgeKey::get_innbd_iter()
{
    uint64_t num_nodes = this->get_num_nodes();
    InCursor *toReturn = new EkeyInCursor(get_new_dst_idx_cursor(), session);
    toReturn->set_num_nodes(num_nodes);
    toReturn->set_key_range({-1, (int64_t)num_nodes - 1});
    return toReturn;
}

NodeCursor *EdgeKey::get_node_iter()
{
    return new EkeyNodeCursor(get_new_dst_src_idx_cursor(), session);
}

EdgeCursor *EdgeKey::get_edge_iter()
{
    return new EkeyEdgeCursor(get_new_edge_cursor(), session);
}

WT_CURSOR *EdgeKey::get_node_cursor() { return get_dst_src_idx_cursor(); }

WT_CURSOR *EdgeKey::get_new_node_cursor() { return get_new_dst_idx_cursor(); }

node EdgeKey::get_next_node(WT_CURSOR *dst_cur)
{
    dst_cur->set_key(dst_cur, OutOfBand_ID);
    WT_CURSOR *e_cur = get_edge_cursor();
    node found = {-1};
    if (dst_cur->search(dst_cur) == 0)
    {
        node_id_t node_id, temp;
        dst_cur->get_value(dst_cur, &node_id, &temp);
        assert(temp == OutOfBand_ID);  // this should be true
        e_cur->set_key(e_cur, node_id, OutOfBand_ID);
        if (e_cur->search(e_cur) == 0)
        {
            found.id = node_id;
            CommonUtil::__record_to_node_ekey(e_cur, &found);
        }
    }
    return found;
}

WT_CURSOR *EdgeKey::get_edge_cursor()
{
    if (edge_cursor == nullptr)
    {
        if (_get_table_cursor(EDGE_TABLE, &edge_cursor, session, false) != 0)
        {
            throw GraphException("Could not get a cursor to the Edge table");
        }
    }

    return edge_cursor;
}

WT_CURSOR *EdgeKey::get_new_edge_cursor()
{
    WT_CURSOR *new_edge_cursor = nullptr;
    if (_get_table_cursor(EDGE_TABLE, &new_edge_cursor, session, false) != 0)
    {
        throw GraphException("Could not get a cursor to the Edge table");
    }

    return new_edge_cursor;
}

edge EdgeKey::get_next_edge(WT_CURSOR *e_cur)
{
    edge found = {-1};
    while (e_cur->next(e_cur) == 0)
    {
        e_cur->get_key(e_cur, &found.src_id, &found.dst_id);
        if (found.dst_id != OutOfBand_ID)
        {
            if (opts.is_weighted)
            {
                CommonUtil::__record_to_edge_ekey(e_cur, &found);
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

void EdgeKey::add_to_nnodes(int amnt)
{
    if (locks != nullptr)
    {
        omp_set_lock(locks->get_node_num_lock());
        set_num_nodes(get_num_nodes() + amnt, this->metadata_cursor);
        omp_unset_lock(locks->get_node_num_lock());
    }
    else
    {
        set_num_nodes(get_num_nodes() + amnt, this->metadata_cursor);
    }
}

void EdgeKey::add_to_nedges(int amnt)
{
    if (locks != nullptr)
    {
        omp_set_lock(locks->get_edge_num_lock());
        set_num_edges(get_num_edges() + amnt, this->metadata_cursor);
        omp_unset_lock(locks->get_edge_num_lock());
    }
    else
    {
        set_num_edges(get_num_edges() + amnt, this->metadata_cursor);
    }
}
