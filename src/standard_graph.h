#ifndef STD_GRAPH
#define STD_GRAPH

#include "common.h"
#include "graph_exception.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;

class StandardGraph
{
public:
    // create params
    graph_opts opts;
    StandardGraph(graph_opts &opt_params);
    StandardGraph();
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
    bool has_edge(int src_id, int dst_id, edge *found);
    void delete_edge(int src_id, int dst_id);
    edge get_edge(int src_id, int dst_id); // todo <-- implement this
    std::vector<edge> get_edges();
    std::vector<edge> get_out_edges(int node_id);
    std::vector<node> get_out_nodes(int node_id);
    std::vector<edge> get_in_edges(int node_id);
    std::vector<node> get_in_nodes(int node_id);
    void close();
    std::string get_db_name() const { return opts.db_name; };
    void create_indices();

    // internal cursor methods
    //! Check if these should be public
    void init_cursors(); // todo <-- implement this
    WT_CURSOR *get_node_cursor();
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_src_idx_cursor();
    WT_CURSOR *get_dst_idx_cursor();
    WT_CURSOR *get_src_dst_idx_cursor();
    WT_CURSOR *get_node_iter();
    WT_CURSOR *get_edge_iter();
    std::vector<edge> test_cursor_iter(int node_id);

protected:
    WT_CONNECTION *conn;
    WT_SESSION *session;

    // Cursors
    WT_CURSOR *node_cursor = NULL;
    WT_CURSOR *random_node_cursor = NULL;
    WT_CURSOR *edge_cursor = NULL;
    WT_CURSOR *src_dst_index_cursor = NULL;
    WT_CURSOR *src_index_cursor = NULL;
    WT_CURSOR *dst_index_cursor = NULL;
    WT_CURSOR *metadata_cursor = NULL;

    // structure of the graph
    vector<string> node_columns = {ID}; //Always there :)
    vector<string> edge_columns = {SRC, DST};
    string node_value_format;
    string node_key_format = "I";
    string edge_key_format = "II";
    string edge_value_format = ""; //I if weighted or b if unweighted.

    string node_count = "nNodes";
    string edge_count = "nEdges";

    // Serialization methods
    void __node_to_record(WT_CURSOR *cursor, node to_insert);
    void __record_to_node(WT_CURSOR *cursor, node *found);
    void __record_to_edge(WT_CURSOR *cursor, edge *found);
    void __read_from_edge_idx(WT_CURSOR *cursor, edge *e_idx);

    // Internal methods
    int _get_table_cursor(string table, WT_CURSOR **cursor, bool is_random);
    int _get_index_cursor(std::string table_name, std::string idx_name,
                          std::string projection, WT_CURSOR **cursor);
    void drop_indices();
    void set_num_nodes(int numNodes);
    void set_num_edges(int numEdges);
    void delete_related_edges(WT_CURSOR *index_cursor, int node_id);
    void update_node_degree(WT_CURSOR *cursor, int node_id, int in_degree,
                            int out_degree);
    void update_edge_weight(int src_id, int dst_id, int edge_weight);
    node get_next_node(WT_CURSOR *n_iter);
    edge get_next_edge(WT_CURSOR *e_iter);

    // Metadata and restore operations:
    void insert_metadata(string key, string value_format, char *value);
    string get_metadata(string key);
    void __restore_from_db(string db_name);

    WT_CONNECTION *get_db_conn() { return this->conn; }
};

#endif