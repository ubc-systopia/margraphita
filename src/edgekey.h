#ifndef EDGE_KEY
#define EDGE_KEY

#include "common.h"
#include "graph_exception.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;

class EdgeKey
{
public:
    EdgeKey(graph_opts opt_params);
    EdgeKey();

    // Metadata operations:
    void insert_metadata(string key, char *value);
    string get_metadata(string key);
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_src_idx_cur();
    WT_CURSOR *get_dst_idx_cur();
    node get_node(int node_id);
    node get_random_node();
    void create_new_graph();
    void add_node(node to_insert);
    bool has_node(int node_id);
    void delete_node(int node_id);
    void delete_related_edges(WT_CURSOR *idx_cursor, WT_CURSOR *edge_cur, int node_id);
    void update_node_degree(int node_id, int indeg, int outdeg);
    void add_edge(edge to_insert);
    void delete_edge(int src_id, int dst_id);
    edge get_edge(int src_id, int dst_id);
    //void update_edge(edge to_update); no need to implement.
    std::vector<node> get_nodes();
    std::vector<edge> get_edges();

    bool has_edge(int src_id, int dst_id);
    void get_num_nodes_and_edges(int *node_count, int *edge_count);
    int get_out_deg(int node_id);
    int get_in_deg(int node_id);
    std::vector<edge> get_out_edges(int node_id);
    std::vector<node> get_out_nodes(int node_id);
    std::vector<edge> get_in_edges(int node_id);
    std::vector<node> get_in_nodes(int node_id);
    void close();
    //----------------^DONE

private:
    WT_CONNECTION *conn;
    WT_SESSION *session;
    //create params
    bool create_new = true;
    bool read_optimize = true;
    bool is_directed = true;
    bool is_weighted = false; //needed to understand when to interpret the weight field in struct edge
    std::string db_name;

    //Cursors
    WT_CURSOR *edge_cursor = nullptr;
    WT_CURSOR *metadata_cursor = nullptr;
    WT_CURSOR *src_idx_cursor = nullptr;
    WT_CURSOR *dst_idx_cursor = nullptr;

    //structure of the graph
    int edge_id;
    int node_attr_size = 0; // set on checking the list len

    vector<string> edge_columns = {SRC, DST, ATTR};
    string edge_key_format = "ii";  // SRC DST
    string edge_value_format = "S"; //Packed binary

    int _get_table_cursor(string table, WT_CURSOR **cursor, bool is_random);
    int _get_index_cursor(std::string table_name,
                          std::string idx_name,
                          std::string projection,
                          WT_CURSOR **cursor);
    void __restore_from_db(string db_name);
    void create_indices();
    void drop_indices();
    void __record_to_node(WT_CURSOR *cur, node *found);
    void __record_to_edge(WT_CURSOR *cur, edge *found);
    void extract_from_string(string packed_str, int *a, int *b);
    string pack_int_to_str(int a, int b);
};
#endif