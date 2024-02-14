
#include "graph.h"

#include <wiredtiger.h>

#include <cstring>
#include <iostream>
#include <string>

#include "common_util.h"
#include "graph_exception.h"
using namespace std;

GraphBase::GraphBase(graph_opts &opt_params, WT_CONNECTION *conn)
{
    opts = opt_params;
    connection = conn;
    if (CommonUtil::open_session(conn, &session) != 0)
    {
        throw GraphException("Cannot open session");
    }
    if (opts.create_new == false) _restore_from_db();
}

void GraphBase::create_metadata_table(graph_opts &opts, WT_CONNECTION *conn)
{
    WT_SESSION *session;
    if (CommonUtil::open_session(conn, &session) != 0)
    {
        throw GraphException("Cannot open session");
    }

    /* Now doing the metadata table creation.
    function This table stores the graph metadata
    value_format:string (S)
    key_format: string (S)
    */
    string metadata_table_name = "table:metadata";
    if (session->create(session,
                        metadata_table_name.c_str(),
                        "key_format=I,value_format=u") > 0)
    {
        fprintf(stderr, "Failed to create the metadata table ");
    }
    WT_CURSOR *metadata_cursor;
    GraphBase::_get_table_cursor(
        METADATA, &metadata_cursor, session, false, false);

    // DB_NAME
    GraphBase::insert_metadata(MetadataKey::db_name,
                               const_cast<char *>(opts.db_name.c_str()),
                               opts.db_name.length(),
                               metadata_cursor);

    // DB_DIR
    GraphBase::insert_metadata(MetadataKey::db_dir,
                               const_cast<char *>(opts.db_dir.c_str()),
                               opts.db_dir.length(),
                               metadata_cursor);

    // READ_OPTIMIZE
    GraphBase::insert_metadata(MetadataKey::read_optimize,
                               (char *)(&opts.read_optimize),
                               sizeof(bool),
                               metadata_cursor);

    // IS_DIRECTED
    GraphBase::insert_metadata(MetadataKey::is_directed,
                               (char *)(&opts.is_directed),
                               sizeof(bool),
                               metadata_cursor);

    // is_weighted
    GraphBase::insert_metadata(MetadataKey::is_weighted,
                               (char *)(&opts.is_weighted),
                               sizeof(bool),
                               metadata_cursor);

    // NUM_NODES = 0
    node_id_t temp_num = 0;
    GraphBase::insert_metadata(MetadataKey::num_nodes,
                               (char *)&temp_num,
                               sizeof(node_id_t),
                               metadata_cursor);

    // NUM_EDGES = 0
    GraphBase::insert_metadata(MetadataKey::num_edges,
                               (char *)&temp_num,
                               sizeof(uint64_t),
                               metadata_cursor);

    // Max_node_it =0
    GraphBase::insert_metadata(MetadataKey::max_node_id,
                               (char *)&temp_num,
                               sizeof(node_id_t),
                               metadata_cursor);
    // Min_node_it =0
    GraphBase::insert_metadata(MetadataKey::min_node_id,
                               (char *)&temp_num,
                               sizeof(node_id_t),
                               metadata_cursor);

    metadata_cursor->close(metadata_cursor);
    session->close(session, nullptr);
}

/**
 * @brief This private function inserts metadata values into the metadata
 * table. The fields are self explanatory.
 *
 */
void GraphBase::insert_metadata(const int key,
                                const char *value,
                                size_t size,
                                WT_CURSOR *cursor)
{
    cursor->set_key(cursor, key);
    WT_ITEM item;
    item.data = value;
    item.size = size;
    cursor->set_value(cursor, &item);
    int ret = cursor->insert(cursor);
    if (ret != 0)
    {
        fprintf(stderr, "Failed to insert metadata for the key %d", key);
        fprintf(stderr, "Error: %s\n", wiredtiger_strerror(ret));
        exit(-1);
    }
}

