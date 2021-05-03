#include "adj_list.h"
#include "adj_common.h"
#include <filesystem>
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

AdjList::AdjList() {}

AdjList::AdjList(opt_args opt_params)

{
    this->create_new = opt_params.create_new;
    this->read_optimize = opt_params.read_optimize;
    this->is_directed = opt_params.is_directed;
    this->is_weighted = opt_params.is_weighted;
    this->db_name = opt_params.db_name;

    try
    {
        CommonUtil::check_graph_params(opt_params);
    }
    catch (GraphException G)
    {
        std::cout << G.what() << std::endl;
    }

    if (opt_params.create_new == false)
    {
        throw GraphException("Missing param " + CREATE_NEW);
    }
    if (opt_params.create_new)
    {
        create_new_graph();
        if (opt_params.optimize_create == false)
        {
            /**
       *  Create indices with graph.
       * NOTE: If user opts to optimize graph creation, then they're
       * responsible for calling create_indices() AFTER all the
       * nodes/edges have been added to graph.
       * This is done to improve performance of bulk-loading.
       */
        }
    }
    else
    {
        __restore_from_db(db_name); //! APT Check towards the end by a unit test
    }
}

/**
 * @brief Returns the metadata associated with the key param from the METADATA
 * table.
 * Same from standard_graph implementation
 */
string AdjList::get_metadata(string key)
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
    metadata_cursor->close(metadata_cursor); //? Should I close this?

    return string(value);
}

void AdjList::create_new_graph()
{
    int ret;
    // Create new directory for WT DB
    std::filesystem::path dirname = "./db/" + db_name;
    if (std::filesystem::exists(dirname))
    {
        filesystem::remove_all(dirname); // remove if exists;
    }
    std::filesystem::create_directories(dirname);

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
    edge_id = -999;

    // Set up the node table
    // The node entry is of the form: <id>,<in_degree>,<out_degree>
    // If the graph is read_optimized, add columns and format for in/out degrees
    if (read_optimize)
    {
        node_columns.push_back(IN_DEGREE);
        node_columns.push_back(OUT_DEGREE);
        node_value_format = "II";
    }
    else
    {
        node_columns.push_back("na"); //have to do this because the column count must match.
        node_value_format = "s";      //1 byte fixed length char[] to hold ""
    }
    // Now Create the Node Table
    //!What happens when the table is not read-optimized? I store "" <-ok?

    CommonUtil::set_table(session, NODE_TABLE, node_columns, node_key_format,
                          node_value_format);

    // ******** Now set up the Edge Table     **************
    // Edge Column Format : <id><src><dst><weight>
    //Now prepare the edge value format. starts with II for src,dst. Add another I if weighted
    if (is_weighted)
    {
        edge_columns.push_back(WEIGHT);
        edge_value_format += "I";
    }

    // Create edge table
    CommonUtil::set_table(session, EDGE_TABLE, edge_columns,
                          edge_key_format, edge_value_format);

    /* Now doing the metadata table creation.
     function This table stores the graph metadata
     value_format:raw byte string (Su)
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
    // char *db_name_packed =
    //     CommonUtil::pack_string_wt(db_name, session, &db_name_fmt);
    // insert_metadata(DB_NAME, db_name_fmt, db_name_packed);
    insert_metadata(DB_NAME, const_cast<char *>(db_name.c_str()));

    // READ_OPTIMIZE
    string read_optimized_str = read_optimize ? "true" : "false";
    insert_metadata(READ_OPTIMIZE, const_cast<char *>(read_optimized_str.c_str()));

    // IS_DIRECTED
    string is_directed_str = is_directed ? "true" : "false";
    insert_metadata(IS_DIRECTED, const_cast<char *>(is_directed_str.c_str()));

    //IS_WEIGHTED
    string is_weighted_str = is_weighted ? "true" : "false";
    insert_metadata(IS_WEIGHTED, const_cast<char *>(is_weighted_str.c_str()));

    this->metadata_cursor->close(this->metadata_cursor);
}

/**
 * @brief This private function inserts metadata values into the metadata
 * table. The fields are self explanatory.
 * Same from standard_graph implementation.
 */
void AdjList::insert_metadata(string key, char *value)
{
    this->metadata_cursor->set_key(metadata_cursor, key.c_str());
    this->metadata_cursor->set_value(metadata_cursor, value);
    if ((ret = this->metadata_cursor->insert(metadata_cursor)) != 0)
    {
        fprintf(stderr, "failed to insert metadata for key %s", key.c_str());
        // TODO(puneet): Maybe create a GraphException?
    }
}