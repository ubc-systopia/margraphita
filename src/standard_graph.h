#ifndef STD_GRAPH
#define STD_GRAPH

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common.h"
#include "graph.h"
#include "graph_exception.h"

using namespace std;
namespace StdIterator
{
class InCursor : public table_iterator
{
   private:
    int prev_node;
    int cur_node;
    key_range keys;

   public:
    InCursor(WT_CURSOR *dst_edge_cur, WT_SESSION *sess)
    {
        init(dst_edge_cur, sess);
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

        edge curr_edge;

        if (is_first)
        {
            is_first = false;

            if (keys.start != -1)
            {
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
            else
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

        has_next = false;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }

    void next(adjlist *found, int key)
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

        edge curr_edge;
        cursor->set_key(cursor, key);

        found->degree = 0;
        found->edgelist.clear();
        found->node_id = key;

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
                if (keys.end != -1 && curr_edge.dst_id > keys.end)
                {
                    has_next = false;
                }
                return;
            }
            found->edgelist.push_back(curr_edge.src_id);
            found->degree++;
        } while (cursor->next(cursor) == 0);

        has_next = false;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }
};

class OutCursor : public table_iterator
{
   private:
    key_range keys;

   public:
    OutCursor(WT_CURSOR *src_edge_cur, WT_SESSION *sess)
    {
        init(src_edge_cur, sess);
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

        edge curr_edge;

        if (is_first)
        {
            is_first = false;

            if (keys.start != -1)
            {
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
            else
            {
                // Advances the cursor
                if (cursor->next(cursor) != 0)
                {
                    goto no_next;
                }
            }
        }

        cursor->get_value(cursor, &src, &dst);
        if (keys.end != -1 && src > keys.end)
        {
            goto no_next;
        }

        found->degree = 0;
        found->edgelist.clear();
        found->node_id = src;

        do
        {
            CommonUtil ::__read_from_edge_idx(cursor, &curr_edge);
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

        has_next = false;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }

    void next(adjlist *found, int key)
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

        edge curr_edge;
        cursor->set_key(cursor, key);

        found->degree = 0;
        found->edgelist.clear();
        found->node_id = key;

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
            if (curr_edge.src_id != key)
            {
                if (keys.end != -1 && curr_edge.src_id > keys.end)
                {
                    has_next = false;
                }
                return;
            }
            found->edgelist.push_back(curr_edge.dst_id);
            found->degree++;
        } while (cursor->next(cursor) == 0);

        has_next = false;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }
};

class NodeCursor : public table_iterator
{
   private:
    key_range keys;

   public:
    NodeCursor(WT_CURSOR *node_cur, WT_SESSION *sess)
    {
        init(node_cur, sess);
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, keys.start);
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

        if (cursor->next(cursor) == 0)
        {
        first_time_skip_next:
            // error_check(cursor->get_key(cursor, &found->id));
            cursor->get_key(cursor, &found->id);
            if (keys.end != -1 && found->id > keys.end)
            {
                goto no_next;
            }

            CommonUtil::__record_to_node(cursor, found, true);
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

class EdgeCursor : public table_iterator
{
   private:
    key_pair start_edge;
    key_pair end_edge;

   public:
    EdgeCursor(WT_CURSOR *composite_edge_cur, WT_SESSION *sess)
    {
        init(composite_edge_cur, sess);
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
            cursor->get_key(cursor, &found->src_id, &found->dst_id);

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

};  // namespace StdIterator

class StandardGraph : public GraphBase
{
   public:
    // create params
    StandardGraph(graph_opts &opt_params);
    void create_new_graph();
    void add_node(node to_insert);

    bool has_node(int node_id);
    node get_node(int node_id);
    void delete_node(int node_id);
    node get_random_node();
    int get_in_degree(int node_id);
    int get_out_degree(int node_id);
    std::vector<node> get_nodes();
    void add_edge(edge to_insert, bool is_bulk);
    bool has_edge(int src_id, int dst_id);
    void delete_edge(int src_id, int dst_id);
    edge get_edge(int src_id, int dst_id);  // todo <-- implement this
    std::vector<edge> get_edges();
    std::vector<edge> get_out_edges(int node_id);
    std::vector<node> get_out_nodes(int node_id);
    std::vector<edge> get_in_edges(int node_id);
    std::vector<node> get_in_nodes(int node_id);
    void get_nodes(vector<node> &nodes);
    std::string get_db_name() const { return opts.db_name; };

    StdIterator::OutCursor get_outnbd_iter();
    StdIterator::InCursor get_innbd_iter();
    StdIterator::EdgeCursor get_edge_iter();
    StdIterator::NodeCursor get_node_iter();

    // internal cursor methods
    //! Check if these should be public
    void init_cursors();  // todo <-- implement this
    WT_CURSOR *get_node_cursor();
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_src_idx_cursor();
    WT_CURSOR *get_dst_idx_cursor();
    WT_CURSOR *get_src_dst_idx_cursor();
    std::vector<edge> test_cursor_iter(int node_id);
    void make_indexes();

   private:
    // Cursors
    WT_CURSOR *node_cursor = NULL;
    WT_CURSOR *random_node_cursor = NULL;
    WT_CURSOR *edge_cursor = NULL;
    WT_CURSOR *src_dst_index_cursor = NULL;
    WT_CURSOR *src_index_cursor = NULL;
    WT_CURSOR *dst_index_cursor = NULL;
    WT_CURSOR *metadata_cursor = NULL;

    // structure of the graph
    vector<string> node_columns = {ID};  // Always there :)
    vector<string> edge_columns = {SRC, DST};
    string node_value_format;
    string node_key_format = "I";
    string edge_key_format = "II";
    string edge_value_format = "";  // I if weighted or b if unweighted.

    // Internal methods
    void drop_indices();
    void create_indices();
    void update_node_degree(WT_CURSOR *cursor,
                            int node_id,
                            int indeg,
                            int outdeg);
    void delete_related_edges(WT_CURSOR *index_cursor, int node_id);

    node get_next_node(WT_CURSOR *n_iter);
    edge get_next_edge(WT_CURSOR *e_iter);
};

#endif