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

StandardGraph::StandardGraph(graph_opts &opt_params) : GraphBase(opt_params)
{
    if (opts.create_new)
    {
        create_new_graph();
        if (opts.optimize_create == false)
        {
            /**
             *  Create indices with graph.
             * NOTE: If user opts to optimize graph creation, then they're
             * responsible for calling create_indices() AFTER all the
             * nodes/edges have been added to graph.
             * This is done to improve performance of bulk-loading.
             */
            create_indices();
        }
    }
    else
    {
        // Check that the DB directory exists
        std::string dirname = opts.db_dir + "/" + opts.db_name;
        if (CommonUtil::check_dir_exists(dirname))
        {
            __restore_from_db(dirname);
        }
        else
        {
            throw GraphException(
                "Could not find the expected WT DB directory - " + dirname);
        }
    }
}

void StandardGraph::create_new_graph()
{
    int ret;
    // Create new directory for WT DB
    std::string dirname = opts.db_dir + "/" + opts.db_name;
    CommonUtil::create_dir(dirname);

    // open connection to WT
    if (CommonUtil::open_connection(const_cast<char *>(dirname.c_str()),
                                    opts.stat_log,
                                    opts.conn_config,
                                    &conn) < 0)
    {
        exit(-1);
    };

    // Open a session handle for the database
    if (CommonUtil::open_session(conn, &session) != 0)
    {
        exit(-1);
    }

    // Set up the node table
    // The node entry is of the form: <id>,<in_degree><out_degree>
    // If the graph is opts.read_optimized, add columns and format for in/out
    // degrees
    if (opts.read_optimize)
    {
        node_columns.push_back(IN_DEGREE);
        node_columns.push_back(OUT_DEGREE);
        node_value_format = "II";
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
        session, NODE_TABLE, node_columns, node_key_format, node_value_format);

    // ******** Now set up the Edge Table     **************
    // Edge Column Format : <src><dst><weight>
    // Now prepare the edge value format. starts with II for src,dst. Add
    // another I if weighted
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
        session, EDGE_TABLE, edge_columns, edge_key_format, edge_value_format);

    /* Now doing the metadata table creation. //TODO:
     function This table stores the graph metadata
     value_format:raw byte string (Su)
     key_format: string (S)
  */
    string metadata_table_name = "table:" + string(METADATA);
    if ((ret = session->create(session,
                               metadata_table_name.c_str(),
                               "key_format=S,value_format=S")) > 0)
    {
        fprintf(stderr, "Failed to create the metadata table ");
    }

    if ((ret = _get_table_cursor(
             METADATA, &this->metadata_cursor, this->session, false)) != 0)
    {
        fprintf(stderr, "Failed to create cursor to the metadata table.");
        exit(-1);
    }

    insert_metadata(DB_NAME,
                    const_cast<char *>(opts.db_name.c_str()),
                    this->metadata_cursor);

    // opts.db_dir
    insert_metadata(opts.db_dir,
                    const_cast<char *>(opts.db_dir.c_str()),
                    this->metadata_cursor);

    // opts.READ_OPTIMIZE
    string is_read_optimized_str = opts.read_optimize ? "true" : "false";
    insert_metadata(READ_OPTIMIZE,
                    const_cast<char *>(is_read_optimized_str.c_str()),
                    this->metadata_cursor);

    // IS_DIRECTED
    string is_directed_str = opts.is_directed ? "true" : "false";
    insert_metadata(IS_DIRECTED,
                    const_cast<char *>(is_directed_str.c_str()),
                    this->metadata_cursor);

    // opts.IS_WEIGHTED
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

