#ifndef EDGE_KEY
#define EDGE_KEY

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common.h"
#include "graph.h"
#include "graph_exception.h"

using namespace std;
namespace EKeyIterator
{
class InCursor : public table_iterator
{
   private:
    key_range keys;

   public:
    InCursor(WT_CURSOR *dst_cur, WT_SESSION *sess)
    {
        init(dst_cur, sess);
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, keys.start);
    }

    void next(adjlist *found)
    {
        if (!has_next)
        {
            goto no_next;
        }

        int src;
        int dst;

        if (is_first)
        {
            is_first = false;

            int to_search = 0;

            if (keys.start != -1)
            {
                to_search = keys.start;
            }

            int status;
            // error_check(cursor->search_near(cursor, &status));
            cursor->search_near(cursor, &status);
            if (status < 0)
            {
                // Advances the cursor
                if (cursor->next(cursor) != 0)
                {
                    goto no_next;
                }
            }
        }

        cursor->get_value(cursor, &src, &dst);
        if (keys.end != -1 && dst > keys.end)
        {
            goto no_next;
        }

        found->degree = 0;
        found->edgelist.clear();
        found->node_id = dst;

        edge curr_edge;

        do
        {
            cursor->get_value(cursor, &curr_edge->src_id, &curr_edge->dst_id);
        } while (true);
        // TODO
    }

    void next(adjlist *found, key_pair kp)
    {
        // TODO
    }
};

class OutCursor : public table_iterator
{
   private:
    key_range keys;

   public:
    OutCursor(WT_CURSOR *src_cur, WT_SESSION *sess)
    {
        init(src_cur, sess);
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, keys.start);
    }

    void next(adjlist *found)
    {
        // TODO
    }

    void next(adjlist *found, key_pair kp)
    {
        // TODO
    }
};
/**
 * @brief This class is used to iterate over the nodes of a graph.
 * Considering the way we imlpement EdgeKey, this class needs a cursor to the
 * dst index. FIXIT: We need to change this to make it transparent to the user.
 */

class NodeIterator : public table_iterator
{
   private:
    key_range keys;

   public:
    // Takes a composite index cursor on (dst, src)
    NodeIterator(WT_CURSOR *comp_idx_cur, WT_SESSION *sess)
    {
        init(comp_idx_cur, sess);
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, -1);
    }

    void next(node *found)
    {
        // TODO
    }
};

class EdgeCursor : public table_iterator
{
   private:
    key_pair start_edge;
    key_pair end_edge;

   public:
    EdgeCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        init(cur, sess);
        start_edge = {-1, -1};
        end_edge = {-1, -1};
    }

    // Overwrites set_key(int key) implementation in table_iterator
    void set_key(int key) = delete;

    void set_key(key_pair start, key_pair end)
    {
        start_edge = start;
        end_edge = end;
        cursor->set_key(cursor, start.src_id, start.dst_id);
    }

    void next(edge *found)
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
                    goto skip_first_advance;
                }
            }
        }

        if (cursor->next(cursor) != 0)
        {
            goto no_next;
        }

    skip_first_advance:
        // Advance until cursor is pointing to an edge
        while (true)
        {
            // error_check(
            //     cursor->get_key(cursor, &found->src_id, &found->dst_id));
            cursor->get_key(cursor, &found->src_id, &found->dst_id);
            if (found->dst_id != -1)
            {
                goto edge_found;
            }
            if (cursor->next(cursor) != 0)
            {
                goto no_next;
            }
        }

    edge_found:
        // If end_edge is set
        if (end_edge.src_id != -2)
        {
            // If found > end edge
            if (!(found->src_id < end_edge.src_id ||
                  ((found->src_id == end_edge.src_id) &&
                   (found->dst_id <= end_edge.dst_id))))
            {
                goto no_next;
            }
        }
        CommonUtil::__record_to_edge(cursor, found);
        return;

    no_next:
        found->src_id = -1;
        found->dst_id = -1;
        found->edge_weight = -1;
        has_next = false;
    }
};
}  // namespace EKeyIterator
class EdgeKey : public GraphBase
{
   public:
    EdgeKey(graph_opts opt_params);
    void create_new_graph();
    void add_node(node to_insert);

    bool has_node(node_id_t node_id);
    node get_node(node_id_t node_id);
    void delete_node(node_id_t node_id);
    node get_random_node();
    degree_t get_in_degree(node_id_t node_id);
    degree_t get_out_degree(node_id_t node_id);
    std::vector<node> get_nodes();
    void add_edge(edge to_insert, bool is_bulk);
    bool has_edge(node_id_t src_id, node_id_t dst_id);
    void delete_edge(node_id_t src_id, node_id_t dst_id);
    edge get_edge(node_id_t src_id, node_id_t dst_id);
    std::vector<edge> get_edges();
    std::vector<edge> get_out_edges(node_id_t node_id);
    std::vector<node> get_out_nodes(node_id_t node_id);
    std::vector<edge> get_in_edges(node_id_t node_id);
    std::vector<node> get_in_nodes(node_id_t node_id);

    EKeyIterator::OutCursor get_outnbd_cursor();
    EKeyIterator::InCursor get_innbd_cursor();
    // internal cursor operations:
    void init_cursors();  // todo <-- implement this
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_src_idx_cur();
    WT_CURSOR *get_dst_idx_cur();
    WT_CURSOR *get_node_iter();
    WT_CURSOR *get_edge_iter();
    void make_indexes();

   private:
    // Cursors
    WT_CURSOR *edge_cursor = nullptr;
    WT_CURSOR *metadata_cursor = nullptr;
    WT_CURSOR *src_idx_cursor = nullptr;
    WT_CURSOR *dst_idx_cursor = nullptr;

    // structure of the graph
    vector<string> edge_columns = {SRC, DST, ATTR};
    string edge_key_format = "qq";   // SRC DST
    string edge_value_format = "S";  // Packed binary
    string node_count = "nNodes";
    string edge_count = "nEdges";

    // internal methods
    void delete_related_edges(WT_CURSOR *idx_cursor,
                              WT_CURSOR *edge_cur,
                              node_id_t node_id);
    void update_node_degree(node_id_t node_id, degree_t indeg, degree_t outdeg);
    node get_next_node(WT_CURSOR *n_iter);
    edge get_next_edge(WT_CURSOR *e_iter);

    void create_indices();
    void drop_indices();
    void close_all_cursors();
};
#endif