#ifndef GRAPH_H
#define GRAPH_H
#include <wiredtiger.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

#include "common.h"
#include "lock.h"
#include "graph_exception.h"

class GraphBase
{
   public:
    GraphBase(graph_opts opts);  // ✅
    static void insert_metadata(const std::string key,
                                const char *value,
                                WT_CURSOR *metadata_cursor);  // ✅ check if pvt
    std::string get_metadata(std::string key, WT_CURSOR *metadata_cursor);  // ✅
    virtual node get_node(node_id_t node_id) = 0;  // ✅
    virtual node get_random_node() = 0;            // ✅
    static void create_new_graph(
        graph_opts &opts,
        WT_CONNECTION *conn);             // Used during first-time init of DB
    virtual void create_new_graph() = 0;  // ✅
    virtual void add_node(node to_insert) = 0;                         // ✅
    virtual bool has_node(node_id_t node_id) = 0;                      // ✅
    virtual void delete_node(node_id_t node_id) = 0;                   // ✅
    virtual void add_edge(edge to_insert, bool is_bulk) = 0;           // ✅
    virtual void delete_edge(node_id_t src_id, node_id_t dst_id) = 0;  // ✅
    virtual edge get_edge(node_id_t src_id, node_id_t dst_id) = 0;     // ✅
    // void update_edge(edge to_update); no need to implement.
    virtual std::vector<node> get_nodes() = 0;  // ✅
    virtual std::vector<edge> get_edges() = 0;  // ✅
    virtual bool has_edge(node_id_t src_id, node_id_t dst_id) = 0;

    virtual degree_t get_out_degree(node_id_t node_id) = 0;          // ✅
    virtual degree_t get_in_degree(node_id_t node_id) = 0;           // ✅
    virtual std::vector<edge> get_out_edges(node_id_t node_id) = 0;  // ✅
    virtual std::vector<node> get_out_nodes(node_id_t node_id) = 0;  // ✅

    virtual std::vector<node_id_t> get_out_nodes_id(node_id_t node_id) = 0;
    virtual std::vector<node_id_t> get_in_nodes_id(node_id_t node_id) = 0;

    virtual std::vector<edge> get_in_edges(node_id_t node_id) = 0;  // ✅
    virtual std::vector<node> get_in_nodes(node_id_t node_id) = 0;  // ✅

    virtual OutCursor *get_outnbd_iter() = 0;
    virtual InCursor *get_innbd_iter() = 0;
    virtual NodeCursor *get_node_iter() = 0;
    virtual EdgeCursor *get_edge_iter() = 0;

    virtual void set_locks(LockSet* locks_ptr) = 0;

    void close();
    uint64_t get_num_nodes();
    uint64_t get_num_edges();
    void set_num_nodes(uint64_t num_nodes, WT_CURSOR *cursor);
    void set_num_edges(uint64_t num_edges, WT_CURSOR *cursor);
    virtual void make_indexes() = 0;
    std::string get_db_name() const { return opts.db_name; };

    static int _get_table_cursor(std::string table,
                                 WT_CURSOR **cursor,
                                 WT_SESSION *session,
                                 bool is_random);

   protected:
    graph_opts opts;
    WT_CONNECTION *conn;
    WT_SESSION *session;
    LockSet* locks;

    WT_CONNECTION *get_db_conn() { return this->conn; }
    WT_SESSION *get_db_session() { return this->session; }
    int _get_index_cursor(std::string table_name,
                          std::string idx_name,
                          std::string projection,
                          WT_CURSOR **cursor);
    void __restore_from_db(std::string db_name);

    void drop_indices();
    void close_all_cursors();

    std::string get_metadata(const std::string &key);
};
#endif