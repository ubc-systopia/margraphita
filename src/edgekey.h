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
    node_id_t prev_dst;
    node_id_t prev_node = 0;
    node_id_t cur_node = 0;

   public:
    InCursor(WT_CURSOR *beg_cur, WT_SESSION *sess) { init(beg_cur, sess); }
    bool has_more() { return has_next; }

    void next(adjlist *found)
    {
        int ret = 0;
        // node_id_t prev_node, cur_node = 0;
        while ((ret = cursor->next(cursor)) == 0)
        {
            node_id_t src, dst = 0;
            cursor->get_key(cursor, &cur_node);
            if (cur_node == -1)
            {
                continue;
            }
            if (is_first)
            {
                prev_node = cur_node;
                is_first = false;
            }
            cursor->get_value(cursor, &src, &dst);
            if (prev_node == cur_node)
            {
                found->degree++;
                found->edgelist.push_back(src);  // in nbd;
                found->node_id = dst;
            }
            else  // We have moved to the next node id.
            {
                cursor->prev(cursor);
                is_first = true;
                break;
            }
        }
        if (ret != 0)
        {
            has_next = false;
        }
    }

    void next(adjlist *found, key_pair kp)
    {
        cursor->set_key(cursor, kp.dst_id);
        node_id_t src = 0, dst = 0;
        int ret = cursor->search(cursor);

        if ((ret == 0) && (has_next == true))
        {
            do
            {
                node_id_t temp = 0;
                cursor->get_value(cursor, &src, &dst);
                cursor->get_key(cursor, &temp);
                if (dst == kp.dst_id)
                {
                    found->degree++;
                    found->edgelist.push_back(src);
                    found->node_id = dst;
                }
                else
                {
                    break;
                }
            } while (cursor->next(cursor) == 0);
        }
        else
        {
            has_next = false;
            found->node_id = kp.dst_id;
            found->degree = 0;
            found->edgelist = {};
        }
    }
};

class OutCursor : public table_iterator
{
   private:
    int prev_dst;
    int prev_node = 0;
    int cur_node = 0;

   public:
    OutCursor(WT_CURSOR *beg_cur, WT_SESSION *sess) { init(beg_cur, sess); }

    bool has_more() { return has_next; }

    void next(adjlist *found)
    {
        int ret = 0;

        while ((ret = cursor->next(cursor)) == 0)
        {
            int src, dst = 0;
            cursor->get_key(cursor, &cur_node);
            cursor->get_value(cursor, &src, &dst);

            if (dst == -1 && is_first)
            {
                is_first = false;
                prev_node = cur_node;
                prev_dst = dst;
                continue;
            }
            if (dst != -1 && (prev_node == cur_node))
            {
                found->degree++;
                found->edgelist.push_back(dst);
                prev_dst = dst;

                found->node_id = src;
            }
            else if (dst == -1 && (prev_node != cur_node))
            {
                // handle nodes with no out edges
                if (prev_dst == -1)
                {
                    // the previous_node had no out edges
                    found->degree = 0;
                    found->edgelist = {};
                    found->node_id = prev_node;
                    prev_node = cur_node;
                    return;
                }
                cursor->prev(cursor);
                is_first = true;
                break;
            }
        }
        if (ret != 0)
        {
            found->degree = 0;
            found->edgelist = {};
            found->node_id = cur_node;
            has_next = false;
        }
    }

    void next(adjlist *found, key_pair kp)
    {
        cursor->set_key(cursor, kp.src_id);
        int src = 0, dst = 0;
        int ret = cursor->search(cursor);

        if ((ret == 0) && (has_next == true))
        {
            do
            {
                int temp = 0;
                cursor->get_value(cursor, &src, &dst);
                cursor->get_key(cursor, &temp);
                if (src == kp.src_id)
                {
                    if (dst != -1)
                    {
                        found->degree++;
                        found->edgelist.push_back(dst);
                        found->node_id = src;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    break;
                }
            } while (cursor->next(cursor) == 0);
        }
        else
        {
            found->degree = 0;
            found->edgelist = {};
            found->node_id = kp.src_id;
            has_next = false;
        }
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
    NodeIterator(WT_CURSOR *beg_cur, WT_SESSION *sess)
    {
        init(beg_cur, sess);
        keys = {
            -1,
            -1};  // this can't be -1 because it is used as a node identifier
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, keys.start, -1);
    }

    void next(node *found)
    {
        int ret = 0;
        while ((ret = cursor->next(cursor)) == 0)
        {
            int src, dst = 0;
            int cur_node = 0;
            cursor->get_key(cursor, &cur_node);
            if (cur_node != 0)
            {
                continue;
            }
            cursor->get_value(cursor, &src, &dst);
            if (keys.start > -1 && keys.end > -1)  // keys were set
            {
                if (src >= keys.start && src <= keys.end)
                {
                    CommonUtil::__record_to_node_ekey(cursor, found);
                    found->id = src;
                    return;
                }
                else
                {
                    has_next = false;
                    found->id = -1;
                    found->in_degree = -1;
                    found->out_degree = -1;
                }
            }
            else
            {
                CommonUtil::__record_to_node_ekey(cursor, found);
                found->id = src;
                return;
            }
        }
        if (ret != 0)
        {
            has_next = false;
            found->id = -1;
            found->in_degree = -1;
            found->out_degree = -1;
        }
    }
};

class EdgeCursor : public table_iterator
{
   private:
    key_pair start_edge;
    key_pair end_edge;

   public:
    EdgeCursor(WT_CURSOR *cur, WT_SESSION *sess) { init(cur, sess); }

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
            return;
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