#ifndef EDGELIST_H
#define EDGELIST_H

#include <wiredtiger.h>

#include <iostream>
#include <source_location>
#include <string>
#include <unordered_map>

#include "common.h"
#include "graph.h"
#include "graph_exception.h"

class UnOrderedEdgeList : public GraphBase
{
   public:
    UnOrderedEdgeList(
        graph_opts &opt_params,
        WT_CONNECTION *connection);  // TODO: merge the 2 constructors
    static void create_wt_tables(
        graph_opts &opts, WT_CONNECTION *conn);  // Need this to init graph db
    int add_node(node to_insert);
    bool has_node(node_id_t node_id) override;
    int delete_node(node_id_t node_id) override { return 0; };
    int add_edge(edge to_insert, bool is_bulk) override;
    int delete_edge(node_id_t src_id, node_id_t dst_id) override { return 0; }
    edge get_edge(node_id_t src_id, node_id_t dst_id) override;
    std::vector<node> get_nodes() override;
    std::vector<edge> get_edges() override;
    bool has_edge(node_id_t src_id, node_id_t dst_id) override;
    
    int add_node_txn(node to_insert);

    node get_node(node_id_t node_id) override;
    node get_random_node() override;
    degree_t get_in_degree(node_id_t node_id) override { return 0; };
    degree_t get_out_degree(node_id_t node_id) override { return 0; };

    std::vector<edge> get_out_edges(node_id_t node_id) override;
    std::vector<node> get_out_nodes(node_id_t node_id) override;

    std::vector<node_id_t> get_out_nodes_id(node_id_t node_id) override;
    std::vector<edge> get_in_edges(node_id_t node_id) override;
    std::vector<node> get_in_nodes(node_id_t node_id) override;
    std::vector<node_id_t> get_in_nodes_id(node_id_t node_id) override;

    std::string get_db_name() const { return opts.db_name; }

    int update_node_degree(WT_CURSOR *cursor,
                           node_id_t node_id,
                           degree_t in_degree,
                           degree_t out_degree);
    OutCursor *get_outnbd_iter() override { return nullptr; };
    InCursor *get_innbd_iter() override { return nullptr; };
    NodeCursor *get_node_iter() override { return nullptr; };
    EdgeCursor *get_edge_iter() override { return nullptr; };

    // internal cursor operations:
    //! Check if these should be public:
    void init_cursors();
    void drop_indices();
    static void create_indices(WT_SESSION *sess);

   private:
    // structure of the graph
    int node_attr_size = 0;  // set on checking the list len

    WT_CURSOR *node_cursor = NULL;
    WT_CURSOR *random_node_cursor = NULL;
    WT_CURSOR *edge_cursor = NULL;
    WT_CURSOR *src_index_cursor = NULL;
    WT_CURSOR *dst_index_cursor = NULL;
    WT_CURSOR *src_dst_index_cursor = NULL;

    // AdjList specific internal methods:

    void create_indices();  // here because defined in interface

    int error_check_insert_txn(int return_val, bool ignore_duplicate_key);
    int error_check_update_txn(int return_val);
    int error_check_read_txn(int return_val);
    int error_check_remove_txn(int return_val);
    int error_check(int ret, std::source_location loc);
};
#endif