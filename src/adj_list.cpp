#include "adj_list.h"
#include "common.h"
#include <algorithm>
#include <cstring>
#include <cassert>
#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;
const std::string GRAPH_PREFIX = "adj";

AdjList::AdjList()
{
    throw GraphException("No parameters passed in graph_opts. Exiting now.");
}

AdjList::AdjList(graph_opts &opt_params)

{
    this->opts = opt_params;
    try
    {
        CommonUtil::check_graph_params(opt_params);
    }
    catch (GraphException &G)
    {
        std::cout << G.what() << std::endl;
    }

    if (opt_params.create_new)
    {
        create_new_graph();
    }
    else
    {
        std::string dirname = opts.db_dir + "/" + opts.db_name;
        if (CommonUtil::check_dir_exists(dirname))
        {
            __restore_from_db(dirname);
        }
        else
        {
            throw GraphException("Could not find the expected WT DB directory - " + dirname);
        }
    }
}

/**
 * @brief Returns the metadata associated with the key param from the METADATA
 * table.
 * Same from standard_graph implementation
 */
string AdjList::get_metadata(const string &key)
{

    int ret = 0;
    if (this->metadata_cursor == NULL)
    {
        if ((ret = _get_table_cursor(METADATA, &metadata_cursor, false)) != 0)
        {
            fprintf(stderr, "Failed to create cursor to the metadata table.");
            exit(-1);
        }
    }
    metadata_cursor->set_key(metadata_cursor, key.c_str());
    ret = metadata_cursor->search(metadata_cursor);
    if (ret != 0)
    {
        fprintf(stderr, "Failed to retrieve metadata for the key %s", key.c_str());
        exit(-1);
    }

    const char *value;
    ret = metadata_cursor->get_value(metadata_cursor, &value);
    metadata_cursor->close(metadata_cursor);

    return string(value);
}

/**
 * @brief This function is used to check if a node identified by node_id exists
 * in the node table.
 *
 * @param node_id the node_id to be searched.
 * @return true if the node is found; false otherwise.
 */
