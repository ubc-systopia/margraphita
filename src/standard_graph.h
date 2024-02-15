#ifndef STD_GRAPH
#define STD_GRAPH

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common_util.h"
#include "graph.h"
#include "graph_exception.h"

using namespace std;

class StdInCursor : public InCursor
{
   private:
    edge curr_edge;
    bool more_edges = true;
    // WT_CURSOR *node_cursor = nullptr;

   public:
    /**
     * @param cur: The cursor for finding nodes that have an incoming edge. In
     * this case, the cursor is the dst_idx_cursor
     * @param sess :The session for the cursor. It is used to open a cursor to
     * the node table
     */

    StdInCursor(WT_CURSOR *cur, WT_SESSION *sess) : InCursor(cur, sess) {}

    void no_next(adjlist *found)
    {
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }

    void set_key_range(key_range _keys) override
    {
        keys = _keys;
        if (_keys.end == 0)
        {
            keys.end = INT32_MAX;
        }

        CommonUtil::set_key(
            cursor, keys.start);  // set the key to the start of the range
        int status;
        cursor->search_near(cursor, &status);
        if (status < 0)
        {
            // Advances the cursor
            if (cursor->next(cursor) != 0)
            {
                has_next = false;
                return;
            }
        }
        CommonUtil::get_key(cursor, &curr_edge.dst_id);
    }

    void next(adjlist *found) final
    {
        node_id_t src;
        node_id_t dst;

        if (!has_next)
        {
            goto no_next;
        }

        CommonUtil::get_val_idx(cursor, &src, &dst);

        if (keys.end != UINT32_MAX && dst > keys.end)
        {
            goto no_next;
        }

        found->node_id = dst;
        do
        {
            CommonUtil ::read_from_edge_idx(cursor, &curr_edge);
            if (dst == curr_edge.dst_id)
            {
                found->degree++;
                found->edgelist.push_back(curr_edge.src_id);
            }
            else
            {
                curr_edge.dst_id = dst;
                return;
            }
        } while (cursor->next(cursor) == 0);
        has_next = false;
        return;

    no_next:
        found->degree = UINT32_MAX;
        found->edgelist.clear();
        found->node_id = UINT32_MAX;
        has_next = false;
    }

    void next(adjlist *found, node_id_t key) override {}
};

class StdOutCursor : public OutCursor
{
    node_id_t curr_edge_src, unused;
    bool more_edges = true;

   public:
    StdOutCursor(WT_CURSOR *cur, WT_SESSION *sess) : OutCursor(cur, sess) {}

    void no_next(adjlist *found)
    {
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }

    void set_key_range(key_range _keys) override
    {
        keys = _keys;
        if (_keys.end == 0)
        {
            keys.end = INT32_MAX;
        }

        CommonUtil::set_key(
            cursor, keys.start, 0);  // set the key to the start of the range
        int status;
        cursor->search_near(cursor, &status);
        if (status < 0)
        {
            // Advances the cursor
            if (cursor->next(cursor) != 0)
            {
                has_next = false;
                return;
            }
        }
        CommonUtil::get_key(cursor, &curr_edge_src, &unused);
    }

    void next(adjlist *found) override
    {
        node_id_t temp_src, temp_dst;

        if (!has_next)
        {
            no_next(found);
            return;
        }

        CommonUtil::get_key(cursor, &curr_edge_src, &temp_dst);

        found->node_id = curr_edge_src;
        found->edgelist.push_back(temp_dst);
            found->degree++;

        // advance the edge cursor till we hit a new src
        while (cursor->next(cursor) == 0)
        {
            CommonUtil::get_key(cursor, &temp_src, &temp_dst);
            if (temp_src == curr_edge_src)
            {
                found->edgelist.push_back(temp_dst);
                found->degree++;
            }
            else
            {
                curr_edge_src = temp_src;
                return;
            }
        }
        has_next = false;
    }

    void next(adjlist *found, node_id_t key) override {}
};

class StdNodeCursor : public NodeCursor
{
   public:
    StdNodeCursor(WT_CURSOR *cur, WT_SESSION *sess) : NodeCursor(cur, sess) {}

    void set_key_range(key_range _keys) override
    {
        keys = _keys;
        is_first = false;

        // Advances the cursor to the first valid record in range
        if (keys.start != UINT32_MAX)
        {
            int status;
            CommonUtil::set_key(cursor, keys.start);
            cursor->search_near(cursor, &status);
            if (status < 0)
            {
                // Advances the cursor
                if (cursor->next(cursor) != 0)
                {
                    this->has_next = false;
                }
            }
        }
        else
        {
            // Advances the cursor to the first position in the table.
            if (cursor->next(cursor) != 0)
            {
                this->has_next = false;
            }
        }
    }
    void no_next(node *found)
    {
        found->id = UINT32_MAX;
        found->in_degree = UINT32_MAX;
        found->out_degree = UINT32_MAX;
        has_next = false;
    }

    void next(node *found, node_id_t key) override {}

    void next(node *found) override
    {
        if (!has_next)
        {
            no_next(found);
            return;
        }

        CommonUtil::get_key(cursor, &found->id);
        if (keys.end != UINT32_MAX &&
            found->id > keys.end)  // gone beyond the end of range
        {
            no_next(found);
            return;
        }

        CommonUtil::record_to_node(cursor, found, true);
        if (cursor->next(cursor) != 0)
        {
            has_next = false;
        }
    }
};