/**
 * @brief Returns the metadata associated with the key param from the METADATA
 * table.
 */
void GraphBase::get_metadata(const int key, WT_ITEM &item, WT_CURSOR *cursor)
{
    int ret;
    if (cursor == nullptr)
    {
        _get_table_cursor(METADATA, &metadata_cursor, session, false, false);
    }
    cursor->set_key(cursor, key);
    ret = cursor->search(cursor);
    if (ret != 0)
    {
        fprintf(stderr,
                "Could not find key %s in metadata table\n",
                MetadataKeyNames[key].c_str());
        exit(-1);
    }

    ret = cursor->get_value(cursor, &item);
    if (ret != 0)
    {
        fprintf(stderr,
                "Failed to retrieve metadata for the key %s\n",
                MetadataKeyNames[key].c_str());
        exit(-1);
    }
}

/**
 * @brief This is the generic function to get a cursor on the table
 *
 * @param table This is the table name for which the cursor is needed.
 * @param cursor This is the pointer that will hold the set cursor.
 * @param is_random This is a bool value to indicate if the cursor must be
 * random.
 * @param prevent_overwrite This is a bool value to indicate whether we want to
 * specify overwrite=false, imposing stricter checks on inserts/updates
 * @return 0 if the cursor could be set
 */
int GraphBase::_get_table_cursor(const std::string &table,
                                 WT_CURSOR **cursor,
                                 WT_SESSION *session,
                                 bool is_random,
                                 bool prevent_overwrite)
{
    std::string table_name = "table:" + table;
    std::string config;
    if (is_random)
    {
        config += "next_random=true,";
    }
    if (prevent_overwrite)
    {
        config += "overwrite=false,";
    }
    if (session->open_cursor(
            session, table_name.c_str(), nullptr, config.c_str(), cursor) != 0)
    {
        throw GraphException("Failed to open the cursor on the table " +
                             table_name);
    }
    return 0;
}

void GraphBase::close()
{
    int ret = this->connection->close(this->connection, nullptr);
    if (ret != 0)
    {
        throw GraphException("Failed to close the connection");
    }
}

// Close, restore from DB, create/drop indices
void GraphBase::_restore_from_db()
{
    WT_CURSOR *cursor = nullptr;
    _get_table_cursor(METADATA, &cursor, session, false, false);

    int key;
    WT_ITEM item;
    while (cursor->next(cursor) == 0)
    {
        cursor->get_key(cursor, &key);
        cursor->get_value(cursor, &item);

        if (key == MetadataKey::db_dir)
        {
            this->opts.db_dir = string((char *)item.data, item.size);
        }
        else if (key == MetadataKey::db_name)
        {
            this->opts.db_name = string((char *)item.data, item.size);
        }
        else if (key == MetadataKey::is_weighted)
        {
            this->opts.is_weighted = *((bool *)item.data);
        }
        else if (key == MetadataKey::read_optimize)
        {
            this->opts.read_optimize = *((bool *)item.data);
        }
        else if (key == MetadataKey::is_directed)
        {
            this->opts.is_directed = *((bool *)item.data);
        }
        else if (key == MetadataKey::num_nodes)
        {
            this->opts.num_nodes = *((node_id_t *)item.data);
        }
        else if (key == MetadataKey::num_edges)
        {
            this->opts.num_edges = *((uint64_t *)item.data);
        }
    }
}