bool AdjList::has_node(int node_id)
{
    int ret = 0;
    if (this->node_cursor == NULL)
    {
        ret = _get_table_cursor(NODE_TABLE, &(this->node_cursor), false);
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

int AdjList::get_num_nodes()
{
    int ret = 0;
    WT_CURSOR *cursor = nullptr;
    ret = _get_table_cursor(NODE_TABLE, &cursor, false);

    int count = 0;
    while ((ret = cursor->next(cursor)) == 0)
    {
        count += 1;
    }
    // cursor->close(cursor);
    return count;
}

int AdjList::get_num_edges()
{
    int ret = 0;
    WT_CURSOR *cursor = nullptr;
    ret = _get_table_cursor(EDGE_TABLE, &cursor, false);

    int count = 0;
    while ((ret = cursor->next(cursor)) == 0)
    {
        count += 1;
    }
    // cursor->close(cursor);
    return count;
}

//#if 0
/**
 * @brief This private function inserts metadata values into the metadata
 * table. The fields are self explanatory.
 * Same from standard_graph implementation.
 */
void AdjList::insert_metadata(const string &key, const char *value)
{
    int ret = 0;

    if ((ret = _get_table_cursor(METADATA, &metadata_cursor, false)) != 0)
    {
        fprintf(stderr, "Failed to create cursor to the metadata table.");
        exit(-1);
    }

    this->metadata_cursor->set_key(metadata_cursor, key.c_str());
    this->metadata_cursor->set_value(metadata_cursor, value);
    if ((ret = this->metadata_cursor->insert(metadata_cursor)) != 0)
    {
        fprintf(stderr, "failed to insert metadata for key %s", key.c_str());
        // TODO(puneet): Maybe create a GraphException?
    }
    // this->metadata_cursor->close(metadata_cursor);
}
//#endif

void AdjList::create_new_graph()
{
    int ret;
    // Create new directory for WT DB
    std::string dirname = opts.db_dir + "/" + opts.db_name;
    CommonUtil::create_dir(dirname);

    // open connection to WT
    if (CommonUtil::open_connection(const_cast<char *>(dirname.c_str()), &conn) < 0)
    {
        exit(-1);
    };

    // Open a session handle for the database
    if (CommonUtil::open_session(conn, &session) != 0)
    {
        exit(-1);
    }

    // Initialize edge ID for the edge table
    // AdjList has no edge id in the edge table but we are using the same structure as used by Standardgraph. So, the edge_id value will be -999 for all edges in the AdjList implementation.
    edge_id = -999; // edge_id is uninterpreted

    // Set up the node table
    // The node entry is of the form: <id>,<in_degree>,<out_degree>
    // If the graph is opts.read_optimized, add columns and format for in/out degrees
    if (opts.read_optimize)
    {
        node_columns.push_back(IN_DEGREE);
        node_columns.push_back(OUT_DEGREE);
        node_value_format = "II";
    }
    else
    {
        node_columns.push_back("na"); // have to do this because the column count must match.
        node_value_format = "s";      // 1 byte fixed length char[] to hold ""
    }
    // Now Create the Node Table
    //! What happens when the table is not read-optimized? I store "" <-ok?

    CommonUtil::set_table(session, NODE_TABLE, node_columns, node_key_format,
                          node_value_format);

    // ******** Now set up the Edge Table     **************
    // Edge Column Format : <src><dst><weight>
    // Now prepare the edge value format. starts with II for src,dst. Add another I if weighted
    if (opts.is_weighted)
    {
        edge_columns.push_back(WEIGHT);
        edge_value_format += "I";
    }
    else
    {
        edge_columns.push_back("NA");
        edge_value_format += "b"; // uses 8 bits, which is the smallest possible value (?) other than x -- padded byte which I don't fully understand
        // TODO: check if this is referrred anywhere in the unweighted code path
    }

    // Create edge table
    CommonUtil::set_table(session, EDGE_TABLE, edge_columns,
                          edge_key_format, edge_value_format);

    // Create adjlist_in_edges table
    CommonUtil::set_table(session, IN_ADJLIST, in_adjlist_columns,
                          adjlist_key_format, adjlist_value_format);

    // Create adjlist_out_edges table
    CommonUtil::set_table(session, OUT_ADJLIST, out_adjlist_columns,
                          adjlist_key_format, adjlist_value_format);

    /* Now doing the metadata table creation.
     function This table stores the graph metadata
     value_format:string (S)
     key_format: string (S)
  */
    string metadata_table_name = "table:" + string(METADATA);
    if ((ret = session->create(session, metadata_table_name.c_str(),
                               "key_format=S,value_format=S")) > 0)
    {
        fprintf(stderr, "Failed to create the metadata table ");
    }

    if ((ret = _get_table_cursor(METADATA, &metadata_cursor, false)) != 0)
    {
        fprintf(stderr, "Failed to create cursor to the metadata table.");
        exit(-1);
    }

    // DB_NAME
    string db_name_fmt;
    insert_metadata(DB_NAME, const_cast<char *>(opts.db_name.c_str()));

    // DB_DIR
    insert_metadata(DB_DIR, const_cast<char *>(opts.db_dir.c_str()));

    // READ_OPTIMIZE
    string read_optimized_str = opts.read_optimize ? "true" : "false";
    insert_metadata(READ_OPTIMIZE, const_cast<char *>(read_optimized_str.c_str()));

    // IS_DIRECTED
    string is_directed_str = opts.is_directed ? "true" : "false";
    insert_metadata(IS_DIRECTED, const_cast<char *>(is_directed_str.c_str()));

    // is_weighted
    string is_weighted_str = opts.is_weighted ? "true" : "false";
    insert_metadata(IS_WEIGHTED, const_cast<char *>(is_weighted_str.c_str()));
    //#endif

    // this->metadata_cursor->close(this->metadata_cursor);
}

void AdjList::__restore_from_db(string dbname)
{
    int ret = CommonUtil::open_connection(const_cast<char *>(dbname.c_str()), &conn);
    ret = CommonUtil::open_session(conn, &session);
    // Initialize all cursors
    init_cursors();
    WT_CURSOR *cursor = nullptr;
    const char *key, *value;
    ret = _get_table_cursor(METADATA, &cursor, false);

    while ((ret = cursor->next(cursor)) == 0)
    {
        ret = cursor->get_key(cursor, &key);
        ret = cursor->get_value(cursor, &value);

        if (strcmp(key, DB_DIR.c_str()) == 0)
        {

            this->opts.db_dir = value; // CommonUtil::unpack_string_wt(value, this->session);
        }
        else if (strcmp(key, DB_NAME.c_str()) == 0)
        {

            this->opts.db_name = value; // CommonUtil::unpack_string_wt(value, this->session);
        }
        else if (strcmp(key, READ_OPTIMIZE.c_str()) == 0)
        {
            if (strcmp(value, "true") == 0)
            {
                this->opts.read_optimize = true;
            }
            else
            {
                this->opts.read_optimize = false;
            }
        }
        else if (strcmp(key, IS_DIRECTED.c_str()) == 0)
        {
            if (strcmp(value, "true") == 0)
            {
                this->opts.is_directed = true;
            }
            else
            {
                this->opts.is_directed = false;
            }
        }
        else if (strcmp(key, IS_WEIGHTED.c_str()) == 0)
        {
            if (strcmp(value, "true") == 0)
            {
                this->opts.is_weighted = true;
            }
            else
            {
                this->opts.is_weighted = false;
            }
        }
    }
    // cursor->close(cursor);
}
/**
 * @brief This is the generic function to get a cursor on the table
 *
 * @param table This is the table name for which the cursor is needed.
 * @param cursor This is the pointer that will hold the set cursor.
 * @param is_random This is a bool value to indicate if the cursor must be
 * random.
 * @return 0 if the cursor could be set
 */
int AdjList::_get_table_cursor(const string &table, WT_CURSOR **cursor,
                               bool is_random)
{
    std::string table_name = "table:" + table;
    const char *config = NULL;
    if (is_random)
    {
        config = "next_random=true";
    }

    if (int ret = session->open_cursor(session, table_name.c_str(), NULL, config, cursor) != 0) //! APT: Check for cursor close.
    {
        fprintf(stderr, "Failed to get table cursor to %s\n", table_name.c_str());
        return ret;
    }
    return 0;
}

void AdjList::init_cursors()
{
    int ret = 0;
    // node_cursor initialization
    if (node_cursor == nullptr)
    {
        ret = _get_table_cursor(NODE_TABLE, &node_cursor, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a cursor to the NODE TABLE" + string(wiredtiger_strerror(ret)));
        }
    }
    // out_adjlist_cursors initialization
    if (out_adjlist_cursor == nullptr)
    {
        ret = _get_table_cursor(OUT_ADJLIST, &out_adjlist_cursor, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a cursor to the out adjlist table : " + string(wiredtiger_strerror(ret)));
        }
    }
    // in_adjlist_cursor initialization
    if (in_adjlist_cursor == nullptr)
    {
        ret = _get_table_cursor(IN_ADJLIST, &in_adjlist_cursor, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a cursor to the in_Adjlist table: " + string(wiredtiger_strerror(ret)));
        }
    }
}

/**
 * @brief The information that gets persisted to WT is of the form:
 * <node_id>,in_degree,out_degree.
 * in_degree and out_degree are persisted if opts.read_optimize is true. ints
 *
 *
 * @param to_insert
 */
// todo:add overloaded function that accepts an adjlist and just passes it to add_adjlist.
void AdjList::add_node(node to_insert)
{
    int ret = 0;
    WT_CURSOR *in_adj_cur, *out_adj_cur, *n_cur = nullptr;

    in_adj_cur = get_in_adjlist_cursor();
    out_adj_cur = get_out_adjlist_cursor();
    n_cur = get_node_cursor();

    n_cur->set_key(n_cur, to_insert.id);

    if (opts.read_optimize)
    {
        n_cur->set_value(n_cur, 0, 0);
    }
    else
    {
        n_cur->set_value(n_cur, "");
    }

    ret = n_cur->insert(n_cur);

    if (ret != 0)
    {
        throw GraphException("Failed to add node_id" +
                             std::to_string(to_insert.id));
    }
    // Now add the adjlist entires
    add_adjlist(in_adj_cur, to_insert.id);
    add_adjlist(out_adj_cur, to_insert.id);
}

void AdjList::add_node(int to_insert, std::vector<int> &inlist, std::vector<int> &outlist)
{
    int ret = 0;
    WT_CURSOR *in_adj_cur, *out_adj_cur, *n_cur = nullptr;

    in_adj_cur = get_in_adjlist_cursor();
    out_adj_cur = get_out_adjlist_cursor();
    n_cur = get_node_cursor();

    n_cur->set_key(n_cur, to_insert);

    if (opts.read_optimize)
    {
        n_cur->set_value(n_cur, inlist.size(), outlist.size());
    }
    else
    {
        n_cur->set_value(n_cur, "");
    }

    ret = n_cur->insert(n_cur);

    if (ret != 0)
    {
        throw GraphException("Failed to add node_id" +
                             std::to_string(to_insert));
    }
    // Now add the adjlist entires
    add_adjlist(in_adj_cur, to_insert, inlist);
    add_adjlist(out_adj_cur, to_insert, outlist);
}

/**
 * @brief Add a record for the node_id in the in or out adjlist,
 * as pointed by the cursor.
 * if the node_id record already exists then reset it with an empty list.
 **/
// TODO:create an overloaded function that accepts a fully formed in adj list and adds it directly.
void AdjList::add_adjlist(WT_CURSOR *cursor, int node_id)
{
    int ret = 0;
    // Check if the cursor is not NULL, else throw exception
    if (cursor == NULL)
    {
        throw GraphException("Uninitiated Cursor passed to add_adjlist call");
    }

    cursor->set_key(cursor, node_id);

    // Now, initialize the in/out degree to 0 and adjlist to empty list
    WT_ITEM item = {.data = {}, .size = 0}; // todo: check
    cursor->set_value(cursor, 0, &item);    // serialize the vector and send ""
    ret = cursor->insert(cursor);
    if (ret != 0)
    {
        throw GraphException("Failed to add node_id" +
                             std::to_string(node_id));
    }
}

void AdjList::add_adjlist(WT_CURSOR *cursor, int node_id, std::vector<int> &list)
{
    int ret = 0;
    // Check if the cursor is not NULL, else throw exception
    if (cursor == NULL)
    {
        throw GraphException("Uninitiated Cursor passed to add_adjlist call");
    }

    cursor->set_key(cursor, node_id);

    // Now, initialize the in/out degree to 0 and adjlist to empty list
    WT_ITEM item;
    item.data = CommonUtil::pack_int_vector_wti(session, list, &item.size);
    cursor->set_value(cursor, list.size(), &item); // serialize the vector and send ""

    ret = cursor->insert(cursor);
    if (ret != 0)
    {
        throw GraphException("Failed to add node_id" +
                             std::to_string(node_id));
    }
}

/**
 * @brief Delete the record of the node_id in the in or out
 * adjlist as pointed by the cursor.
 **/
void AdjList::delete_adjlist(WT_CURSOR *cursor, int node_id)
{
    int ret = 0;
    // Check if the cursor is not NULL, else throw exception
    if (cursor == nullptr)
    {
        throw GraphException("Uninitiated Cursor passed to delete_adjlist");
    }

    cursor->set_key(cursor, node_id);
    ret = cursor->remove(cursor);

    if (ret != 0)
    {
        throw GraphException("Could not delete node with ID " + to_string(node_id));
    }
}

void AdjList::add_edge(edge to_insert, bool is_bulk_insert)
{
    int ret = 0;
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

    WT_CURSOR *cursor = get_edge_cursor();

    // We don't need to check if the edge exists already -- overwrite it regardless
    to_insert.id = -1; // uninterpreted
    cursor->set_key(cursor, to_insert.src_id, to_insert.dst_id);
    // cout << "New Edge ID inserted" << endl;

    if (opts.is_weighted)
    {
        cursor->set_value(cursor, to_insert.edge_weight);
    }
    else
    {
        cursor->set_value(cursor, 0);
    }
    ret = cursor->insert(cursor);
    if (ret != 0)
    {
        throw GraphException("Failed to insert edge (" +
                             to_string(to_insert.src_id) + "," +
                             to_string(to_insert.dst_id));
    }

    // insert the reverse edge if undirected
    if (!opts.is_directed)
    {
        cursor->set_key(cursor, to_insert.dst_id, to_insert.src_id);
        if (opts.is_weighted)
        {
            cursor->set_value(cursor, to_insert.edge_weight);
        }
        else
        {
            cursor->set_value(cursor, 0);
        }
        cursor->reset(cursor);
    }
    // cursor->close(cursor);
    if (is_bulk_insert)
    {
        return; // we have already added adjlists while adding nodes.
    }
    //! This assumes that there are no duplicate edges.
    add_to_adjlists(out_adjlist_cursor, to_insert.src_id, to_insert.dst_id);
    add_to_adjlists(in_adjlist_cursor, to_insert.dst_id, to_insert.src_id);
    if (!opts.is_directed)
    {
        add_to_adjlists(out_adjlist_cursor, to_insert.dst_id, to_insert.src_id);
        add_to_adjlists(in_adjlist_cursor, to_insert.src_id, to_insert.dst_id);
    }

    // If opts.read_optimized is true, we update in/out degreees in the node table.
    if (this->opts.read_optimize)
    {
        // update in/out degrees for src node in NODE_TABLE
        cursor = get_node_cursor();

        cursor->set_key(cursor, to_insert.src_id);
        ret = cursor->search(cursor);
        if (ret != 0)
        {
            throw GraphException("Could not find " + to_string(to_insert.src_id) + " in the Node Table. At " + to_string(__LINE__));
        }
        // TODO: do a check
        node found = {.id = to_insert.src_id};
        CommonUtil::__record_to_node(cursor, &found, opts.read_optimize);
        // found.id = to_insert.src_id;
        found.out_degree = found.out_degree + 1;
        update_node_degree(cursor, found.id, found.in_degree,
                           found.out_degree); //! pass the cursor to this function

        // update in/out degrees for the dst node in the NODE_TABLE
        cursor->reset(cursor);
        cursor->set_key(cursor, to_insert.dst_id);
        cursor->search(cursor);
        found = {.id = to_insert.dst_id, .in_degree = 0, .out_degree = 0};
        CommonUtil::__record_to_node(cursor, &found, opts.read_optimize);
        found.id = to_insert.dst_id;
        found.in_degree = found.in_degree + 1;
        update_node_degree(cursor, found.id, found.in_degree, found.out_degree);
        // cursor->close(cursor);
    }
}

node AdjList::get_random_node()
{
    node found = {0};
    WT_CURSOR *cursor;
    int ret = _get_table_cursor(NODE_TABLE, &cursor, true);
    if (ret != 0)
    {
        throw GraphException("could not get a random cursor to the node table");
    }
    ret = cursor->next(cursor);
    if (ret != 0)
    {
        throw GraphException("Could not seek a random node in the table");
    }

    CommonUtil::__record_to_node(cursor, &found, opts.read_optimize);
    cursor->get_key(cursor, &found.id);
    return found;
}

void AdjList::close()
{
    // TODO:check if there is any state that needs to be saved in the metadata
    // table.
    CommonUtil::close_connection(conn);
}

/**
 * @brief Deletes the node_id from the node table and also removes all of its
 * related edges and adjacency list, and node_id's in and out adj lists.
 *
 * @param node_id the node to be removed
 */
void AdjList::delete_node(int node_id)
{
    // Delete Edges
    delete_related_edges_and_adjlists(node_id);

    WT_CURSOR *cursor;
    int ret = _get_table_cursor(NODE_TABLE, &cursor, false);
    if (ret != 0)
    {
        throw GraphException("Could not get a cursor to the NODE table");
    }
    cursor->set_key(cursor, node_id);
    ret = cursor->remove(cursor);
    if (ret != 0)
    {
        throw GraphException("failed to delete node with ID " + std::to_string(node_id));
    }
    cursor->close(cursor);

    // delete IN_ADJLIST entries
    cursor = get_in_adjlist_cursor();
    delete_adjlist(cursor, node_id);
    // cursor->close(cursor);

    // delete OUT_ADJLIST entrties
    cursor = get_out_adjlist_cursor();
    delete_adjlist(cursor, node_id);
    // cursor->close(cursor);
}

/**
 * @brief Get the in degree for the node provided
 *
 * @param node_id ID for which in_degree is required
 * @return int in degree of the node node_id
 */
int AdjList::get_in_degree(int node_id)
{
    int ret = 0;
    WT_CURSOR *cursor;
    if (opts.read_optimize)
    {
        cursor = get_node_cursor();
        cursor->set_key(cursor, node_id);
        ret = cursor->search(cursor);
        if (ret != 0)
        {
            throw GraphException("Could not find a node with ID " + std::to_string(node_id));
        }
        node found{.id = node_id, .in_degree = 0, .out_degree = 0};
        CommonUtil::__record_to_node(cursor, &found, opts.read_optimize);
        // cursor->close(cursor);
        return found.in_degree;
    }
    else
    {
        cursor = get_in_adjlist_cursor();
        cursor->set_key(cursor, node_id);
        ret = cursor->search(cursor);
        if (ret != 0)
        {
            throw GraphException("Could not find node with ID" + std::to_string(node_id) + " in the adjlist");
        }
        adjlist in_edges;
        in_edges.node_id = node_id;
        CommonUtil::__record_to_adjlist(session, cursor, &in_edges);
        // cursor->close(cursor);
        return in_edges.degree;
    }
}

/**
 * @brief Get the out degree for the node requested
 *
 * @param node_id The ID of the node for which the degree is sought
 * @return int the node degree for the node with ID node_id.
 */
int AdjList::get_out_degree(int node_id)
{
    int ret = 0;
    WT_CURSOR *cursor = get_node_cursor();
    cursor->reset(cursor);
    if (opts.read_optimize)
    {
        cursor->set_key(cursor, node_id);
        ret = cursor->search(cursor);
        if (ret != 0)
        {
            throw GraphException("Could not find a node with ID " + std::to_string(node_id));
        }
        node found{.id = node_id, .in_degree = 0, .out_degree = 0};
        CommonUtil::__record_to_node(cursor, &found, opts.read_optimize);
        // cursor->close(cursor);
        return found.out_degree;
    }

    else
    {
        cursor = get_out_adjlist_cursor();
        cursor->set_key(cursor, node_id);
        ret = cursor->search(cursor);
        if (ret != 0)
        {
            throw GraphException("Could not find a node with ID " + std::to_string(node_id) + " in the adjlist");
        }
        adjlist out_edges;
        out_edges.node_id = node_id;
        CommonUtil::__record_to_adjlist(session, cursor, &out_edges);
        // cursor->close(cursor);
        return out_edges.degree;
    }
}

/**
 * @brief Get all nodes in the graph
 *
 * @return std::vector<node> vector of all nodes
 */
std::vector<node> AdjList::get_nodes()
{
    std::vector<node> nodelist;
    WT_CURSOR *n_cursor = get_node_cursor();
    int ret = 0;
    node found;
    while ((ret = n_cursor->next(n_cursor) == 0))
    {
        CommonUtil::__record_to_node(n_cursor, &found, opts.read_optimize);
        n_cursor->get_key(n_cursor, &found.id);
        nodelist.push_back(found);
    }
    return nodelist;
}

/**
 * @brief Get the node identified by node ID
 *
 * @param node_id the Node ID
 * @return node the node struct containing the node
 */
node AdjList::get_node(int node_id)
{
    int ret = 0;
    WT_CURSOR *n_cursor = get_node_cursor();
    n_cursor->set_key(n_cursor, node_id);
    ret = n_cursor->search(n_cursor);
    node found = {0};
    if (ret == 0)
    {
        CommonUtil::__record_to_node(n_cursor, &found, opts.read_optimize);
        found.id = node_id;
    }
    return found;
}

/**
 * @brief Get a list of all the edges in the graph
 *
 * @return std::vector<edge> Vector containing all the edges in the graph
 */
std::vector<edge> AdjList::get_edges()
{
    std::vector<edge> edgelist;
    WT_CURSOR *e_cursor = get_edge_cursor();
    while (e_cursor->next(e_cursor) == 0)
    {
        edge found;
        e_cursor->get_key(e_cursor, &found.src_id, &found.dst_id);
        if (opts.is_weighted)
        {
            CommonUtil::__record_to_edge(e_cursor, &found);
        }

        edgelist.push_back(found);
    }
    return edgelist;
}

/**
 * @brief Get the edge identified by (src_id, dst_id)
 *
 * @param src_id source id
 * @param dst_id destination id
 * @return edge edge identified by (src,dst) pair
 */
edge AdjList::get_edge(int src_id, int dst_id)
{

    edge found = {0};
    found.src_id = src_id;
    found.dst_id = dst_id;
    if (!opts.is_weighted)
    {
        return found;
    }
    int ret = 0;

    WT_CURSOR *e_cursor = get_edge_cursor();
    e_cursor->set_key(e_cursor, src_id, dst_id);
    ret = e_cursor->search(e_cursor);
    if (ret != 0)
    {
        found = {0};
        return found;
    }
    else
    {
        CommonUtil::__record_to_edge(e_cursor, &found);
        return found;
    }
}

/**
 * @brief Check if an edge (srd_id, dst_id) exists in the graph
 *
 * @param src_id source id
 * @param dst_id destination id
 * @return true if the edge exists
 * @return false if the edge does not exist
 */
bool AdjList::has_edge(int src_id, int dst_id)
{
    int ret = 0;

    WT_CURSOR *e_cursor = get_edge_cursor();
    e_cursor->set_key(e_cursor, src_id, dst_id);
    ret = e_cursor->search(e_cursor);
    return (ret == 0); // true if found :)
}

/**
 * @brief Get the weight associated with an edge between src_id and dst_id
 *
 * @param src_id source node ID
 * @param dst_id Destination node ID
 * @return int the weigght associated with the edge
 * @throws GraphExpection If the graph is not weighted or if an edge is not found
 */
int AdjList::get_edge_weight(int src_id, int dst_id)
{
    if (!this->opts.is_weighted)
    {
        throw GraphException("Aborting. Trying to get weight for an unweighted graph");
    }

    WT_CURSOR *e_cursor = get_edge_cursor();
    e_cursor->set_key(e_cursor, src_id, dst_id);
    int ret = e_cursor->search(e_cursor);
    if (ret != 0)
    {
        throw GraphException("There is no edge between " + std::to_string(src_id) + " and " + std::to_string(dst_id));
    }
    else
    {
        edge found;
        CommonUtil::__record_to_edge(e_cursor, &found);
        return found.edge_weight;
    }
}

/**
 * @brief update the in/out degree for the node identified by node_id
 *
 * @param cursor Cursor to the node table
 * @param node_id node for which in/out degrees are being modified
 * @param in_degree new in_degree
 * @param out_degree new out_degree
 * @throws GraphException if the node degree could not be updated
 *
 */
void AdjList::update_node_degree(WT_CURSOR *cursor, int node_id, int in_degree, int out_degree)
{
    cursor->set_key(cursor, node_id);
    cursor->set_value(cursor, in_degree, out_degree);
    int ret = cursor->insert(cursor);
    if (ret != 0)
    {
        throw GraphException("Could not update degrees for node with ID " + std::to_string(node_id));
    }
}

/**
 * @brief Get a list of nodes that have an incoming edge from node_id (Are an
 * out node for node_id)
 *
 * @param node_id The node which is being queried.
 * @return std::vector<node> The vector of out nodes
 * @throws GraphException if could not acquire cursors to node table or the
 * out_adjlist tables; if a dst node was found in the adj_list for node_id but
 * does not exist in the node table
 */
std::vector<node> AdjList::get_out_nodes(int node_id)
{
    std::vector<node> out_nodes;
    WT_CURSOR *outadj_cursor = get_out_adjlist_cursor();
    WT_CURSOR *n_cursor = get_node_cursor();

    std::vector<int> adjlist = get_adjlist(outadj_cursor, node_id);

    for (int dst_id : adjlist)
    {
        n_cursor->set_key(n_cursor, dst_id);
        node found = {0};
        if (n_cursor->search(n_cursor) == 0)
        {
            CommonUtil::__record_to_node(n_cursor, &found, opts.read_optimize);
            found.id = dst_id;
            out_nodes.push_back(found);
        }
        else
        {
            throw GraphException("node with ID " + std::to_string(dst_id) + "found in adjlist for " + std::to_string(node_id) + " but not in node table");
        }
    }
    return out_nodes;
}

/**
 * @brief Return a vector of all edges that have node_id as their source node.
 *
 * @param node_id The source node
 * @return std::vector<edge> the vector of all edges which have node_id as src
 */
std::vector<edge> AdjList::get_out_edges(int node_id)
{
    int ret = 0;
    std::vector<edge> out_edges;
    std::vector<int> dst_nodes;
    WT_CURSOR *out_adj_cur = get_out_adjlist_cursor();
    WT_CURSOR *e_cur = get_edge_cursor();

    dst_nodes = get_adjlist(out_adj_cur, node_id);

    for (int dst : dst_nodes)
    {
        e_cur->set_key(e_cur, node_id, dst);
        ret = e_cur->search(e_cur);
        if (ret != 0)
        {
            throw GraphException("expected to find an edge from " + to_string(node_id) + " to " + to_string(dst) + " but didn't");
        }
        edge found;
        found.src_id = node_id;
        found.dst_id = dst;
        CommonUtil::__record_to_edge(e_cur, &found);
        out_edges.push_back(found);
    }
    return out_edges;
}

/**
 * @brief Get a list of nodes that have an outgoing edge to node_id (are an in
 * node for node_id)
 *
 * @param node_id The node which is being queried.
 * @return std::vector<node> the vector of in nodes
 * @throws GraphException if could not acquire cursors to node table or the
 * in_adjlist tables; if a src node was found in the in_adj_list for node_id but
 * does not exist in the node table
 */
std::vector<node> AdjList::get_in_nodes(int node_id)
{
    std::vector<node> in_nodes;
    WT_CURSOR *inadj_cursor = get_in_adjlist_cursor();
    WT_CURSOR *n_cursor = get_node_cursor();

    std::vector<int> adjlist = get_adjlist(inadj_cursor, node_id);
    node found = {0};
    for (int src_id : adjlist)
    {
        n_cursor->set_key(n_cursor, src_id);
        if (n_cursor->search(n_cursor) == 0)
        {
            CommonUtil::__record_to_node(n_cursor, &found, opts.read_optimize);
            found.id = src_id;
            in_nodes.push_back(found);
        }
        else
        {
            throw GraphException("node with ID " + std::to_string(src_id) + "found in adjlist for " + std::to_string(node_id) + " but not in node table");
        }
    }
    return in_nodes;
}

/**
 * @brief Return a vector of all edges that have node_id as their dst node
 *
 * @param node_id the destination node
 * @return std::vector<edge> the vector of all edges that have node_id as dst
 */
std::vector<edge> AdjList::get_in_edges(int node_id)
{
    std::vector<edge> in_edges;
    std::vector<int> src_nodes;
    int ret = 0;
    WT_CURSOR *e_cur = get_edge_cursor();
    WT_CURSOR *in_adj_cur = get_in_adjlist_cursor();

    src_nodes = get_adjlist(in_adj_cur, node_id);

    for (int src : src_nodes)
    {
        e_cur->set_key(e_cur, src, node_id);
        ret = e_cur->search(e_cur);
        if (ret != 0)
        {
            throw GraphException("Expected to find an edge from " + to_string(src) + " to " + to_string(node_id) + " but didn't");
        }
        edge found;
        found.src_id = src;
        found.dst_id = node_id;
        CommonUtil::__record_to_edge(e_cur, &found);
        in_edges.push_back(found);
    }
    return in_edges;
}

/**
 * @brief Delete an edge identified by (src_id, dst_id)
 *
 * @param src_id source node ID
 * @param dst_id Destination node ID
 * @returns void
 * @throw GraphException if could not delete the edge or if the node degree
 * could not be updated.
 */
void AdjList::delete_edge(int src_id, int dst_id)
{
    // Delete (src_id, dst_id) from edge table
    int ret = 0;
    WT_CURSOR *e_cursor = get_edge_cursor();
    e_cursor->set_key(e_cursor, src_id, dst_id);
    if ((e_cursor->remove(e_cursor)) != 0)
    {
        throw GraphException("Could not delete edge (" + std::to_string(src_id) + ", " + std::to_string(dst_id) + ")");
    }

    // delete (dst_id, src_id) from edge table if undirected
    if (!opts.is_directed)
    {
        e_cursor->set_key(e_cursor, dst_id, src_id);
        if ((e_cursor->remove(e_cursor)) != 0)
        {
            throw GraphException("Could not delete edge (" + std::to_string(dst_id) + ", " + std::to_string(src_id) + ")");
        }
    }
    out_adjlist_cursor->reset(out_adjlist_cursor);
    in_adjlist_cursor->reset(in_adjlist_cursor);
    // remove from adjacency lists
    delete_from_adjlists(out_adjlist_cursor, src_id, dst_id);
    delete_from_adjlists(in_adjlist_cursor, dst_id, src_id);

    // remove reverse from adj lists if undirected
    if (!opts.is_directed)
    {
        delete_from_adjlists(in_adjlist_cursor, src_id, dst_id);
        delete_from_adjlists(out_adjlist_cursor, dst_id, src_id);
    }

    // if opts.read_optimized -- update in/out degrees in the node table
    if (opts.read_optimize)
    {
        WT_CURSOR *n_cursor = get_node_cursor();
        if (n_cursor == nullptr)
        {
            ret = _get_table_cursor(NODE_TABLE, &n_cursor, false);
            if (ret != 0)
            {
                throw GraphException("Could not get a cursor to the node table");
            }
        }
        n_cursor->set_key(n_cursor, src_id);
        ret = n_cursor->search(n_cursor);
        if (ret != 0)
        {
            throw GraphException("Could not find " + std::to_string(src_id) + " in the node table");
        }
        node found = {0};
        CommonUtil::__record_to_node(n_cursor, &found, opts.read_optimize);
        found.id = src_id;
        found.out_degree--;
        if (!opts.is_directed)
        {
            found.in_degree--;
        }
        CommonUtil::__node_to_record(n_cursor, found, opts.read_optimize);

        n_cursor->set_key(n_cursor, dst_id);
        ret = n_cursor->search(n_cursor);
        if (ret != 0)
        {
            throw GraphException("Could not find " + std::to_string(dst_id) + " in the node table");
        }
        CommonUtil::__record_to_node(n_cursor, &found, opts.read_optimize);
        found.id = dst_id;
        found.in_degree--;
        if (!opts.is_directed)
        {
            found.out_degree--;
        }
        CommonUtil::__node_to_record(n_cursor, found, opts.read_optimize);
    }
}

/**
 * @brief For the edge identified with (src_id, dst_id) update the edge weight
 *
 * @param src_id source id
 * @param dst_id dst ID
 * @param edge_weight new edge weight
 * @throws GraphException if trying to update weight for an unweighted graph, if
 * the edge cursor could not be found, or if the update operation fails.
 */
void AdjList::update_edge_weight(int src_id, int dst_id, int edge_weight)
{
    if (!opts.is_weighted)
    {
        throw GraphException("Trying to insert weight for an unweighted graph");
    }
    int ret = 0;
    if (edge_cursor == nullptr)
    {
        ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a cursor to the edge table");
        }
    }
    edge_cursor->set_key(edge_cursor, src_id, dst_id);
    edge_cursor->set_value(edge_cursor, edge_weight);
    ret = edge_cursor->insert(edge_cursor);
    if (ret != 0)
    {
        throw GraphException("Could not update edge weight for edge (" + std::to_string(src_id) + ", " + std::to_string(dst_id) + ")");
    }
}

/**
 * @brief Return the adjacency list for the node using the cursor provided. The
 * caller passes the cursor to the table from which the adjlist is required to
 * be read.
 *
 * @param cursor A cursor to the in_list or out_list table.
 * @param node_id Node ID for which the adjlist is to be read from cursor.
 * @return std::vector<int> The AdjList for the node.
 */
std::vector<int> AdjList::get_adjlist(WT_CURSOR *cursor, int node_id)
{
    int ret;
    adjlist adj_list;

    cursor->set_key(cursor, node_id);
    ret = cursor->search(cursor);
    if (ret != 0)
    {
        throw GraphException("Could not find " + std::to_string(node_id) + " in the AdjList");
    }

    // !APT: Check with puneet
    // We have the entire node we only need the list how to assign it without knowing which table is it in or out?

    CommonUtil::__record_to_adjlist(session, cursor, &adj_list);
    return adj_list.edgelist;
}

void AdjList::add_to_adjlists(WT_CURSOR *cursor, int node_id, int to_insert)
{
    // Not checking for directional or undirectional that would be taken care by the caller.

    int ret;

    cursor->set_key(cursor, node_id);
    ret = cursor->search(cursor);
    if (ret != 0)
    {
        throw GraphException("Could not find " + std::to_string(node_id) + " in the AdjList");
    }

    // ! APT: Check below lines with Puneet and we need __adjlist_to_record, correct? Verify with Puneet!
    adjlist found;
    found.node_id = node_id;
    CommonUtil::__record_to_adjlist(session, cursor, &found); //<-- This works just fine.
    found.edgelist.push_back(to_insert);
    found.degree += 1;

    CommonUtil::__adjlist_to_record(session, cursor, found);
}

void AdjList::delete_from_adjlists(WT_CURSOR *cursor, int node_id, int to_delete)
{
    // Not checking for directional or undirectional that would be taken care by the caller.

    int ret;

    cursor->set_key(cursor, node_id);
    ret = cursor->search(cursor);
    if (ret != 0)
    {
        throw GraphException("Could not find " + std::to_string(node_id) + " in the AdjList");
    }

    adjlist found;
    found.node_id = node_id;
    CommonUtil::__record_to_adjlist(session, cursor, &found);
    for (int i = 0; i < found.edgelist.size(); i++)
    {
        if (found.edgelist.at(i) == to_delete)
        {
            found.edgelist.erase(found.edgelist.begin() + i);
        }
    }

    found.degree -= 1; // FIXME: There is something not right. The degree is going negative.

    CommonUtil::__adjlist_to_record(session, cursor, found);
}

void AdjList::delete_node_from_adjlists(int node_id)
{
    // We need to delete the node from both tables
    // and go through all the adjlist values, iterate over its edgelist and correspondingly delete the node_id from the edgelist of its neighbors.
    std::vector<int> in_edgelist;
    std::vector<int> out_edgelist;

    WT_CURSOR *in_cursor = nullptr;
    int ret = _get_table_cursor(IN_ADJLIST, &in_cursor, false);
    in_cursor->set_key(in_cursor, node_id);
    ret = in_cursor->search(in_cursor);
    if (ret != 0)
    {
        throw GraphException("Could not find " + std::to_string(node_id) + " in the AdjList Table");
    }

    in_edgelist = get_adjlist(in_cursor, node_id);
    // Iterate through the in_edgelist in the OUT_ADJLIST and delete the node_id from its neighbors

    WT_CURSOR *out_cursor = nullptr;
    ret = _get_table_cursor(OUT_ADJLIST, &out_cursor, false);

    for (int neighbor : in_edgelist)
    {
        delete_from_adjlists(out_cursor, neighbor, node_id);
    }

    // Now, get the out_edgelist from the OUTLIST Table to delete from the IN_Table

    out_cursor->set_key(out_cursor, node_id);
    ret = out_cursor->search(out_cursor);
    if (ret != 0)
    {
        throw GraphException("Could not find " + std::to_string(node_id) + " in the AdjList Table");
    }

    out_edgelist = get_adjlist(out_cursor, node_id);

    // Now, go to IN_TABLE and delete node_id from its tables in edgelist
    for (int neighbor : out_edgelist)
    {
        delete_from_adjlists(in_cursor, neighbor, node_id);
    }

    // Now, remove the node from both the tables
    out_cursor->set_key(out_cursor, node_id);
    ret = out_cursor->remove(out_cursor);

    if (ret != 0)
    {
        throw GraphException("Could not delete node with ID " + to_string(node_id) + " from the OUT_ADJLIST");
    }

    in_cursor->set_key(in_cursor, node_id);
    ret = in_cursor->remove(in_cursor);

    if (ret != 0)
    {
        throw GraphException("Could not delete node with ID " + to_string(node_id) + " from the IN_ADJLIST");
    }
}

/**
 * @brief Delete all edges related to the node node_id. This means deleting
 * edges that have node_id as either src or dst node. Entries are deleted from
 * the edge and the adjlist tables.

 * @param node_id node ID which has to be deleted from edge and adjlist tables
 */
void AdjList::delete_related_edges_and_adjlists(int node_id)
{
    // initialize all the cursors
    WT_CURSOR *e_cursor, *inadj_cursor, *outadj_cursor = nullptr;
    e_cursor = get_edge_cursor();
    inadj_cursor = get_in_adjlist_cursor();
    outadj_cursor = get_out_adjlist_cursor();

    // Get outgoing nodes
    std::vector<int> out_nodes = get_adjlist(outadj_cursor, node_id);

    // Get incoming nodes
    std::vector<int> in_nodes = get_adjlist(inadj_cursor, node_id);
    for (int dst : out_nodes)
    {
        delete_edge(node_id, dst);
    }
    for (int src : in_nodes)
    {
        delete_edge(src, node_id);
    }
    cout << std::endl;
}

/*
Get test cursors
*/
WT_CURSOR *AdjList::get_node_cursor()
{
    if (node_cursor == nullptr)
    {

        int ret = _get_table_cursor(NODE_TABLE, &node_cursor, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a test node cursor");
        }
    }

    return node_cursor;
}

WT_CURSOR *AdjList::get_edge_cursor()
{
    if (edge_cursor == nullptr)
    {
        int ret = _get_table_cursor(EDGE_TABLE, &edge_cursor, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a test edge cursor");
        }
    }
    // WT_CURSOR *cursor;

    // int ret = _get_table_cursor(EDGE_TABLE, &cursor, false);
    // if (ret != 0)
    // {
    //     throw GraphException("Could not get a test edge cursor");
    // }
    // return cursor;
    return edge_cursor;
}

WT_CURSOR *AdjList::get_in_adjlist_cursor()
{
    if (in_adjlist_cursor == nullptr)
    {
        int ret = _get_table_cursor(IN_ADJLIST, &in_adjlist_cursor, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a test in_adjlist cursor");
        }
    }
    // WT_CURSOR *cursor;

    // int ret = _get_table_cursor(IN_ADJLIST, &cursor, false);
    // if (ret != 0)
    // {
    //     throw GraphException("Could not get a test in_adjlist cursor");
    // }
    // return cursor;
    return in_adjlist_cursor;
}

WT_CURSOR *AdjList::get_out_adjlist_cursor()
{
    if (out_adjlist_cursor == nullptr)
    {
        int ret = _get_table_cursor(OUT_ADJLIST, &out_adjlist_cursor, false);
        if (ret != 0)
        {
            throw GraphException("Could not get a test out_adjlist cursor");
        }
    }
    // WT_CURSOR *cursor;

    // int ret = _get_table_cursor(OUT_ADJLIST, &cursor, false);
    // if (ret != 0)
    // {
    //     throw GraphException("Could not get a test out_adjlist cursor");
    // }
    // return cursor;
    return out_adjlist_cursor;
}

WT_CURSOR *AdjList::get_node_iter()
{
    return get_node_cursor();
}

node AdjList::get_next_node(WT_CURSOR *n_cur)
{
    node found = {0};
    if (n_cur->next(n_cur) == 0)
    {
        CommonUtil::__record_to_node(n_cur, &found, opts.read_optimize);
        n_cur->get_key(n_cur, &found.id);
    }
    else
    {
        found.id = -1;
    }
    return found;
}

WT_CURSOR *AdjList::get_edge_iter()
{
    return get_edge_cursor();
}

edge AdjList::get_next_edge(WT_CURSOR *e_cur)
{
    edge found = {-1};
    if (e_cur->next(e_cur) == 0)
    {
        e_cur->get_key(e_cur, &found.src_id, &found.dst_id);
        if (opts.is_weighted)
        {
            CommonUtil::__record_to_edge(e_cur, &found);
        }
    }
    else
    {
        found.id = -1;
    }
    return found;
}

void AdjList::dump_tables()
{
    WT_CURSOR *n_cur, *e_cur, *inadj_cur, *outadj_cur;
    n_cur = get_node_cursor();
    e_cur = get_edge_cursor();
    inadj_cur = get_in_adjlist_cursor();
    outadj_cur = get_out_adjlist_cursor();

    int ret;
    cout << "------\n"
         << "Nodes:\n"
         << "------\n";
    while ((ret = n_cur->next(n_cur)) == 0)
    {
        node found;
        n_cur->get_key(n_cur, &found.id);
        n_cur->get_value(n_cur, &found.in_degree, &found.out_degree);
        CommonUtil::dump_node(found);
    }

    // dump all edges:
    cout << "------\n"
         << "Edges:\n"
         << "------\n";
    while ((ret = e_cur->next(e_cur)) == 0)
    {
        edge found;
        e_cur->get_key(e_cur, &found.src_id, &found.dst_id);
        e_cur->get_value(e_cur, &found.edge_weight);
        CommonUtil::dump_edge(found);
    }

    // dump in_adjlist:
    cout << "------\n"
         << "In Adjlist:\n"
         << "------\n";
    while ((ret = inadj_cur->next(inadj_cur)) == 0)
    {
        adjlist found;
        inadj_cur->get_key(inadj_cur, &found.node_id);
        CommonUtil::__record_to_adjlist(session, inadj_cur, &found);
        CommonUtil::dump_adjlist(found);
    }

    // dump in outadj_list:
    cout << "------\n"
         << "Out Adjlist:\n"
         << "------\n";
    while ((ret = outadj_cur->next(outadj_cur)) == 0)
    {
        adjlist found;
        outadj_cur->get_key(outadj_cur, &found.node_id);
        CommonUtil::__record_to_adjlist(session, outadj_cur, &found);
        CommonUtil::dump_adjlist(found);
    }
}