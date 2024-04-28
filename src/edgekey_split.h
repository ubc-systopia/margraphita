#ifndef EDGELIST_H
#define EDGELIST_H

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common_util.h"
#include "graph.h"
#include "graph_exception.h"

class SplitEdgeKey : public GraphBase
{
   public:
    SplitEdgeKey(graph_opts &opt_params,
                 WT_CONNECTION *connection);  // TODO: merge the 2 constructors
    static void create_wt_tables(graph_opts &opts, WT_CONNECTION *conn);
    int add_node(node to_insert) override;

    bool has_node(node_id_t node_id) override;
    node get_node(node_id_t node_id) override;
    int delete_node(node_id_t node_id) override;
    node get_random_node() override;
    void get_random_node_ids(std::vector<node_id_t> &ids,
                             int num_nodes) override;
    degree_t get_in_degree(node_id_t node_id) override;
    degree_t get_out_degree(node_id_t node_id) override;
    std::vector<node> get_nodes() override;
    int add_edge(edge to_insert, bool is_bulk) override;
    bool has_edge(node_id_t src_id, node_id_t dst_id) override;
    int delete_edge(node_id_t src_id, node_id_t dst_id) override;
    edge get_edge(node_id_t src_id, node_id_t dst_id) override;
    std::vector<edge> get_edges() override;
    std::vector<edge> get_out_edges(node_id_t node_id) override;
    std::vector<node> get_out_nodes(node_id_t node_id) override;
    std::vector<node_id_t> get_out_nodes_id(node_id_t node_id) override;
    std::vector<edge> get_in_edges(node_id_t node_id) override;
    std::vector<node> get_in_nodes(node_id_t node_id) override;
    std::vector<node_id_t> get_in_nodes_id(node_id_t node_id) override;

    node_id_t get_max_node_id() override;
    node_id_t get_min_node_id() override;

    OutCursor *get_outnbd_iter() override;
    InCursor *get_innbd_iter() override;
    NodeCursor *get_node_iter() override;
    EdgeCursor *get_edge_iter() override;

    // internal cursor operations:
    void init_cursors();  // todo <-- implement this

   private:
    // Cursors
    WT_CURSOR *out_edge_cursor = nullptr;

   public:
    WT_CURSOR *get_out_edge_cursor() const { return out_edge_cursor; }
    WT_CURSOR *get_random_node_cursor() const { return random_node_cursor; }
    WT_CURSOR *get_in_edge_cursor() const { return in_edge_cursor; }
    WT_CURSOR *get_node_index_cursor() const { return dst_src_idx_cursor; }

   private:
    WT_CURSOR *random_node_cursor = nullptr;
    WT_CURSOR *in_edge_cursor = nullptr;
    WT_CURSOR *dst_src_idx_cursor = nullptr;

    // internal methods
    [[maybe_unused]] WT_CURSOR *get_metadata_cursor();
    int delete_node_and_related_edges(node_id_t node_id, int *num_edges_to_del);
    int update_node_degree(node_id_t node_id, int in_change, int out_change);
    int add_edge_only(edge to_insert);
    int add_node_txn(node to_insert);
    int error_check_add_edge(int ret);
    int error_check_insert_txn(int return_val);

    [[maybe_unused]] inline void close_all_cursors() override
    {
        out_edge_cursor->close(out_edge_cursor);
        random_node_cursor->close(random_node_cursor);
        in_edge_cursor->close(in_edge_cursor);
    }
};

class SplitEkeyInCursor : public InCursor
{
   public:
    SplitEkeyInCursor(WT_CURSOR *cursor, node_id_t node_id)
        : cursor(cursor), node_id(node_id)
    {
        cursor->set_key(cursor, node_id);
        cursor->search(cursor);
    }

    void next(adjlist *nbd) override
    {
        WT_ITEM item;
        cursor->get_value(cursor, &item);
        *nbd = *(adjlist *)item.data;
        cursor->next(cursor);
    }

    void reset() override
    {
        cursor->set_key(cursor, node_id);
        cursor->search(cursor);
    }

   private:
    WT_CURSOR *cursor;
    node_id_t node_id;
};

class SplitEKeyOutCursor : public OutCursor
{
   public:
    SplitEKeyOutCursor(WT_CURSOR *cursor, node_id_t node_id)
        : cursor(cursor), node_id(node_id)
    {
        cursor->set_key(cursor, node_id);
        cursor->search(cursor);
    }

    void next(adjlist *nbd) override
    {
        WT_ITEM item;
        cursor->get_value(cursor, &item);
        *nbd = *(adjlist *)item.data;
        cursor->next(cursor);
    }

    void reset() override
    {
        cursor->set_key(cursor, node_id);
        cursor->search(cursor);
    }

   private:
    WT_CURSOR *cursor;
    node_id_t node_id;
};

class SplitEKeyNodeCursor : public NodeCursor
{
   public:
    SplitEKeyNodeCursor(WT_CURSOR *cursor, node_id_t num_nodes)
        : cursor(cursor), num_nodes(num_nodes)
    {
        cursor->set_key(cursor, 0);
        cursor->search(cursor);
    }

    void next(node *found) override
    {
        WT_ITEM item;
        cursor->get_value(cursor, &item);
        *found = *(node *)item.data;
        cursor->next(cursor);
    }

   private:
    WT_CURSOR *cursor;
    node_id_t num_nodes;
};

class SplitEdgeKeyEdgeCursor : public EdgeCursor
{
   public:
    SplitEdgeKeyEdgeCursor(WT_CURSOR *cursor, node_id_t num_edges)
        : cursor(cursor), num_edges(num_edges)
    {
        cursor->set_key(cursor, 0);
        cursor->search(cursor);
    }

    void next(edge *found) override
    {
        WT_ITEM item;
        cursor->get_value(cursor, &item);
        *found = *(edge *)item.data;
        cursor->next(cursor);
    }

   private:
    WT_CURSOR *cursor;
    node_id_t num_edges;
};

#endif