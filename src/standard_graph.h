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
    InCursor(WT_CURSOR *beg_cur, WT_SESSION *sess)
    {
        init(beg_cur, sess);
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, keys.start);
    }

    // Return the next node in the graph. This function calls the cursor on dst
    // column
    void next(adjlist *found)
    {
        edge idx;
        int ret = 0;
        while ((ret = cursor->next(cursor)) == 0)
        {
            cursor->get_key(cursor, &cur_node);
            if ((keys.start != -1 && cur_node < keys.start) ||
                (keys.end != -1 && cur_node > keys.end))  // out of key range
            {
                has_next = false;
                found->degree = -1;
                found->node_id = -1;
                return;
            }

            if (is_first)
            {
                prev_node = cur_node;
                is_first = false;
            }

            CommonUtil::__read_from_edge_idx(cursor, &idx);
            if (prev_node == idx.dst_id)
            {
                found->degree++;
                found->edgelist.push_back(idx.src_id);  // in nbd;
                found->node_id = idx.dst_id;
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

    void reset() override
    {
        cursor->reset(cursor);
        is_first = true;
        has_next = true;
        prev_node = cur_node = 0;
    }

    void next(adjlist *found, int key)
    {
        edge idx;
        cursor->reset(cursor);
        cursor->set_key(cursor, key);
        int ret = 0;
        if (cursor->search(cursor) == 0 && has_next)
        {
            int iter_key;
            do
            {
                CommonUtil::__read_from_edge_idx(cursor, &idx);
                found->edgelist.push_back(idx.src_id);
                found->degree++;
                found->node_id = key;
                // now check if the next key is still the same as the one we
                // want
                ret = cursor->next(cursor);
                if (ret != 0)
                {
                    has_next = false;
                    return;
                }
                cursor->get_key(cursor, &iter_key);
            } while (iter_key == key);
        }
        else
        {
            found->node_id = -1;
            cursor->reset(cursor);
        }
    }
};

class OutCursor : public table_iterator
{
   private:
    int prev_node;
    int cur_node;
    key_range keys;

   public:
    OutCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        init(cur, sess);
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, keys.start);
    }

    // This calls the next method on the src_index cursor
    void next(adjlist *found)  //! Make this a reference not a pointer to use
                               //! move semantics
    {
        edge idx;
        int ret = 0;
        while ((ret = cursor->next(cursor)) == 0)
        {
            cursor->get_key(cursor, &cur_node);
            if ((keys.start != -1 && cur_node < keys.start) ||
                (keys.end != -1 && cur_node > keys.end))  // out of key range
            {
                has_next = false;
                found->degree = -1;
                found->node_id = -1;
                return;
            }
            if (is_first)
            {
                prev_node = cur_node;
                is_first = false;
            }
            CommonUtil ::__read_from_edge_idx(cursor, &idx);
            if (prev_node == idx.src_id)
            {
                found->degree++;
                found->edgelist.push_back(idx.dst_id);
                found->node_id = idx.src_id;
            }
            else
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

    void reset() override
    {
        cursor->reset(cursor);
        is_first = true;
        has_next = true;
        prev_node = cur_node = 0;
    }

    void next(adjlist *found, int key)
    {
        edge idx;
        cursor->set_key(cursor, key);
        int ret = 0;
        if (cursor->search(cursor) == 0 && has_next)
        {
            int iter_key;
            do
            {
                CommonUtil::__read_from_edge_idx(cursor, &idx);
                found->edgelist.push_back(idx.dst_id);
                found->degree++;
                found->node_id = key;
                // check if the next key is the same as the one we are currently
                // looking for.
                ret = cursor->next(cursor);
                if (ret != 0)
                {
                    has_next = false;
                    return;
                }
                cursor->get_key(cursor, &iter_key);
            } while (iter_key == key);
        }
        else
        {
            found->node_id = -1;
            cursor->reset(cursor);
        }
    }
};

class NodeCursor : public table_iterator
{
   private:
    key_range keys;

   public:
    NodeCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        init(cur, sess);
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, keys.start);
    }

    void next(node *found)
    {
        if (cursor->next(cursor) == 0)
        {
            cursor->get_key(cursor, &found->id);
            if (keys.start > -1 && keys.start > -1)  // keys were set
            {
                if (found->id >= keys.start && found->id <= keys.end)
                {
                    CommonUtil::__record_to_node(
                        cursor, found, true);  // works for read_opt only
                }
                else
                {
                    found->id = -1;
                    found->in_degree = -1;
                    found->out_degree = -1;
                    has_next = false;
                }
            }
            else
            {
                CommonUtil::__record_to_node(
                    cursor, found, true);  // works for read_opt only
            }
        }
        else
        {
            found->id = -1;
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
        // If first time calling next, we want the exact record corresponding to
        // the key_pair start or, if there is no such record, the smallest
        // record larger than the key_pair
        if (is_first == true)
        {
            int status;
            cursor->search_near(cursor, &status);
            is_first = false;
            if (!(status < 0))
            {
                goto first_time_skip_next;
            }
        }

        // Check existence of next record
        if (cursor->next(cursor) == 0)
        {
        first_time_skip_next:
            cursor->get_key(cursor, &found->src_id, &found->dst_id);

            // If start_edge is set
            if (start_edge.src_id != -1 && start_edge.dst_id != -1)
            {
                // Expect start_edge <= found
                if (start_edge.src_id < found->src_id ||
                    ((start_edge.src_id == found->src_id) &&
                     (start_edge.dst_id <= found->dst_id)))
                {
                    // pass
                }
                else
                {
                    goto no_next;
                }
            }

            // If end_edge is set
            if (end_edge.src_id != -1 && end_edge.dst_id != -1)
            {
                // Expect found <= end edge
                if (found->src_id < end_edge.src_id ||
                    ((found->src_id == end_edge.src_id) &&
                     (found->dst_id <= end_edge.dst_id)))
                {
                    // pass
                }
                else
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