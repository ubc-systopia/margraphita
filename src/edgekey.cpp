#include "edgekey.h"
#include "common.h"
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <cassert>
#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
#include <sstream>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;
const std::string GRAPH_PREFIX = "edgekey";

EdgeKey::EdgeKey(){};
EdgeKey::EdgeKey(graph_opts opt_params)
{
    this->create_new = opt_params.create_new;
    this->read_optimize = opt_params.read_optimize;
    this->is_directed = opt_params.is_directed;
    this->is_weighted = opt_params.is_weighted;
    this->db_name = opt_params.db_name;
    this->db_dir = opt_params.db_dir;

    try
    {
        CommonUtil::check_graph_params(opt_params);
    }
    catch (GraphException G)
    {
        std::cout << G.what() << std::endl;
    }
    if (this->create_new)
    {
        create_new_graph();
        if (opt_params.optimize_create == false)
        {
            create_indices();
        }
    }
    else
    {
        std::filesystem::path dirname = db_dir + "/" + db_name;
        if (std::filesystem::exists(dirname))
        {
            __restore_from_db(db_dir + "/" + db_name);
        }
        else
        {
            throw GraphException("Could not find the expected WT DB directory - " + db_dir + "/" + db_name);
        }
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
    //Create new directory for WT DB
    std::filesystem::path dirname = db_dir + "/" + db_name;
    if (std::filesystem::exists(dirname))
    {
        filesystem::remove_all(dirname);
    }
    std::filesystem::create_directories(dirname);

    //open connection to WT DB
    ret = CommonUtil::open_connection(const_cast<char *>(dirname.c_str()), &conn);
    if (ret != 0)
    {
        throw GraphException("Could not open a connection to the DB" + string(wiredtiger_strerror(ret)));
        exit(-1);
    }
    if (CommonUtil::open_session(conn, &session) != 0)
    {
        exit(-1);
    }
    //Set up the edge table
    //Edge Columns: <src> <dst> <weight/in_degree> <out_degree>
    //Edge Column Format: iiS

    CommonUtil::set_table(session, EDGE_TABLE, edge_columns, edge_key_format, edge_value_format);

    //Set up the Metadata table
    //This does not change across implementations.
    //value_format:string (S)
    //key_format: string (S)
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
    insert_metadata(DB_NAME, const_cast<char *>(db_name.c_str()));

    //DB_DIR
    insert_metadata(DB_DIR, const_cast<char *>(db_dir.c_str()));

    // READ_OPTIMIZE
    string read_optimized_str = read_optimize ? "true" : "false";
    insert_metadata(READ_OPTIMIZE, const_cast<char *>(read_optimized_str.c_str()));

    // IS_DIRECTED
    string is_directed_str = is_directed ? "true" : "false";
    insert_metadata(IS_DIRECTED, const_cast<char *>(is_directed_str.c_str()));

    //IS_WEIGHTED
    string is_weighted_str = is_weighted ? "true" : "false";
    insert_metadata(IS_WEIGHTED, const_cast<char *>(is_weighted_str.c_str()));
    //#endif

    this->metadata_cursor->close(this->metadata_cursor);
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
int EdgeKey::_get_table_cursor(string table, WT_CURSOR **cursor,
                               bool is_random)
{
    std::string table_name = "table:" + table;
    const char *config = NULL;
    if (is_random)
    {
        config = "next_random=true";
    }

    if (int ret = session->open_cursor(session, table_name.c_str(), NULL, config, cursor) != 0)
    {
        fprintf(stderr, "Failed to get table cursor to %s\n", table_name.c_str());
        return ret;
    }
    return 0;
}

/**
 * @brief Returns the metadata associated with the key param from the METADATA
 * table.
 * Same from standard_graph implementation
 */
string EdgeKey::get_metadata(string key)
{

    int ret = 0;
    WT_CURSOR *metadata_cursor = nullptr;
    if ((ret = _get_table_cursor(METADATA, &metadata_cursor, false)) != 0)
    {
        fprintf(stderr, "Failed to create cursor to the metadata table.");
        exit(-1);
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
 * @brief This private function inserts metadata values into the metadata
 * table. The fields are self explanatory.
 * Same from standard_graph implementation.
 */
void EdgeKey::insert_metadata(string key, char *value)
{
    int ret = 0;
    WT_CURSOR *metadata_cursor = nullptr;
    if ((ret = _get_table_cursor(METADATA, &metadata_cursor, false)) != 0)
    {
        fprintf(stderr, "Failed to create cursor to the metadata table.");
        exit(-1);
    }

    metadata_cursor->set_key(metadata_cursor, key.c_str());
    metadata_cursor->set_value(metadata_cursor, value);
    if ((ret = metadata_cursor->insert(metadata_cursor)) != 0)
    {
        fprintf(stderr, "failed to insert metadata for key %s", key.c_str());
    }
    metadata_cursor->close(metadata_cursor);
}

/**
 * @brief get the node identified by node_id
 * 
 * @param node_id the Node ID
 * @return node the node struct containing the node
 */
node EdgeKey::get_node(int node_id)
{
    WT_CURSOR *cursor = get_edge_cursor();
    cursor->set_key(cursor, node_id, -1);
    node found = {0};
    if (cursor->search(cursor) == 0)
    {
        found.id = node_id;
        __record_to_node(cursor, &found);
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
    int ret = _get_table_cursor(EDGE_TABLE, &random_cur, true);
    if (ret != 0)
    {
        throw GraphException("could not get a random cursor to the node table");
    }
    int src, dst;
    ret = random_cur->next(random_cur);
    do
    {
        ret = random_cur->get_key(random_cur, &src, &dst);
        if (ret != 0)
        {
            throw GraphException("here");
        }
        if (dst == -1)
        {
            //random_cur->set_key(random_cur, src, dst);
            __record_to_node(random_cur, &rando);
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
            if (dst == -1)
            {
                //random_cur->set_key(random_cur, src, dst);
                __record_to_node(random_cur, &rando);
                rando.id = src;
                break;
            }
        }
    } while (dst != -1);
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
    WT_CURSOR *node_cursor;
    ret = _get_table_cursor(EDGE_TABLE, &node_cursor, false);
    node_cursor->set_key(node_cursor, to_insert.id, -1);
    if (read_optimize)
    {
        string packed = pack_int_to_str(to_insert.in_degree, to_insert.out_degree);
        node_cursor->set_value(node_cursor, packed.c_str());
    }
    else
    {
        node_cursor->set_value(node_cursor, "");
    }
    if (node_cursor->insert(node_cursor) != 0)
    {
        throw GraphException("Failed to insert a node with ID " + std::to_string(to_insert.id) + " into the edge table");
    }
}

/**
 * @brief returns true if a node with id node_id exists in the edge table
 * 
 * @param node_id the ID to look for
 * @return true / false depending on if node_id is found or not
 */
bool EdgeKey::has_node(int node_id)
{
    int ret = 0;
    WT_CURSOR *e_cur = get_edge_cursor();

    e_cur->set_key(e_cur, node_id, -1);
    if (e_cur->search(e_cur) == 0)
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
bool EdgeKey::has_edge(int src_id, int dst_id)
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

void EdgeKey::delete_node(int node_id)
{
    WT_CURSOR *e_cur = get_edge_cursor();
    WT_CURSOR *src_cur = get_src_idx_cur();
    WT_CURSOR *dst_cur = get_dst_idx_cur();

    if (_get_table_cursor(EDGE_TABLE, &e_cur, false) != 0)
    {
        throw GraphException("Failed to get a cursro to the edge table");
    }
    e_cur->set_key(e_cur, node_id, -1);
    if (e_cur->remove(e_cur) != 0)
    {
        throw GraphException("Failed to remove node with id " + std::to_string(node_id) + " from the edge table");
    }
    delete_related_edges(src_cur, e_cur, node_id);
    delete_related_edges(dst_cur, e_cur, node_id);
}

void EdgeKey::delete_related_edges(WT_CURSOR *idx_cur, WT_CURSOR *e_cur, int node_id)
{
    int ret = 0;
    e_cur->reset(e_cur);

    idx_cur->set_key(idx_cur, node_id);
    if (idx_cur->search(idx_cur) == 0)
    {
        int src, dst;
        idx_cur->get_value(idx_cur, &src, &dst);
        e_cur->set_key(e_cur, src, dst);
        ret = e_cur->remove(e_cur);
        if (ret != 0)
        {
            throw GraphException("Failed to remove edge between " + to_string(src) + " and " + to_string(dst));
        }
        //if readoptimize: Get the node to decrement in/out degree
        if (read_optimize)
        {
            node temp;
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
        }
        while (idx_cur->next(idx_cur) == 0 && idx_cur->get_key(idx_cur) == node_id)
        {
            int src, dst;
            idx_cur->get_value(idx_cur, &src, &dst);
            e_cur->set_key(e_cur, src, dst);
            ret = e_cur->remove(e_cur);
            if (ret != 0)
            {
                throw GraphException("Failed to remove edge between " + to_string(src) + " and " + to_string(dst));
            }
            if (read_optimize)
            {
                node temp;
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
            }
        }
    }
}

/**
 * @brief update the in and out degrees for the node identified by node_id
 * Does nothing if not read_optimize
 * @param node_id The ID of the node to be updated
 * @param indeg new in_degree
 * @param outdeg new out_degree
 */
void EdgeKey::update_node_degree(int node_id, int indeg, int outdeg)
{
    if (read_optimize)
    {
        int ret = 0;
        WT_CURSOR *e_cur = get_edge_cursor();

        e_cur->set_key(e_cur, node_id, -1);
        string val = pack_int_to_str(indeg, outdeg);
        e_cur->set_value(e_cur, val.c_str());
        if (e_cur->insert(e_cur) != 0)
        {
            throw GraphException("Could not update the node with ID" + std::to_string(node_id));
        }
    }
}

/**
 * @brief This function adds the edge passed as param
 * 
 * @param to_insert the edge struct containing info about the edge to insert. 
 */
void EdgeKey::add_edge(edge to_insert)
{
    //Check if the src and dst nodes exist.
    if (!has_node(to_insert.src_id))
    {
        node src;
        src.id = to_insert.src_id;
        add_node(src);
    }

    if (!has_node(to_insert.dst_id))
    {
        node dst;
        dst.id = to_insert.dst_id;
        add_node(dst);
    }
    //Insert the edge
    WT_CURSOR *e_cur = get_edge_cursor();
    e_cur->set_key(e_cur, to_insert.src_id, to_insert.dst_id);
    if (is_weighted)
    {
        e_cur->set_value(e_cur, std::to_string(to_insert.edge_weight).c_str());
    }
    else
    {
        e_cur->set_value(e_cur, "");
    }
    if (e_cur->insert(e_cur) != 0)
    {
        throw GraphException("Failed to insert edge between " + std::to_string(to_insert.src_id) + " and " + std::to_string(to_insert.dst_id));
    }
    //insert reverse edge if undirected
    if (!is_directed)
    {
        e_cur->set_key(e_cur, to_insert.dst_id, to_insert.src_id);
        if (is_weighted)
        {
            e_cur->set_value(e_cur, std::to_string(to_insert.edge_weight).c_str());
        }
        else
        {
            e_cur->set_value(e_cur, "");
        }
        if (e_cur->insert(e_cur) != 0)
        {
            throw GraphException("Failed to insert the reverse edge between " + std::to_string(to_insert.src_id) + " and " + std::to_string(to_insert.dst_id));
        }
    }

    //Update the in/out degrees if read_optimize
    if (read_optimize)
    {
        node src = get_node(to_insert.src_id);
        src.out_degree++;
        if (!is_directed)
        {
            src.in_degree++;
        }
        update_node_degree(src.id, src.in_degree, src.out_degree);

        node dst = get_node(to_insert.dst_id);
        dst.in_degree++;
        if (!is_directed)
        {
            dst.out_degree++;
        }
        update_node_degree(dst.id, dst.in_degree, dst.out_degree);
    }
}

void EdgeKey::bulk_add_edge(int src, int dst, int weight)
{
    // No need to Check if the src and dst nodes exist. Inserted already.
    //Insert the edge
    WT_CURSOR *e_cur = get_edge_cursor();
    e_cur->set_key(e_cur, src, dst);
    if (is_weighted)
    {
        e_cur->set_value(e_cur, std::to_string(weight).c_str());
    }
    else
    {
        e_cur->set_value(e_cur, "");
    }
    if (e_cur->insert(e_cur) != 0)
    {
        throw GraphException("Failed to insert edge between " + std::to_string(src) + " and " + std::to_string(dst));
    }
    //insert reverse edge if undirected
    if (!is_directed)
    {
        e_cur->set_key(e_cur, dst, src);
        if (is_weighted)
        {
            e_cur->set_value(e_cur, std::to_string(weight).c_str());
        }
        else
        {
            e_cur->set_value(e_cur, "");
        }
        if (e_cur->insert(e_cur) != 0)
        {
            throw GraphException("Failed to insert the reverse edge between " + std::to_string(src) + " and " + std::to_string(dst));
        }
    }
}

/**
 * @brief Delete the edge identified by (src_id, dst_id)
 * 
 * @param src_id Source node ID for the edge to be deleted
 * @param dst_id Dst node ID for the edge to be deleted
 */
void EdgeKey::delete_edge(int src_id, int dst_id)
{
    //delete edge
    WT_CURSOR *e_cur = get_edge_cursor();
    e_cur->set_key(e_cur, src_id, dst_id);
    if (e_cur->remove(e_cur) != 0)
    {
        throw GraphException("Failed to delete the edge between " + std::to_string(src_id) + " and " + std::to_string(dst_id));
    }

    //delete reverse edge
    if (!is_directed)
    {
        e_cur->set_key(e_cur, dst_id, src_id);
        if (e_cur->remove(e_cur) != 0)
        {
            throw GraphException("Failed to remove the reverse  edge between " + std::to_string(src_id) + " and " + to_string(dst_id));
        }
    }

    //update node degrees
    if (read_optimize)
    {
        node src = get_node(src_id);

        if (src.out_degree > 0)
        {
            src.out_degree--;
        }

        if (!is_directed && src.in_degree > 0)
        {

            src.in_degree--;
        }
        update_node_degree(src.id, src.in_degree, src.out_degree);

        node dst = get_node(dst_id);
        if (dst.in_degree > 0)
        {
            dst.in_degree--;
        }
        if (!is_directed && dst.out_degree > 0)
        {
            dst.out_degree--;
        }
        update_node_degree(dst_id, dst.in_degree, dst.out_degree);
    }
}

/**
 * @brief Get the edge identified by (src_id, dst_id)
 * 
 * @param src_id source node_id
 * @param dst_id destination node_id
 * @return edge the edge object found
 */
edge EdgeKey::get_edge(int src_id, int dst_id)
{
    edge found = {0};
    WT_CURSOR *e_cur = get_edge_cursor();
    e_cur->set_key(e_cur, src_id, dst_id);
    if (e_cur->search(e_cur) == 0)
    {
        found.src_id = src_id;
        found.dst_id = dst_id;
        __record_to_edge(e_cur, &found);
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

    WT_CURSOR *dst_cur = get_dst_idx_cur();
    WT_CURSOR *e_cur = get_edge_cursor();

    dst_cur->set_key(dst_cur, -1);
    if (dst_cur->search(dst_cur) == 0)
    {
        int node_id, temp;
        dst_cur->get_value(dst_cur, &node_id, &temp);
        assert(temp == -1); //this should be true
        e_cur->set_key(e_cur, node_id, -1);
        if (e_cur->search(e_cur) == 0)
        {
            node found;
            found.id = node_id;
            __record_to_node(e_cur, &found);
            nodes.push_back(found);
        }

        while (dst_cur->next(dst_cur) == 0)
        {
            dst_cur->get_value(dst_cur, &node_id, &temp);
            if (temp == -1)
            {
                e_cur->set_key(e_cur, node_id, -1);
                if (e_cur->search(e_cur) == 0)
                {
                    node found;
                    found.id = node_id;
                    __record_to_node(e_cur, &found);
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
 * @brief This sets the value of the passed int references to hold the number of
 * nodes and edges present in the table.
 * 
 * @param node_count Arg to hold the number of nodes
 * @param edge_count Arg to hold the number of edges. 
 */
//TODO: Possible optimization: store the value of node and edge counts in the
//metadata table while calling close(). Restore the global node and edge counts
//value on restore from DB. Then add and delete operations modify this global
//value. This function is wasteful.
int EdgeKey::get_num_nodes()
{
    int node_count = 0;
    WT_CURSOR *dst_cur = get_dst_idx_cur();
    while (dst_cur->next(dst_cur) == 0)
    {
        int dst;
        dst_cur->get_key(dst_cur, &dst); // dst_cursor key points to dst
        if (dst == -1)
        {
            node_count++;
        }
    }
    dst_cur->reset(dst_cur);
    return node_count;
}

int EdgeKey::get_num_edges()
{
    WT_CURSOR *dst_cur = get_dst_idx_cur();
    int edge_count = 0;
    while (dst_cur->next(dst_cur) == 0)
    {
        int dst;
        dst_cur->get_key(dst_cur, &dst); // dst_cursor key points to dst
        if (dst != -1)
        {
            edge_count++;
        }
    }
    dst_cur->reset(dst_cur);
    return edge_count;
}

/**
 * @brief get the out_degree of the node that has ID = node_id
 * 
 * @param node_id ID of the node whose out degree is sought
 * @return int outdegree of node with ID node_id
 */
int EdgeKey::get_out_degree(int node_id)
{
    if (read_optimize)
    {
        WT_CURSOR *e_cur = get_edge_cursor();
        e_cur->set_key(e_cur, node_id, -1);
        e_cur->search(e_cur);
        node found;
        __record_to_node(e_cur, &found);
        e_cur->reset(e_cur);
        return found.out_degree;
    }
    else
    {
        int out_deg;
        WT_CURSOR *src_cur = get_src_idx_cur();
        src_cur->set_key(src_cur, node_id);
        if (src_cur->search(src_cur) == 0)
        {
            int src, dst;
            src_cur->get_value(src_cur, &src, &dst);

            if (src == node_id && dst != -1)
            {
                out_deg++;
            }
            while (src_cur->next(src_cur) == 0)
            {
                src_cur->get_value(src_cur, &src, &dst);
                if (src == node_id)
                {
                    if (dst != -1)
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
int EdgeKey::get_in_degree(int node_id)
{
    if (read_optimize)
    {
        WT_CURSOR *e_cur = get_edge_cursor();
        e_cur->set_key(e_cur, node_id, -1);
        e_cur->search(e_cur);
        node found;
        __record_to_node(e_cur, &found);
        e_cur->reset(e_cur);
        return found.in_degree;
    }
    else
    {
        int in_degree;
        WT_CURSOR *dst_cur = get_dst_idx_cur();
        dst_cur->set_key(dst_cur, node_id);
        if (dst_cur->search(dst_cur) == 0)
        {
            int src, dst;
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
        if (found.dst_id != -1)
        {
            if (is_weighted)
            {
                __record_to_edge(e_cur, &found);
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
std::vector<edge> EdgeKey::get_out_edges(int node_id)
{
    std::vector<edge> out_edges;
    WT_CURSOR *e_cur = get_edge_cursor();
    WT_CURSOR *src_cur = get_src_idx_cur();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    src_cur->set_key(src_cur, node_id);
    int search_ret = src_cur->search(src_cur);
    int flag = 0;
    if (search_ret == 0)
    {
        int src_id, dst_id;

        do
        {
            src_cur->get_value(src_cur, &src_id, &dst_id);
            if (src_id != node_id)
            {
                break;
            }
            if (src_id == node_id && dst_id == -1)
            {
                flag++;
            }
            else
            {
                e_cur->set_key(e_cur, src_id, dst_id);
                if (e_cur->search(e_cur) == 0)
                {
                    edge found;
                    found.dst_id = dst_id;
                    found.src_id = src_id;
                    __record_to_edge(e_cur, &found);
                    out_edges.push_back(found);
                }
            }

        } while (src_id == node_id && src_cur->next(src_cur) == 0 && flag <= 1); //flag check ensures that if the first entry we hit was (node_id, -1), we try to look for the next entry the cursor points to to see if it has a (node_id, <dst>) entry.
    }
    e_cur->reset(e_cur);
    src_cur->reset(src_cur);
    return out_edges;
}

/**
 * @brief Returns a list containing all nodes that have an incoming edge from node_id
 * 
 * @param node_id Node ID of the node who's out nodes are sought.
 * @return std::vector<node> 
 */
std::vector<node> EdgeKey::get_out_nodes(int node_id)
{
    std::vector<node> out_nodes;
    WT_CURSOR *src_cur = get_src_idx_cur();
    WT_CURSOR *e_cur = get_edge_cursor();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    src_cur->set_key(src_cur, node_id);
    int search_ret = src_cur->search(src_cur);
    int flag = 0;
    if (search_ret == 0)
    {
        int src, dst;
        do
        {
            src_cur->get_value(src_cur, &src, &dst);
            //don't need to check if src == node_id because search_ret was 0
            if (src != node_id)
            {
                break;
            }
            if (src == node_id && dst == -1)
            {
                flag++;
            }
            else
            {
                e_cur->set_key(e_cur, dst, -1);
                if (e_cur->search(e_cur) == 0)
                {
                    node found;
                    found.id = dst;
                    __record_to_node(e_cur, &found);
                    out_nodes.push_back(found);
                }
            }
        } while (src == node_id && src_cur->next(src_cur) == 0 && flag <= 1); //flag check ensures that if the first entry we hit was (node_id, -1), we try to look for the next entry the cursor points to to see if it has a (node_id, <dst>) entry.
    }
    e_cur->reset(e_cur);
    src_cur->reset(src_cur);
    return out_nodes;
}

/**
 * @brief Get a list of all edges that have node_id as the destination
 * 
 * @param node_id the id of the node whose in_edges are sought
 * @return std::vector<edge> 
 */
std::vector<edge> EdgeKey::get_in_edges(int node_id)
{
    std::vector<edge> in_edges;
    WT_CURSOR *e_cur = get_edge_cursor();
    WT_CURSOR *dst_cur = get_dst_idx_cur();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    dst_cur->set_key(dst_cur, node_id);
    int search_ret = dst_cur->search(dst_cur);
    if (search_ret == 0)
    {
        int src_id, dst_id;
        dst_cur->get_value(dst_cur, &src_id, &dst_id);

        do
        {
            e_cur->set_key(e_cur, src_id, dst_id);
            if (e_cur->search(e_cur) == 0)
            {
                edge found;
                found.src_id = src_id;
                found.dst_id = dst_id;
                __record_to_edge(e_cur, &found);
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
std::vector<node> EdgeKey::get_in_nodes(int node_id)
{
    std::vector<node> in_nodes;
    WT_CURSOR *e_cur = get_edge_cursor();
    WT_CURSOR *dst_cur = get_dst_idx_cur();

    if (!has_node(node_id))
    {
        throw GraphException("There is no node with ID " + to_string(node_id));
    }
    dst_cur->set_key(dst_cur, node_id);
    if (dst_cur->search(dst_cur) == 0)
    {
        int src_id, dst_id;
        if (dst_cur->get_value(dst_cur, &src_id, &dst_id) != 0)
        {
            throw GraphException("here" + __LINE__);
        }

        do
        {
            e_cur->set_key(e_cur, src_id, -1);
            if (e_cur->search(e_cur) == 0)
            {
                node found;
                found.id = src_id;
                __record_to_node(e_cur, &found);
                in_nodes.push_back(found);
            }
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
    e_cur->reset(e_cur);
    dst_cur->reset(dst_cur);
    return in_nodes;
}

//Close, restore from DB, create/drop indices
void EdgeKey::__restore_from_db(string db_name)
{
    int ret = CommonUtil::open_connection(const_cast<char *>(db_name.c_str()), &conn);
    WT_CURSOR *cursor = nullptr;

    ret = CommonUtil::open_session(conn, &session);
    const char *key, *value;
    ret = _get_table_cursor(METADATA, &cursor, false);

    while ((ret = cursor->next(cursor)) == 0)
    {
        ret = cursor->get_key(cursor, &key);
        ret = cursor->get_value(cursor, &value);

        if (strcmp(key, DB_DIR.c_str()) == 0)
        {

            this->db_dir = value; //CommonUtil::unpack_string_wt(value, this->session);
        }
        else if (strcmp(key, DB_NAME.c_str()) == 0)
        {

            this->db_name = value; //CommonUtil::unpack_string_wt(value, this->session);
        }
        else if (strcmp(key, READ_OPTIMIZE.c_str()) == 0)
        {
            if (strcmp(value, "true") == 0)
            {
                this->read_optimize = true;
            }
            else
            {
                this->read_optimize = false;
            }
        }
        else if (strcmp(key, IS_DIRECTED.c_str()) == 0)
        {
            if (strcmp(value, "true") == 0)
            {
                this->is_directed = true;
            }
            else
            {
                this->is_directed = false;
            }
        }
        else if (strcmp(key, IS_WEIGHTED.c_str()) == 0)
        {
            if (strcmp(value, "true") == 0)
            {
                this->is_weighted = true;
            }
            else
            {
                this->is_weighted = false;
            }
        }
    }
    cursor->close(cursor);
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
void EdgeKey::create_indices()
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
    int ret = 0;
    //Index on SRC column of edge table
    string edge_table_idx, edge_table_idx_conf;
    edge_table_idx = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
    edge_table_idx_conf = "columns=(" + SRC + ")";
    if (session->create(session, edge_table_idx.c_str(), edge_table_idx_conf.c_str()) != 0)
    {
        throw GraphException("Failed to create SRC_INDEX on the edge table");
    }

    //Index on the DST column of the edge table
    edge_table_idx = "index:" + EDGE_TABLE + ":" + DST_INDEX;
    edge_table_idx_conf = "columns=(" + DST + ")";
    if (session->create(session, edge_table_idx.c_str(), edge_table_idx_conf.c_str()) != 0)
    {
        throw GraphException("Failed to create an index on the DST column of the edge table");
    }
} /**
 * changing the src index column from SRC to SRC, DST
 * and dst index column from DST to DST,SRC 
 */

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

    //drop SRC index
    string uri = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
    session->drop(session, uri.c_str(), NULL);

    //drop the DST index
    uri = "index:" + EDGE_TABLE + ":" + DST_INDEX;
    session->drop(session, uri.c_str(), NULL);
}

//Get and Set data from/to an edge table cursor

/**
 * @brief This function converts the record the cursor points to into a node
 * struct. In this representation, we know that the Value can be :
 * 1. indeg, out_deg for read_optimize node entry
 * 2. "" for non read_optimize node entry
 * 3. edge_weight for a weighted edge entry
 * 4. "" for an unweighted graph. 
 * 
 * Seeing this, it is wasteful to have a serialization/deserialization call. 
 * Save -1 for any of these entries that don't exist.
 * @param cur the pointer -- already pointing to the record to extract
 * @param found the node to insert the in and outdegrees for. 
 */
void EdgeKey::__record_to_node(WT_CURSOR *cur, node *found)
{
    char *packed_vec;
    int ret = cur->get_value(cur, &packed_vec);
    if (ret != 0)
    {
        throw GraphException("in here");
    }
    std::string str(packed_vec);
    int a, b;
    extract_from_string(packed_vec, &a, &b);
    found->in_degree = a;
    found->out_degree = b;
}

void EdgeKey::__record_to_edge(WT_CURSOR *cur, edge *found)
{
    char *packed_vec;
    int ret = cur->get_value(cur, &packed_vec);
    if (ret != 0)
    {
        throw GraphException("in here");
    }
    std::string str(packed_vec);
    int a, b;
    extract_from_string(str, &a, &b);
    found->edge_weight = a;
}

//Session/Connection/Cursor operations

void EdgeKey::close()
{
    CommonUtil::close_session(session);
    CommonUtil::close_connection(conn);
}

/**
 * @brief Generic function to create the indexes on a table
 *
 * @param table_name The name of the table on which the index is to be created.
 * @param idx_name The name of the index
 * @param projection The columns that are to be included in the index. This is
 * in the format "(col1,col2,..)"
 * @param cursor This is the cursor variable that needs to be set.
 * @return 0 if the index could be set
 */
int EdgeKey::_get_index_cursor(std::string table_name,
                               std::string idx_name,
                               std::string projection,
                               WT_CURSOR **cursor)
{
    std::string index_name = "index:" + table_name + ":" + idx_name + projection;
    if (int ret = session->open_cursor(session, index_name.c_str(), NULL, NULL,
                                       cursor) != 0)
    {
        fprintf(stderr, "Failed to open the cursor on the index %s on table %s \n",
                index_name.c_str(), table_name.c_str());
        return ret;
    }
    return 0;
}

/*
Get cursors
*/
WT_CURSOR *EdgeKey::get_edge_cursor()
{
    if (edge_cursor == nullptr)
    {
        if (_get_table_cursor(EDGE_TABLE, &edge_cursor, false) != 0)
        {
            throw GraphException("Could not get a cursor to the Edge table");
        }
    }

    return edge_cursor;
}

WT_CURSOR *EdgeKey::get_src_idx_cur()
{
    if (src_idx_cursor == nullptr)
    {
        string projection = "(" + SRC + "," + DST + ")";
        if (_get_index_cursor(EDGE_TABLE, SRC_INDEX, projection,
                              &src_idx_cursor) != 0)
        {
            throw GraphException("Could not get a cursor to the SRC_INDEX ");
        }
    }

    return src_idx_cursor;
}

WT_CURSOR *EdgeKey::get_dst_idx_cur()
{
    if (dst_idx_cursor == nullptr)
    {
        string projection = "(" + SRC + "," + DST + ")";
        if (_get_index_cursor(EDGE_TABLE, DST_INDEX, projection, &dst_idx_cursor) != 0)
        {
            throw GraphException("Could not get a cursor to DST_INDEX");
        }
    }

    return dst_idx_cursor;
}

//Because we know that there can only be two. By design.
void EdgeKey::extract_from_string(string packed_str, int *a, int *b)
{
    std::stringstream strstream(packed_str);
    strstream >> *a >> *b;
    return;
}

string EdgeKey::pack_int_to_str(int a, int b)
{
    std::stringstream sstream;
    sstream << a << " " << b;
    return sstream.str();
}

WT_CURSOR *EdgeKey::get_node_iter()
{
    return get_dst_idx_cur();
}

node EdgeKey::get_next_node(WT_CURSOR *dst_cur)
{
    dst_cur->set_key(dst_cur, -1);
    WT_CURSOR *e_cur = get_edge_cursor();
    node found = {-1};
    if (dst_cur->search(dst_cur) == 0)
    {
        int node_id, temp;
        dst_cur->get_value(dst_cur, &node_id, &temp);
        assert(temp == -1); //this should be true
        e_cur->set_key(e_cur, node_id, -1);
        if (e_cur->search(e_cur) == 0)
        {

            found.id = node_id;
            __record_to_node(e_cur, &found);
        }
    }
    return found;
}

WT_CURSOR *EdgeKey::get_edge_iter()
{
    return get_edge_cursor();
}

edge EdgeKey::get_next_edge(WT_CURSOR *e_cur)
{
    edge found = {-1};
    while (e_cur->next(e_cur) == 0)
    {
        e_cur->get_key(e_cur, &found.src_id, &found.dst_id);
        if (found.dst_id != -1)
        {
            if (is_weighted)
            {
                __record_to_edge(e_cur, &found);
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