class StdEdgeCursor : public EdgeCursor
{
   public:
    StdEdgeCursor(WT_CURSOR *cur, WT_SESSION *sess) : EdgeCursor(cur, sess) {}

    void set_key_range(edge_range range) override
    {
        start_edge = range.start;
        end_edge = range.end;
        is_first = false;

        // Advances the cursor to the first valid record in range
        if (start_edge.src_id != UINT32_MAX && start_edge.dst_id != UINT32_MAX)
        {
            int status;
            CommonUtil::set_key(cursor, start_edge.src_id, start_edge.dst_id);
            cursor->search_near(cursor, &status);
            if (status < 0)
            {
                // Advances the cursor
                if (cursor->next(cursor) != 0)
                {
                    this->has_next = false;
                }
            }
        }
        else
        {
            // Advances the cursor to the first position in the table.
            if (cursor->next(cursor) != 0)
            {
                this->has_next = false;
            }
        }
        // //dump the set key
        // node_id_t src,dst;
        // CommonUtil::get_key(cursor, &src, &dst);
        // std::cout<<"set key: "<<src<<" "<<dst<<std::endl;
    }

    void no_next(edge *found)
    {
        found->src_id = UINT32_MAX;
        found->dst_id = UINT32_MAX;
        found->edge_weight = UINT32_MAX;
        has_next = false;
    }

    void next(edge *found) override
    {
        if (!has_next)
        {
            no_next(found);
            return;
        }

        CommonUtil::get_key(cursor, &found->src_id, &found->dst_id);

        // If end_edge is set
        if (end_edge.src_id != -1)
        {
            // If found > end edge
            if ((found->src_id > end_edge.src_id ||
                 ((found->src_id == end_edge.src_id) &&
                  (found->dst_id >= end_edge.dst_id))))
            {
                no_next(found);
            }
        }

        CommonUtil::record_to_edge(cursor, found);
        if (cursor->next(cursor) != 0)
        {
            has_next = false;
        }
    }
};

class StandardGraph : public GraphBase
{
   public:
    // create params
    StandardGraph(graph_opts &opt_params,
                  WT_CONNECTION *conn);  // TODO: merge the 2 constructors
    static void create_wt_tables(graph_opts &opts, WT_CONNECTION *conn);
    int add_node(node to_insert) override;
    bool has_node(node_id_t node_id) override;
    node get_node(node_id_t node_id) override;
    int delete_node(node_id_t node_id) override;
    node get_random_node() override;
    degree_t get_in_degree(node_id_t node_id) override;
    degree_t get_out_degree(node_id_t node_id) override;
    std::vector<node> get_nodes() override;
    int add_edge(edge to_insert, bool is_bulk) override;
    bool has_edge(node_id_t src_id, node_id_t dst_id) override;
    int delete_edge(node_id_t src_id, node_id_t dst_id) override;
    int update_edge_weight(node_id_t src_id,
                           node_id_t dst_id,
                           edgeweight_t weight);
    edge get_edge(node_id_t src_id,
                  node_id_t dst_id) override;  // todo <-- implement this
    std::vector<edge> get_edges() override;
    std::vector<edge> get_out_edges(node_id_t node_id) override;
    std::vector<node> get_out_nodes(node_id_t node_id) override;
    std::vector<node_id_t> get_out_nodes_id(node_id_t node_id) override;
    std::vector<edge> get_in_edges(node_id_t node_id) override;
    std::vector<node> get_in_nodes(node_id_t node_id) override;
    std::vector<node_id_t> get_in_nodes_id(node_id_t node_id) override;
    void get_nodes(vector<node> &nodes);

    OutCursor *get_outnbd_iter() override;
    InCursor *get_innbd_iter() override;
    NodeCursor *get_node_iter() override;
    EdgeCursor *get_edge_iter() override;

    // internal cursor methods
    void drop_indices();
    static void create_indices(WT_SESSION *sess);

    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_src_idx_cursor();
    WT_CURSOR *get_dst_idx_cursor();
    WT_CURSOR *get_new_node_cursor();
    WT_CURSOR *get_new_edge_cursor();
    WT_CURSOR *get_new_src_idx_cursor();
    WT_CURSOR *get_new_dst_idx_cursor();
    std::vector<edge> test_cursor_iter(node_id_t node_id);

   private:
    // Cursors
    WT_CURSOR *node_cursor = nullptr;
    WT_CURSOR *random_node_cursor = nullptr;
    WT_CURSOR *edge_cursor = nullptr;
    WT_CURSOR *src_dst_index_cursor = nullptr;
    WT_CURSOR *src_index_cursor = nullptr;
    WT_CURSOR *dst_index_cursor = nullptr;

    // Internal methods

    void init_cursors();
    void close_all_cursors() override;
    int update_node_degree(WT_CURSOR *cursor,
                           node_id_t node_id,
                           degree_t indeg,
                           degree_t outdeg);
    int add_node_txn(node to_insert);
    int delete_edge_txn(node_id_t src_id,
                        node_id_t dst_id,
                        int *num_edges_to_add_ptr);
};

#endif