void StandardGraph::make_indexes() { create_indices(); }
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
void StandardGraph::create_indices()
{
    if (this->edge_cursor != nullptr)
    {
        CommonUtil::close_cursor(this->edge_cursor);
    }
    if (this->src_dst_index_cursor != nullptr)
    {
        CommonUtil::close_cursor(this->src_dst_index_cursor);
    }
    if (this->src_index_cursor != nullptr)
    {
        CommonUtil::close_cursor(this->src_index_cursor);
    }
    if (this->dst_index_cursor != nullptr)
    {
        CommonUtil::close_cursor(this->dst_index_cursor);
    }

    // Create table index on (src, dst)
    int ret = 0;
    string edge_table_index_str, edge_table_index_conf_str;
    edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_DST_INDEX;
    edge_table_index_conf_str = "columns=(" + SRC + "," + DST + ")";
    // THere is likely a util fucntion for this
    if ((ret = session->create(session,
                               edge_table_index_str.c_str(),
                               edge_table_index_conf_str.c_str())) > 0)
    {
        fprintf(stderr,
                "Failed to create index SRC_DST_INDEX in the edge table");
    }
    // Create index on SRC column
    edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
    edge_table_index_conf_str = "columns=(" + SRC + ")";
    if ((ret = session->create(session,
                               edge_table_index_str.c_str(),
                               edge_table_index_conf_str.c_str())) > 0)
    {
        fprintf(stderr, "Failed to create index SRC_INDEX in the edge table");
    }
    // Create index on DST column
    edge_table_index_str = "index:" + EDGE_TABLE + ":" + DST_INDEX;
    edge_table_index_conf_str = "columns=(" + DST + ")";
    if ((ret = session->create(session,
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
void StandardGraph::add_node(node to_insert)
{
    int ret = 0;
    if (node_cursor == NULL)
    {
        ret = _get_table_cursor(NODE_TABLE, &node_cursor, session, false);
    }
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

    if (ret != 0)
    {
        throw GraphException("Failed to add node_id" +
                             std::to_string(to_insert.id));
    }
    set_num_nodes(get_num_nodes() + 1, metadata_cursor);
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
    int ret = 0;
    if (this->node_cursor == NULL)
    {
        ret = _get_table_cursor(NODE_TABLE, &node_cursor, session, false);
    }
    this->node_cursor->set_key(this->node_cursor, node_id);
    ret = this->node_cursor->search(this->node_cursor);
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
 * @brief This function tests if the edge identified by <src_id, dst_id> exists
 *
 * @param src_id source node id
 * @param dst_id destination node_id
 * @return true/false for if the edge exists/does not exist
 */
bool StandardGraph::has_edge(node_id_t src_id, node_id_t dst_id)
{
    int ret = 0;
    string projection = "(" + SRC + "," + DST + ")";
    if (src_dst_index_cursor == NULL)
    {
        ret = _get_index_cursor(
            EDGE_TABLE, SRC_DST_INDEX, projection, &(src_dst_index_cursor));
    }
    src_dst_index_cursor->set_key(src_dst_index_cursor, src_id, dst_id);
    ret = src_dst_index_cursor->search(src_dst_index_cursor);
    if (ret != 0)
    {
        return false;
    }
    else
    {
        //? WHY IS THIS NECESSARY?
        node_id_t temp_src_id = 0, temp_dst_id = 0;

        ret = src_dst_index_cursor->get_value(
            src_dst_index_cursor, &temp_src_id, &temp_dst_id);
        if ((temp_src_id == src_id) & (temp_dst_id == dst_id))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

/**
 * @brief This function tests if the edge identified by <src_id, dst_id> exists
 *
 * @param src_id source node id
 * @param dst_id destination node_id
 * @return true/false for if the edge exists/does not exist
 */
edge StandardGraph::get_edge(node_id_t src_id, node_id_t dst_id)
{
    int ret = 0;
    edge found = {-1, -1, -1};
    string projection = "(" + SRC + "," + DST + "," + WEIGHT + ")";
    if (src_dst_index_cursor == NULL)
    {
        ret = _get_index_cursor(
            EDGE_TABLE, SRC_DST_INDEX, projection, &(src_dst_index_cursor));
    }
    src_dst_index_cursor->set_key(src_dst_index_cursor, src_id, dst_id);
    ret = src_dst_index_cursor->search(src_dst_index_cursor);
    if (ret == 0)
    {
        ret = src_dst_index_cursor->get_value(src_dst_index_cursor,
                                              &found.src_id,
                                              &found.dst_id,
                                              &found.edge_weight);
    }
    return found;
}

node StandardGraph::get_node(node_id_t node_id)
{
    node found = {0};
    int ret = 0;

    WT_CURSOR *cursor = get_node_cursor();
    cursor->set_key(cursor, node_id);

    ret = cursor->search(cursor);
    if (ret != 0)
    {
        throw GraphException("Could not find a node with nodeId " +
                             std::to_string(node_id));
    }
    CommonUtil::__record_to_node(cursor, &found, opts.read_optimize);
    found.id = node_id;
    return found;
}

node StandardGraph::get_random_node()
{
    node found = {0};
    int ret = 0;

    if (this->random_node_cursor == NULL)
    {
        ret = _get_table_cursor(NODE_TABLE, &random_node_cursor, session, true);
    }
    random_node_cursor->reset(random_node_cursor);

    ret = this->random_node_cursor->next(random_node_cursor);
    if (ret != 0)
    {
        throw GraphException(wiredtiger_strerror(ret));
    }
    random_node_cursor->get_key(random_node_cursor, &found.id);
    CommonUtil::__record_to_node(
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
void StandardGraph::delete_node(node_id_t node_id)
{
    int ret = 0;

    if (this->node_cursor == NULL)
    {
        ret = _get_table_cursor(NODE_TABLE, &node_cursor, session, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a cursor to the node table");
        }
    }
    // Delete incoming and outgoing edges
    vector<edge> incoming_edges = get_in_edges(node_id);
    vector<edge> outgoing_edges = get_out_edges(node_id);

    for (edge e : incoming_edges)
    {
        delete_edge(e.src_id, e.dst_id);
    }
    for (edge e : outgoing_edges)
    {
        delete_edge(e.src_id, e.dst_id);
    }

    // Delete the node with the matching node_id
    node_cursor->set_key(node_cursor, node_id);
    ret = node_cursor->remove(node_cursor);

    if (ret != 0)
    {
        throw GraphException("Could not delete node with ID " +
                             to_string(node_id));
    }
    set_num_nodes(get_num_nodes() - 1, metadata_cursor);
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
        if (dst_index_cursor == NULL)
        {
            string projection = "(" + SRC + "," + DST + ")";
            if (_get_index_cursor(
                    EDGE_TABLE, DST_INDEX, projection, &(dst_index_cursor)) !=
                0)
            {
                throw GraphException(
                    "Could not get a DST index cursor on the edge table");
            }
        }
        degree_t count = 0;
        dst_index_cursor->set_key(dst_index_cursor, node_id);

        if (dst_index_cursor->search(dst_index_cursor) == 0)
        {
            count++;
            while (dst_index_cursor->next(dst_index_cursor) == 0)
            {
                node_id_t src, dst = 0;
                dst_index_cursor->get_value(dst_index_cursor, &src, &dst);
                if (dst == node_id)
                {
                    count++;
                }
            }
        }
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
        if (src_index_cursor == NULL)
        {
            string projection = "(" + SRC + "," + DST + ")";
            if (_get_index_cursor(
                    EDGE_TABLE, SRC_INDEX, projection, &(src_index_cursor)) !=
                0)
            {
                throw GraphException(
                    "Could not get a SRC index cursor on the edge table");
            }
        }
        degree_t count = 0;

        src_index_cursor->set_key(src_index_cursor, node_id);

        if (src_index_cursor->search(src_index_cursor) == 0)
        {
            count++;

            while (src_index_cursor->next(src_index_cursor) == 0)
            {
                node_id_t src, dst = 0;
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
    WT_CURSOR *cursor = get_node_cursor();
    while ((ret = cursor->next(cursor) == 0))
    {
        node item{0};
        CommonUtil::__record_to_node(cursor, &item, opts.read_optimize);
        cursor->get_key(cursor, &item.id);

        nodes.push_back(item);
    }
    return nodes;
}

/**
 * @brief get all nodes in the nodes table
 * @return std::vector<node> the vector of all nodes in the node table
 */
void StandardGraph::get_nodes(vector<node> &nodes)
{
    int ret;
    WT_CURSOR *cursor = get_node_cursor();
    while ((ret = cursor->next(cursor) == 0))
    {
        node item{0};
        cursor->get_key(cursor, &item.id);
        if (opts.read_optimize)
        {
            cursor->get_value(cursor, &item.in_degree, &item.out_degree);
        }
        else
        {
            item.in_degree = get_in_degree(item.id);
            item.out_degree = get_out_degree(item.id);
        }

        nodes.push_back(item);
    }
}

/**
 * @brief Insert an edge into the edge table
 *
 * @param to_insert The edge struct containing the edge to be inserted.
 */
void StandardGraph::add_edge(edge to_insert, bool is_bulk)
{
    WT_CURSOR *e_cursor, *n_cursor;
    if (!is_bulk)
    {
        // Add dst and src nodes if they don't exist.

        if (!has_node(to_insert.src_id))
        {
            node src = {0};
            src.id = to_insert.src_id;
            add_node(src);
        }
        if (!has_node(to_insert.dst_id))
        {
            node dst = {0};
            dst.id = to_insert.dst_id;
            add_node(dst);
        }
    }
    int ret;
    e_cursor = get_edge_cursor();
    e_cursor->set_key(e_cursor, to_insert.src_id, to_insert.dst_id);

    if (opts.is_weighted)
    {
        e_cursor->set_value(e_cursor, to_insert.edge_weight);
    }
    else
    {
        e_cursor->set_value(e_cursor, 0);
    }
    ret = e_cursor->insert(e_cursor);
    if (ret != 0)
    {
        throw GraphException("Failed to insert edge (" +
                             to_string(to_insert.src_id) + "," +
                             to_string(to_insert.dst_id));
    }

    set_num_edges(get_num_edges() + 1, metadata_cursor);
    // FIXME: This does not handle the undirected scenario
    if (!is_bulk)
    {
        // If opts.read_optimized is true, we update in/out degreees in the node
        // table.
        if (this->opts.read_optimize)
        {
            // update in/out degrees for src node in NODE_TABLE
            n_cursor = get_node_cursor();

            n_cursor->set_key(n_cursor, to_insert.src_id);
            if (n_cursor->search(n_cursor) != 0)
            {
                throw GraphException("Could not find a node with ID " +
                                     to_string(to_insert.src_id));
            }
            node found;
            CommonUtil::__record_to_node(n_cursor, &found, opts.read_optimize);
            found.id = to_insert.src_id;
            found.out_degree = found.out_degree + 1;
            update_node_degree(
                n_cursor,
                found.id,
                found.in_degree,
                found.out_degree);  //! pass the cursor to this function

            // update in/out degrees for the dst node in the NODE_TABLE
            n_cursor->reset(n_cursor);
            n_cursor->set_key(n_cursor, to_insert.dst_id);
            n_cursor->search(n_cursor);
            CommonUtil::__record_to_node(n_cursor, &found, opts.read_optimize);
            found.id = to_insert.dst_id;
            found.in_degree = found.in_degree + 1;
            update_node_degree(
                n_cursor, found.id, found.in_degree, found.out_degree);
        }
    }
}

void StandardGraph::delete_edge(node_id_t src_id, node_id_t dst_id)
{
    node n_found;
    WT_CURSOR *e_cursor, *n_cursor;
    int ret;
    if (!has_edge(src_id, dst_id))
    {
        return;
    }
    e_cursor = get_edge_cursor();

    e_cursor->set_key(e_cursor, src_id, dst_id);
    ret = e_cursor->remove(e_cursor);
    CommonUtil::check_return(ret,
                             "Failed to delete edge (" + to_string(src_id) +
                                 "," + to_string(dst_id));
    set_num_edges(get_num_edges() - 1, metadata_cursor);
    // Delete reverse edge if the graph is undirected.
    if (!opts.is_directed)
    {
        if (!has_edge(dst_id, src_id))
        {
            return;
        }
        e_cursor->set_key(e_cursor, dst_id, src_id);
        ret = e_cursor->remove(e_cursor);
        CommonUtil::check_return(ret,
                                 "Failed to delete edge (" + to_string(src_id) +
                                     "," + to_string(dst_id));
        set_num_edges(get_num_edges() - 1, metadata_cursor);
    }

    // Update in/out degrees for the src and dst nodes if the graph is read
    // optimized

    n_cursor = get_node_cursor();
    // Update src node's in/out degrees
    n_found = {.id = src_id, .in_degree = 0, .out_degree = 0};
    n_cursor->set_key(n_cursor, n_found.id);
    n_cursor->search(n_cursor);
    CommonUtil::__record_to_node(n_cursor, &n_found, opts.read_optimize);

    // Assert that the out degree and later on in degree are > 0.
    // If not then raise an exceptiion, because we shouldn't have deleted an
    // edge where src/dst have degree 0.
    if ((opts.is_directed and n_found.out_degree == 0) or
        ((!opts.is_directed) and
         ((n_found.out_degree == 0) or (n_found.in_degree == 0))))
    {
        throw GraphException(
            "Deleted an edge between src nodeid: " + to_string(src_id) + "," +
            " dst node id:" + to_string(dst_id));
    }

    n_found.out_degree = n_found.out_degree - 1;
    if (!opts.is_directed)
    {
        n_found.in_degree = n_found.in_degree - 1;
    }
    update_node_degree(
        n_cursor, n_found.id, n_found.in_degree, n_found.out_degree);

    // Update dst node's in/out degrees
    n_found = {.id = dst_id, .in_degree = 0, .out_degree = 0};
    n_cursor->set_key(n_cursor, n_found.id);
    n_cursor->search(n_cursor);
    CommonUtil::__record_to_node(n_cursor, &n_found, opts.read_optimize);

    if ((opts.is_directed and n_found.in_degree == 0) or
        ((!opts.is_directed) and (n_found.out_degree == 0) and
         (n_found.in_degree == 0)))
    {
        throw GraphException(
            "Deleted an edge between src nodeid: " + to_string(src_id) + "," +
            " dst node id:" + to_string(dst_id));
    }
    n_found.in_degree = n_found.in_degree - 1;
    if (!opts.is_directed)
    {
        n_found.out_degree = n_found.out_degree - 1;
    }
    update_node_degree(
        n_cursor, n_found.id, n_found.in_degree, n_found.out_degree);
}

void StandardGraph::update_node_degree(WT_CURSOR *cursor,
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

    node found;  //__record_to_node(cursor);

    found.in_degree = in_degree;
    found.out_degree = out_degree;
    found.id = node_id;

    CommonUtil::__node_to_record(cursor, found, opts.read_optimize);
}

/**
 * @brief Get all edges in the edge table
 *
 * @return std::vector<edge> the vector containing all edges in the edge table
 */
std::vector<edge> StandardGraph::get_edges()
{
    int ret;
    WT_CURSOR *cursor = get_edge_cursor();
    vector<edge> edges;
    while ((ret = cursor->next(cursor) == 0))
    {
        edge found;
        CommonUtil::__record_to_edge(cursor, &found);
        edges.push_back(found);
    }

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

    WT_CURSOR *cursor = get_src_idx_cursor();
    cursor->reset(cursor);
    cursor->set_key(cursor, node_id);

    node_id_t temp = -1;
    cursor->get_key(cursor, &temp);

    if (cursor->search(cursor) == 0)
    {
        edge found;
        CommonUtil::__read_from_edge_idx(cursor, &found);
        edges.push_back(found);
        while (cursor->next(cursor) == 0)
        {
            CommonUtil::__read_from_edge_idx(cursor, &found);
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
    WT_CURSOR *cursor = get_node_cursor();
    int ret;
    out_edges = get_out_edges(node_id);
    if (out_edges.size() > 0)
    {
        for (edge out_edge : out_edges)
        {
            cursor->set_key(cursor, out_edge.dst_id);
            ret = cursor->search(cursor);
            if (ret == 0)
            {
                node found_dst;
                CommonUtil::__record_to_node(
                    cursor, &found_dst, opts.read_optimize);
                found_dst.id = out_edge.dst_id;
                nodes.push_back(found_dst);
            }
        }
    }
    return nodes;
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
    if (dst_index_cursor == NULL)
    {
        string projection = "(" + SRC + "," + DST + ")";
        if (_get_index_cursor(
                EDGE_TABLE, DST_INDEX, projection, &(dst_index_cursor)) != 0)
        {
            throw GraphException(
                "Could not get a DST index cursor on the edge table");
        }
    }
    dst_index_cursor->set_key(dst_index_cursor, node_id);
    if (dst_index_cursor->search(dst_index_cursor) == 0)
    {
        edge found;
        CommonUtil::__read_from_edge_idx(dst_index_cursor, &found);
        edges.push_back(found);
        while (dst_index_cursor->next(dst_index_cursor) == 0)
        {
            CommonUtil::__read_from_edge_idx(dst_index_cursor, &found);
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
    WT_CURSOR *cursor = get_dst_idx_cursor();
    if (!has_node(node_id))
    {
        throw GraphException("Could not find node with id" +
                             to_string(node_id));
    }

    cursor->set_key(cursor, node_id);
    if (cursor->search(cursor) == 0)
    {
        edge found;
        CommonUtil::__read_from_edge_idx(cursor, &found);
        nodes.push_back(get_node(found.src_id));
        while (cursor->next(cursor) == 0)
        {
            CommonUtil::__read_from_edge_idx(cursor, &found);
            if (found.dst_id == node_id)
            {
                nodes.push_back(get_node(found.src_id));
            }
            else
            {
                break;
            }
        }
    }
    cursor->reset(cursor);

    return nodes;
}

WT_CURSOR *StandardGraph::get_node_cursor()
{
    if (node_cursor == nullptr)
    {
        int ret = _get_table_cursor(NODE_TABLE, &node_cursor, session, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a node cursor");
        }
    }

    return node_cursor;
}

WT_CURSOR *StandardGraph::get_edge_cursor()
{
    if (edge_cursor == nullptr)
    {
        int ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, session, false);
        if (ret != 0)
        {
            throw GraphException("Could not get an edge cursor");
        }
    }

    return edge_cursor;
}
WT_CURSOR *StandardGraph::get_src_idx_cursor()
{
    string projection = "(" + SRC + "," + DST + ")";
    if (_get_index_cursor(
            EDGE_TABLE, SRC_INDEX, projection, &(src_index_cursor)) != 0)
    {
        throw GraphException(
            "Could not get a SRC index cursor on the edge table");
    }

    return src_index_cursor;
}

WT_CURSOR *StandardGraph::get_dst_idx_cursor()
{
    string projection = "(" + SRC + "," + DST + ")";
    if (_get_index_cursor(
            EDGE_TABLE, DST_INDEX, projection, &(dst_index_cursor)) != 0)
    {
        throw GraphException(
            "Could not get a DST index cursor on the edge table");
    }

    return dst_index_cursor;
}

WT_CURSOR *StandardGraph::get_src_dst_idx_cursor()
{
    string projection = "(" + SRC + "," + DST + ")";
    if (_get_index_cursor(
            EDGE_TABLE, SRC_DST_INDEX, projection, &(src_dst_index_cursor)) !=
        0)
    {
        throw GraphException(
            "Could not get a DST index cursor on the edge table");
    }

    return src_dst_index_cursor;
}

StdIterator::OutCursor StandardGraph::get_outnbd_iter()
{
    return StdIterator::OutCursor(get_src_idx_cursor(), session);
}

StdIterator::InCursor StandardGraph::get_innbd_iter()
{
    return StdIterator::InCursor(get_dst_idx_cursor(), session);
}

StdIterator::NodeCursor StandardGraph::get_node_iter()
{
    return StdIterator::NodeCursor(get_node_cursor(), session);
}

StdIterator::EdgeCursor StandardGraph::get_edge_iter()
{
    return StdIterator::EdgeCursor(get_edge_cursor(), session);
}

/**
 * @brief This is a function that I am using to test how search using cursors
 * works. It returns a cursor pointing to the first entry where the condition is
 * true and you can call next() on the cursor to get all records that match the
 * condition -- Break when the condition is not true anymore.
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
        CommonUtil::__record_to_node(n_iter, &found, opts.read_optimize);
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
        CommonUtil::__record_to_edge(e_iter, &found);
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
        CommonUtil::__read_from_edge_idx(cursor, &found);
        edges.push_back(found);
        while (cursor->next(cursor) == 0)
        {
            CommonUtil::__read_from_edge_idx(cursor, &found);
            edges.push_back(found);
        }
    }
    return edges;
}