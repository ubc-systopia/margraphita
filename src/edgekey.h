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

class EkeyInCursor : public InCursor
{
    node_id_t next_expected = 0;
    bool data_remaining = true;

   public:
    EkeyInCursor(WT_CURSOR *cur, WT_SESSION *sess) : InCursor(cur, sess) {}

    void next(adjlist *found)
    {
        if (!has_next)
        {
            goto no_next;
        }

        if (!data_remaining)
        {
            goto no_data_remaining;
        }

        node_id_t src;
        node_id_t dst;

        if (is_first)
        {
            is_first = false;

            node_id_t to_search = 0;

            if (keys.start != -1)
            {
                to_search = keys.start;
                next_expected = keys.start;
            }

            cursor->set_key(cursor, to_search);

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

        if (keys.end != -1 && next_expected > keys.end)
        {
            goto no_next;
        }

        cursor->get_value(cursor, &src, &dst);

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

        edge curr_edge;

        do
        {
            CommonUtil ::__read_from_edge_idx(cursor, &curr_edge);
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

    void next(adjlist *found, node_id_t key)
    {
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

        edge curr_edge;
        cursor->set_key(cursor, key);

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
                has_next = false;
                return;
            }
        }

        do
        {
            CommonUtil::__read_from_edge_idx(cursor, &curr_edge);
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

        data_remaining = true;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }
};

class EkeyOutCursor : public OutCursor
{
   private:
    node_id_t next_expected = 0;
    bool data_remaining = true;

   public:
    EkeyOutCursor(WT_CURSOR *cur, WT_SESSION *sess) : OutCursor(cur, sess) {}

    void next(adjlist *found)
    {
        if (!has_next)
        {
            goto no_next;
        }

        if (!data_remaining)
        {
            goto no_data_remaining;
        }

        node_id_t src;
        node_id_t dst;

        edge curr_edge;

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

        cursor->get_value(cursor, &src, &dst);

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
            CommonUtil ::__read_from_edge_idx(cursor, &curr_edge);
            if (src == curr_edge.src_id)
            {
                if (curr_edge.dst_id != -1)
                {
                    found->degree++;
                    found->edgelist.push_back(curr_edge.dst_id);
                }
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

    void next(adjlist *found, node_id_t key)
    {
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

        edge curr_edge;
        cursor->set_key(cursor, key);

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
            CommonUtil::__read_from_edge_idx(cursor, &curr_edge);
            if (curr_edge.src_id != key)
            {
                if (keys.end != -1 && next_expected > keys.end)
                {
                    has_next = false;
                }
                return;
            }
            if (curr_edge.dst_id != -1)
            {
                found->edgelist.push_back(curr_edge.dst_id);
                found->degree++;
            }
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
/**
 * @brief This class is used to iterate over the nodes of a graph.
 * Considering the way we imlpement EdgeKey, this class needs a cursor to the
 * dst index. FIXIT: We need to change this to make it transparent to the user.
 */

class EkeyNodeCursor : public NodeCursor
{
   public:
    // Takes a composite index cursor on (dst, src)
    EkeyNodeCursor(WT_CURSOR *cur, WT_SESSION *sess) : NodeCursor(cur, sess) {}

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, OutOfBand_ID, keys.start);
    }

    void next(node *found)
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

        edge curr_edge;

        if (cursor->next(cursor) == 0)
        {
        first_time_skip_next:

            char *buf;
            cursor->get_value(
                cursor,
                &curr_edge.dst_id,
                &curr_edge.src_id,
                &buf);  // getting all of dst, src, in/out degrees at once
            std::string str(buf);
            int a = 0, b = 0;
            CommonUtil::extract_from_string(str, &a, &b);

            found->in_degree = a;
            found->out_degree = b;
            found->id = curr_edge.src_id;

            if (keys.end != -1 && curr_edge.src_id > keys.end)
            {
                goto no_next;
            }

            if (curr_edge.dst_id != -1)
            {
                goto no_next;
            }
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

class EkeyEdgeCursor : public EdgeCursor
{
   public:
    EkeyEdgeCursor(WT_CURSOR *cur, WT_SESSION *sess) : EdgeCursor(cur, sess) {}

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
        CommonUtil::__record_to_edge_ekey(cursor, found);
        return;

    no_next:
        found->src_id = -1;
        found->dst_id = -1;
        found->edge_weight = -1;
        has_next = false;
    }
};

class EdgeKey : public GraphBase
{
   public:
    EdgeKey(graph_opts opt_params);
    EdgeKey(graph_opts &opt_params,
            wt_conn &connection);  // TODO: merge the 2 constructors
    static void create_wt_tables(graph_opts &opts, WT_CONNECTION *conn);
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
    std::vector<node_id_t> get_out_nodes_id(node_id_t node_id);
    std::vector<edge> get_in_edges(node_id_t node_id);
    std::vector<node> get_in_nodes(node_id_t node_id);
    std::vector<node_id_t> get_in_nodes_id(node_id_t node_id);

    OutCursor *get_outnbd_iter();
    InCursor *get_innbd_iter();
    NodeCursor *get_node_iter();
    EdgeCursor *get_edge_iter();
    // internal cursor operations:
    void init_cursors();  // todo <-- implement this
    WT_CURSOR *get_src_idx_cursor();
    WT_CURSOR *get_dst_idx_cursor();
    WT_CURSOR *get_node_cursor();
    WT_CURSOR *get_edge_cursor();

    WT_CURSOR *get_new_src_idx_cursor();
    WT_CURSOR *get_new_dst_idx_cursor();
    WT_CURSOR *get_new_node_cursor();
    WT_CURSOR *get_new_edge_cursor();

    WT_CURSOR *get_dst_src_idx_cursor();
    WT_CURSOR *get_new_dst_src_idx_cursor();
    void make_indexes();

   private:
    // Cursors
    WT_CURSOR *edge_cursor = nullptr;
    WT_CURSOR *metadata_cursor = nullptr;
    WT_CURSOR *src_idx_cursor = nullptr;
    WT_CURSOR *dst_idx_cursor = nullptr;
    WT_CURSOR *dst_src_idx_cursor = nullptr;

    // structure of the graph
    vector<string> edge_columns = {SRC, DST, ATTR};
    string edge_key_format = "qq";   // SRC DST
    string edge_value_format = "S";  // Packed binary
    string node_count = "nNodes";
    string edge_count = "nEdges";

    // internal methods
    WT_CURSOR *get_metadata_cursor();
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
