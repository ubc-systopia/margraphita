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
    bool data_remaining = true;
    node_id_t next_expected = 0;

   public:
    StdInCursor(WT_CURSOR *cur, WT_SESSION *sess) : InCursor(cur, sess) {}

    void set_key_range(key_range _keys) override{};

    void next(adjlist *found) override
    {
        node_id_t src;
        node_id_t dst;
        edge curr_edge;

        if (!has_next)
        {
            goto no_next;
        }

        if (!data_remaining)
        {
            goto no_data_remaining;
        }

        if (is_first)
        {
            is_first = false;

            if (keys.start != -1)
            {
                next_expected = keys.start;
                int status;
                // error_check(cursor->search_near(cursor, &status));
                cursor->search_near(cursor, &status);
                if (status < 0)
                {
                    // Advances the cursor
                    if (cursor->next(cursor) != 0)
                    {
                        goto no_data_remaining;
                    }
                }
            }
            else
            {
                // Advances the cursor
                if (cursor->next(cursor) != 0)
                {
                    goto no_data_remaining;
                }
            }
        }

        if (keys.end != -1 && next_expected > keys.end)
        {
            goto no_next;
        }

        CommonUtil::get_val_idx(cursor, &src, &dst);

        found->degree = 0;
        found->edgelist.clear();
        found->node_id = dst;

        if (dst != next_expected)
        {
            found->node_id = next_expected;
            next_expected += 1;
            return;
        }
        next_expected += 1;

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
                return;
            }
        } while (cursor->next(cursor) == 0);

        data_remaining = false;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
        return;
    no_data_remaining:
        if (keys.end != -1 && next_expected > keys.end)
        {
            goto no_next;
        }
        found->degree = 0;
        found->edgelist.clear();
        found->node_id = next_expected;
        next_expected += 1;
    }

    void next(adjlist *found, node_id_t key) override
    {
        edge curr_edge;
        // Must reset OutCursor if already no_next
        if (!has_next)
        {
            goto no_next;
        }

        // Access outside of range not permitted
        if (keys.end != -1 && key > keys.end)
        {
            goto no_next;
        }

        if (keys.start != -1 && key < keys.start)
        {
            goto no_next;
        }

        next_expected = key + 1;

        CommonUtil::set_key(cursor, key);

        found->degree = 0;
        found->edgelist.clear();
        found->node_id = key;

        data_remaining = true;

        int status;
        // error_check(cursor->search_near(cursor, &status));
        cursor->search_near(cursor, &status);
        if (status < 0)
        {
            // Advances the cursor
            if (cursor->next(cursor) != 0)
            {
                data_remaining = false;
                return;
            }
        }

        do
        {
            CommonUtil::read_from_edge_idx(cursor, &curr_edge);
            if (curr_edge.dst_id != key)
            {
                if (keys.end != -1 && next_expected > keys.end)
                {
                    has_next = false;
                }
                return;
            }
            found->edgelist.push_back(curr_edge.src_id);
            found->degree++;
        } while (cursor->next(cursor) == 0);

        data_remaining = false;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }
};

class StdOutCursor : public OutCursor
{
    bool data_remaining = true;
    node_id_t next_expected = 0;

   public:
    StdOutCursor(WT_CURSOR *cur, WT_SESSION *sess) : OutCursor(cur, sess) {}

    void set_key_range(key_range _keys) override {}

    void next(adjlist *found) override
    {
        node_id_t src;
        node_id_t dst;
        edge curr_edge;

        if (!has_next)
        {
            goto no_next;
        }

        if (!data_remaining)
        {
            goto no_data_remaining;
        }

        if (is_first)
        {
            is_first = false;

            if (keys.start != -1)
            {
                next_expected = keys.start;
                int status;
                // error_check(cursor->search_near(cursor, &status));
                cursor->search_near(cursor, &status);
                if (status < 0)
                {
                    // Advances the cursor
                    if (cursor->next(cursor) != 0)
                    {
                        goto no_data_remaining;
                    }
                }
            }
            else
            {
                // Advances the cursor
                if (cursor->next(cursor) != 0)
                {
                    goto no_data_remaining;
                }
            }
        }

        if (keys.end != -1 && next_expected > keys.end)
        {
            goto no_next;
        }

        CommonUtil::get_val_idx(cursor, &src, &dst);

        found->degree = 0;
        found->edgelist.clear();
        found->node_id = src;

        if (src != next_expected)
        {
            found->node_id = next_expected;
            next_expected += 1;
            return;
        }
        next_expected += 1;

        do
        {
            CommonUtil ::read_from_edge_idx(cursor, &curr_edge);
            if (src == curr_edge.src_id)
            {
                found->degree++;
                found->edgelist.push_back(curr_edge.dst_id);
            }
            else
            {
                return;
            }
        } while (cursor->next(cursor) == 0);

        data_remaining = false;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
        return;
    no_data_remaining:
        if (keys.end != -1 && next_expected > keys.end)
        {
            goto no_next;
        }
        found->degree = 0;
        found->edgelist.clear();
        found->node_id = next_expected;
        next_expected += 1;
    }

