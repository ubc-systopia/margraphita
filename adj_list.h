#ifndef STD_GRAPH
#define STD_GRAPH

#include "common.h"
#include "graph_exception.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;

class AdjList
{
public:
    AdjList(opt_args opt_params);
    AdjList();

    void create_new_graph();

    void add_node(node to_insert);
    bool has_node(int node_id);
    node get_node(int node_id);
    void delete_node(int node_id);
    node get_random_node();
    int get_in_degree(int node_id);
    int get_out_degree(int node_id);
    std::vector<node> get_nodes();

    int get_num_nodes();
    int get_num_edges();

    void add_edge(edge to_insert);
    bool has_edge(int src_id, int dst_id);
    int get_edge_weight(int src_id, int dst_id);
    void delete_edge(int src_id, int dst_id);
    void update_edge_weight(int src_id, int dst_id, int edge_weight);

    void add_adjlist(WT_CURSOR *cursor, int node_id);
    void delete_adjlist(WT_CURSOR *cursor, int node_id);
    void delete_node_from_adjlists(int node_id);
    std::vector<int> get_adjlist(WT_CURSOR *cursor, int node_id);
    void add_to_adjlists(WT_CURSOR *cursor, int node_id, int to_insert);
    void delete_from_adjlists(WT_CURSOR *cursor, int node_id, int to_delete);

    void delete_related_edges(WT_CURSOR *index_cursor, int node_id);

    void update_node_degree(WT_CURSOR *cursor, int node_id, int in_degree,
                            int out_degree);
    edge get_edge(int src_id, int dst_id);
    // void update_edge(int src_id, int dst_id, char *new_attrs); <-not needed.
    // skip.
    std::vector<edge> get_edges();
    std::vector<edge> get_out_edges(int node_id);
    std::vector<node> get_out_nodes(int node_id);
    std::vector<edge> get_in_edges(int node_id);
    std::vector<node> get_in_nodes(int node_id);
    //! void set_node_data(int node_id, int idx, string data);
    //! Do we need get all weights of in/out degree from a node
    std::string get_node_data(int node_id, int idx);
    void get_node_iter(); //! APT Check
    void get_edge_iter(); //! APT Check
    // Metadata operations:
    void insert_metadata(string key, string value_format, char *value);
    string get_metadata(string key);
    void close();

    // WT privates
private:
    WT_CONNECTION *conn;
    WT_SESSION *session;
    //create params`
    bool create_new = true;
    bool read_optimize = true;
    bool is_directed = true;
    bool is_weighted = true; //needed to understand when to interpret the weight field in struct edge
    std::string db_name;

    //structure of the graph
    int edge_id;
    int node_attr_size = 0; // set on checking the list len

    vector<string> node_columns = {ID}; //Always there :)
    vector<string> edge_columns = {SRC, DST};
    string node_value_format;
    string node_key_format = "I";
    string edge_key_format = "II";  // SRC DST in the edge table
    string edge_value_format = "I"; // Edge Weight in the edge table

    WT_CURSOR *node_cursor = NULL;
    WT_CURSOR *random_node_cursor = NULL;
    WT_CURSOR *edge_cursor = NULL;
    WT_CURSOR *src_dst_index_cursor = NULL; //! APT Check
    WT_CURSOR *src_index_cursor = NULL;
    WT_CURSOR *dst_index_cursor = NULL;
    WT_CURSOR *metadata_cursor = NULL;

    void __node_to_record(WT_CURSOR *cursor, node to_insert);  //! APT Check
    node __record_to_node(WT_CURSOR *cursor);                  //! APT Check
    edge __record_to_edge(WT_CURSOR *cursor);                  //! APT Check
    void __read_from_edge_idx(WT_CURSOR *cursor, edge *e_idx); //! APT Check

    // Internal cursor methods
    int _get_table_cursor(string table, WT_CURSOR **cursor, bool is_random); //! APT Check
    int _get_index_cursor(std::string table_name, std::string idx_name,
                          std::string projection, WT_CURSOR **cursor); //! APT Check
    void __restore_from_db(string db_name);
    void create_indices();
    void drop_indices();
};

#endif