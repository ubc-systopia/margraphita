#ifndef EDGE_KEY
#define EDGE_KEY

#include "common.h"
#include "graph_exception.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;

class EdgeKey
{
public:
    graph_opts opts;
    EdgeKey(graph_opts opt_params);
    EdgeKey();
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
    void add_edge(edge to_insert, bool is_bulk);
    bool has_edge(int src_id, int dst_id);
    void delete_edge(int src_id, int dst_id);
    edge get_edge(int src_id, int dst_id);
    std::vector<edge> get_edges();
    std::vector<edge> get_out_edges(int node_id);
    std::vector<node> get_out_nodes(int node_id);
    std::vector<edge> get_in_edges(int node_id);
    std::vector<node> get_in_nodes(int node_id);
    void close();
    std::string get_db_name() const { return opts.db_name; };

    void create_indices();
    // internal cursor operations:
    void init_cursors(); // todo <-- implement this
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_src_idx_cur();
    WT_CURSOR *get_dst_idx_cur();
    WT_CURSOR *get_node_iter();
    WT_CURSOR *get_edge_iter();

private:
    WT_CONNECTION *conn;
    WT_SESSION *session;

    //Cursors
    WT_CURSOR *edge_cursor = nullptr;
    WT_CURSOR *metadata_cursor = nullptr;
    WT_CURSOR *src_idx_cursor = nullptr;
    WT_CURSOR *dst_idx_cursor = nullptr;

    //structure of the graph
    vector<string> edge_columns = {SRC, DST, ATTR};
    string edge_key_format = "ii";  // SRC DST
    string edge_value_format = "S"; //Packed binary

    // internal methods
    void delete_related_edges(WT_CURSOR *idx_cursor, WT_CURSOR *edge_cur, int node_id);
    void update_node_degree(int node_id, int indeg, int outdeg);
    node get_next_node(WT_CURSOR *n_iter);
    edge get_next_edge(WT_CURSOR *e_iter);
    int _get_table_cursor(string table, WT_CURSOR **cursor, bool is_random);
    int _get_index_cursor(std::string table_name,
                          std::string idx_name,
                          std::string projection,
                          WT_CURSOR **cursor);
    void drop_indices();

    // Serialization Methods
    void __record_to_node(WT_CURSOR *cur, node *found);
    void __record_to_edge(WT_CURSOR *cur, edge *found);
    void extract_from_string(string packed_str, int *a, int *b);
    string pack_int_to_str(int a, int b);

    // metadata and restore operations
    void insert_metadata(string key, char *value);
    string get_metadata(string key);
    void __restore_from_db(string db_name);

    void close_all_cursors();
};
#endif