    void next(adjlist *found, node_id_t key) override
    {
        edge curr_edge;
        // Must reset OutCursor if already no_next
        if (!has_next)
        {
            goto no_next;
        }

        // Access outside of range not permitted
        if (keys.end != -1 && key > keys.end)
        {
            goto no_next;
        }

        if (keys.start != -1 && key < keys.start)
        {
            goto no_next;
        }

        next_expected = key + 1;

        CommonUtil::set_key(cursor, key);

        found->degree = 0;
        found->edgelist.clear();
        found->node_id = key;

        data_remaining = true;

        int status;
        // error_check(cursor->search_near(cursor, &status));
        cursor->search_near(cursor, &status);
        if (status < 0)
        {
            // Advances the cursor
            if (cursor->next(cursor) != 0)
            {
                data_remaining = false;
                return;
            }
        }

        do
        {
            CommonUtil::read_from_edge_idx(cursor, &curr_edge);
            if (curr_edge.src_id != key)
            {
                if (keys.end != -1 && next_expected > keys.end)
                {
                    has_next = false;
                }
                return;
            }
            found->edgelist.push_back(curr_edge.dst_id);
            found->degree++;
        } while (cursor->next(cursor) == 0);

        data_remaining = false;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }
};

class StdNodeCursor : public NodeCursor
{
   public:
    StdNodeCursor(WT_CURSOR *cur, WT_SESSION *sess) : NodeCursor(cur, sess) {}

    void set_key_range(key_range _keys) override {}

    void next(node *found, node_id_t key) override {}

    void next(node *found) override
    {
        if (!has_next)
        {
            goto no_next;
        }

        if (is_first)
        {
            is_first = false;

            if (keys.start != -1)
            {
                int status;
                // error_check(cursor->search_near(cursor, &status));
                cursor->search_near(cursor, &status);
                if (status >= 0)
                {
                    goto first_time_skip_next;
                }
            }
        }

        if (cursor->next(cursor) == 0)
        {
        first_time_skip_next:
            // error_check(cursor->get_key(cursor, &found->id));
            CommonUtil::get_key(cursor, &found->id);
            if (keys.end != -1 && found->id > keys.end)
            {
                goto no_next;
            }

            CommonUtil::record_to_node(cursor, found, true);
        }
        else
        {
        no_next:
            found->id = -1;
            found->in_degree = -1;
            found->out_degree = -1;
            has_next = false;
        }
    }
};

class StdEdgeCursor : public EdgeCursor
{
   public:
    StdEdgeCursor(WT_CURSOR *cur, WT_SESSION *sess) : EdgeCursor(cur, sess) {}
    void set_key(edge_range range) override {}

    void next(edge *found) override
    {
        if (!has_next)
        {
            goto no_next;
        }

        // If first time calling next, we want the exact record corresponding to
        // the key_pair start or, if there is no such record, the smallest
        // record larger than the key_pair
        if (is_first)
        {
            is_first = false;

            if (start_edge.src_id != -1 && start_edge.dst_id != -1)
            {
                int status;
                // error_check(cursor->search_near(cursor, &status));
                cursor->search_near(cursor, &status);
                if (status >= 0)
                {
                    goto first_time_skip_next;
                }
            }
        }

        // Check existence of next record
        if (cursor->next(cursor) == 0)
        {
        first_time_skip_next:
            // error_check(
            //     cursor->get_key(cursor, &found->src_id, &found->dst_id));
            CommonUtil::get_key(cursor, &found->src_id, &found->dst_id);

            // If end_edge is set
            if (end_edge.src_id != -1)
            {
                // If found > end edge
                if (!(found->src_id < end_edge.src_id ||
                      ((found->src_id == end_edge.src_id) &&
                       (found->dst_id <= end_edge.dst_id))))
                {
                    goto no_next;
                }
            }

            CommonUtil::record_to_edge(cursor, found);
        }
        else
        {
        no_next:
            found->src_id = -1;
            found->dst_id = -1;
            found->edge_weight = -1;
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