// Close, restore from DB, create/drop indices
void GraphBase::_restore_from_db(const std::string &db_name)
{
    CommonUtil::open_connection(const_cast<char *>(db_name.c_str()),
                                opts.stat_log,
                                opts.conn_config,
                                &connection);
    WT_CURSOR *cursor = nullptr;

    CommonUtil::open_session(connection, &session);
    int key;
    WT_ITEM item;
    _get_table_cursor(METADATA, &cursor, session, false, false);

    while (cursor->next(cursor) == 0)
    {
        cursor->get_key(cursor, &key);
        cursor->get_value(cursor, &item);

        if (key == MetadataKey::db_dir)
        {
            this->opts.db_dir = string((char *)item.data, item.size);
        }
        else if (key == MetadataKey::db_name)
        {
            this->opts.db_name = string((char *)item.data, item.size);
        }
        else if (key == MetadataKey::is_weighted)
        {
            this->opts.is_weighted = *((bool *)item.data);
        }
        else if (key == MetadataKey::read_optimize)
        {
            this->opts.read_optimize = *((bool *)item.data);
        }
        else if (key == MetadataKey::is_directed)
        {
            this->opts.is_directed = *((bool *)item.data);
        }
        else if (key == MetadataKey::num_nodes)
        {
            this->opts.num_nodes = *((node_id_t *)item.data);
        }
        else if (key == MetadataKey::num_edges)
        {
            this->opts.num_edges = *((uint64_t *)item.data);
        }
    }
}

/**
 * @brief Generic function to create the indexes on a table
 *
 * @param table_name The name of the table on which the index is to be
 * created.
 * @param idx_name The name of the index
 * @param projection The columns that are to be included in the index. This
 * is in the format "(col1,col2,..)"
 * @param cursor This is the cursor variable that needs to be set.
 * @return 0 if the index could be set
 */
int GraphBase::_get_index_cursor(const std::string &table_name,
                                 const std::string &idx_name,
                                 const std::string &projection,
                                 WT_CURSOR **cursor)
{
    std::string index_name =
        "index:" + table_name + ":" + idx_name + projection;
    if (int ret = session->open_cursor(
                      session, index_name.c_str(), NULL, NULL, cursor) != 0)
    {
        fprintf(stderr,
                "Failed to open the cursor on the index %s on table %s: %s \n",
                index_name.c_str(),
                table_name.c_str(),
                wiredtiger_strerror(ret));
        return ret;
    }
    return 0;
}

void GraphBase::set_num_nodes(uint64_t num_nodes, WT_CURSOR *metadata_cur)
{
    insert_metadata(MetadataKey::num_nodes,
                    (char *)&num_nodes,
                    sizeof(node_id_t),
                    metadata_cur);
}

void GraphBase::set_num_edges(uint64_t num_edges, WT_CURSOR *metadata_cur)
{
    insert_metadata(MetadataKey::num_edges,
                    (char *)&num_edges,
                    sizeof(uint64_t),
                    metadata_cur);
}

uint32_t GraphBase::get_num_nodes()
{
    WT_ITEM item;
    get_metadata(MetadataKey::num_nodes, item, metadata_cursor);
    return *(uint32_t *)item.data;
}

uint64_t GraphBase::get_num_edges()
{
    WT_ITEM item;
    get_metadata(MetadataKey::num_edges, item, metadata_cursor);
    return *(uint64_t *)item.data;
}

// void GraphBase::set_locks(LockSet *locks_ptr) { locks = locks_ptr; }

// void GraphBase::add_to_nnodes(int amnt)
// {
//     if (locks != nullptr)
//     {
//         omp_set_lock(locks->get_node_num_lock());
//         set_num_nodes(get_num_nodes() + amnt, this->metadata_cursor);
//         omp_unset_lock(locks->get_node_num_lock());
//     }
//     else
//     {
//         set_num_nodes(get_num_nodes() + amnt, this->metadata_cursor);
//     }
// }

// void GraphBase::add_to_nedges(int amnt)
// {
//     if (locks != nullptr)
//     {
//         omp_set_lock(locks->get_edge_num_lock());
//         set_num_edges(get_num_edges() + amnt, this->metadata_cursor);
//         omp_unset_lock(locks->get_edge_num_lock());
//     }
//     else
//     {
//         set_num_edges(get_num_edges() + amnt, this->metadata_cursor);
//     }
//}
