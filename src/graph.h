
#include <wiredtiger.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

#include "common.h"
#include "graph_exception.h"

class GraphBase
{
   public:
    GraphBase(graph_opts opts);
    GraphBase();
    void insert_metadata(std::string key, char *value);
    std::string get_metadata(std::string key);
    node get_node(int node_id);
    node get_random_node();
    void create_new_graph();
    void add_node(node to_insert);
    bool has_node(int node_id);
    void delete_node(int node_id);
    void delete_related_edges(WT_CURSOR *idx_cursor,
                              WT_CURSOR *edge_cur,
                              int node_id);
    void update_node_degree(int node_id, int indeg, int outdeg);
    void add_edge(edge to_insert);
    void bulk_add_edge(int src, int dst, int weight);
    void delete_edge(int src_id, int dst_id);
    edge get_edge(int src_id, int dst_id);
    // void update_edge(edge to_update); no need to implement.
    std::vector<node> get_nodes();
    std::vector<edge> get_edges();
    bool has_edge(int src_id, int dst_id);
    int get_num_edges();
    int get_num_nodes();

    int get_out_degree(int node_id);
    int get_in_degree(int node_id);
    std::vector<edge> get_out_edges(int node_id);
    std::vector<node> get_out_nodes(int node_id);
    std::vector<edge> get_in_edges(int node_id);
    std::vector<node> get_in_nodes(int node_id);
    void close();
    //----------------^DONE
    void create_indices();
    WT_CURSOR *get_node_iter();
    node get_next_node(WT_CURSOR *n_iter);
    WT_CURSOR *get_edge_iter();
    edge get_next_edge(WT_CURSOR *e_iter);
    void close_all_cursors();
    std::string get_db_name() const { return opts.db_name; };

   protected:
    WT_CONNECTION *conn;
    WT_SESSION *session;
    // create params
    graph_opts opts;

    int _get_table_cursor(std::string table,
                          WT_CURSOR **cursor,
                          bool is_random);
    int _get_index_cursor(std::string table_name,
                          std::string idx_name,
                          std::string projection,
                          WT_CURSOR **cursor);
    void __restore_from_db(std::string db_name);

    void drop_indices();
};