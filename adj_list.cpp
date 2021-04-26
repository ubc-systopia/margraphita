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
const std::string GRAPH_PREFIX = "std";

AdjList::AdjList() {}

StandardGraph::StandardGraph(opt_args opt_params)

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
            create_indices(); //! APT Check the indices
        }
    }
    else
    {
        __restore_from_db(db_name); //! APT Check towards the end by a unit test
    }
}

//! APT : create_graph()

/**
 * @brief This private function inserts metadata values into the metadata
 * table. The fields are self explanatory.
 * Same from standard_graph implementation.
 */
void StandardGraph::insert_metadata(string key, string value_format,
                                    char *value)
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
    this->metadata_cursor->close(metadata_cursor); //? Should I close this?
}

/**
 * @brief Returns the metadata associated with the key param from the METADATA
 * table.
 * Same from standard_graph implementation
 */
string StandardGraph::get_metadata(string key)
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
