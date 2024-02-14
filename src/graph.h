#ifndef GRAPH_H
#define GRAPH_H
#include <wiredtiger.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

#include "common_util.h"
#include "graph_exception.h"
#include "lock.h"

class GraphBase
{
   public:
    GraphBase() = default;
    GraphBase(graph_opts &opt_params, WT_CONNECTION *conn);

    //    virtual ~GraphBase() { CommonUtil::close_connection(this->connection);
    //    }

    static void insert_metadata(int key,
                                const char *value,
                                size_t size,
                                WT_CURSOR *cursor);
    void get_metadata(int key, WT_ITEM &item, WT_CURSOR *metadata_cursor);
    virtual node get_node(node_id_t node_id) = 0;
    virtual node get_random_node() = 0;
    static void create_metadata_table(
        graph_opts &opts,
        WT_CONNECTION *conn);  // Used during first-time init of DB
    virtual int add_node(node to_insert) = 0;
    virtual bool has_node(node_id_t node_id) = 0;
    virtual int delete_node(node_id_t node_id) = 0;
    virtual int add_edge(edge to_insert, bool is_bulk) = 0;
    virtual int delete_edge(node_id_t src_id, node_id_t dst_id) = 0;
    virtual edge get_edge(node_id_t src_id, node_id_t dst_id) = 0;
    // void update_edge(edge to_update); no need to implement.
    virtual std::vector<node> get_nodes() = 0;
    virtual std::vector<edge> get_edges() = 0;
    virtual bool has_edge(node_id_t src_id, node_id_t dst_id) = 0;

    virtual degree_t get_out_degree(node_id_t node_id) = 0;
    virtual degree_t get_in_degree(node_id_t node_id) = 0;
    virtual std::vector<edge> get_out_edges(node_id_t node_id) = 0;
    virtual std::vector<node> get_out_nodes(node_id_t node_id) = 0;

    virtual std::vector<node_id_t> get_out_nodes_id(node_id_t node_id) = 0;
    virtual std::vector<node_id_t> get_in_nodes_id(node_id_t node_id) = 0;

    virtual std::vector<edge> get_in_edges(node_id_t node_id) = 0;
    virtual std::vector<node> get_in_nodes(node_id_t node_id) = 0;

    virtual OutCursor *get_outnbd_iter() = 0;
    virtual InCursor *get_innbd_iter() = 0;
    virtual NodeCursor *get_node_iter() = 0;
    virtual EdgeCursor *get_edge_iter() = 0;

    // void set_locks(LockSet *locks_ptr);

    void close();
    uint32_t get_num_nodes();
    uint64_t get_num_edges();
    void set_num_nodes(uint64_t num_nodes, WT_CURSOR *cursor);
    void set_num_edges(uint64_t num_edges, WT_CURSOR *cursor);
    [[nodiscard]] std::string get_db_name() const { return opts.db_name; };

   protected:
    graph_opts opts;
    int local_nnodes = 0;
    int local_nedges = 0;
    WT_CONNECTION *connection = nullptr;
    WT_SESSION *session = nullptr;
    LockSet *locks = nullptr;

    WT_CURSOR *metadata_cursor = nullptr;

    [[maybe_unused]] WT_CONNECTION *get_db_conn() { return this->connection; }
    [[maybe_unused]] WT_SESSION *get_db_session() { return this->session; }
    static int _get_table_cursor(const std::string &table,
                                 WT_CURSOR **cursor,
                                 WT_SESSION *session,
                                 bool is_random,
                                 bool prevent_overwrite);
    int _get_index_cursor(const std::string &table_name,
                          const std::string &idx_name,
                          const std::string &projection,
                          WT_CURSOR **cursor);
    [[maybe_unused]] void _restore_from_db();
    [[maybe_unused]] void _restore_from_db(const std::string &db_name);

    virtual void close_all_cursors() = 0;
};
#endif