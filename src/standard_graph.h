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
        if (_keys.end == -1 || _keys.end == 0)
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

        if (keys.end != -1 && dst > keys.end)
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
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
        return;
    }

    void next(adjlist *found, node_id_t key) override {}
};

class StdOutCursor : public OutCursor
{
    node_id_t curr_node, curr_edge_src, unused;
    bool more_edges = true;
    WT_CURSOR *node_cursor = nullptr;

   public:
    StdOutCursor(WT_CURSOR *cur, WT_SESSION *sess) : OutCursor(cur, sess)
    {
        int ret = session->open_cursor(
            session, "table:node", nullptr, nullptr, &node_cursor);
        if (ret != 0)
        {
            throw GraphException("Failed to open node cursor" +
                                 string(wiredtiger_strerror(ret)));
        }
    }

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
        if (_keys.end == -1 || _keys.end == 0)
        {
            keys.end = INT32_MAX;
        }

        CommonUtil::set_key(
            cursor, keys.start, -1);  // set the key to the start of the range
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

        /*
         * Set the node_cursor to the start of the range
         */
        CommonUtil::set_key(node_cursor, keys.start);
        node_cursor->search_near(node_cursor, &status);
        if (status < 0)
        {
            // Advances the cursor
            if (node_cursor->next(node_cursor) != 0)
            {
                has_next = false;
                return;
            }
        }

        CommonUtil::get_key(node_cursor, &curr_node);
    }

    void next(adjlist *found) override
    {
        node_id_t temp_src, temp_dst;
        int res = 0;

        if (!has_next)
        {
            no_next(found);
            return;
        }

        // get the edge
        if (!more_edges)
        {
            goto no_more_edges;
        }

        CommonUtil::get_key(cursor, &curr_edge_src, &temp_dst);
        if (curr_edge_src == curr_node)
        {
            found->node_id = curr_edge_src;
            found->edgelist.push_back(temp_dst);
            found->degree++;
        }
        else
        {
        no_more_edges:
            // this branch should be taken for all nodes that don't have any
            // out-edges
            found->node_id = curr_node;
            found->degree = 0;
            found->edgelist.clear();

            // advance the node cursor
            if (node_cursor->next(node_cursor) == 0)
            {
                CommonUtil::get_key(node_cursor, &curr_node);
                if (curr_node > keys.end)
                {
                    has_next = false;
                    return;
                }
            }
            else
            {
                has_next = false;
                return;
            }
            return;
        }

        // advance the edge cursor till we hit a new src
        while (cursor->next(cursor) == 0)
        {
            CommonUtil::get_key(cursor, &temp_src, &temp_dst);
            if (temp_src == curr_node)
            {
                found->edgelist.push_back(temp_dst);
                found->degree++;
            }
            else
            {
                // we have hit a new src
                node_cursor->next(node_cursor);
                CommonUtil::get_key(node_cursor, &curr_node);
                if (curr_node == temp_src)
                {
                    if (curr_node > keys.end)
                    {
                        has_next = false;
                        return;
                    }
                    return;
                }
                else
                {
                    // the next node is not the same as the node whose out_nbd
                    // the edge cursor has travelled to.There is nothing to do
                    // here -- it is handled above. Return.
                    return;
                }
            }
        }
        more_edges = false;
        // If we reach here, it means we have exhausted the edge_cursor. We
        // must check if there are more nodes to process, because while
        // there can only be an edge between two existing nodes, there can
        // be nodes not having any edges.
        if (node_cursor->next(node_cursor) == 0)
        {
            CommonUtil::get_key(node_cursor, &curr_node);
            if (curr_node > keys.end)
            {
                has_next = false;
            }
            return;
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
        if (keys.start != -1)
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
        found->id = -1;
        found->in_degree = -1;
        found->out_degree = -1;
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
        if (keys.end != -1 &&
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
        if (start_edge.src_id != -1 && start_edge.dst_id != -1)
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
        found->src_id = -1;
        found->dst_id = -1;
        found->edge_weight = -1;
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