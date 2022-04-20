#ifndef EDGE_KEY
#define EDGE_KEY

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common.h"
#include "graph_exception.h"

using namespace std;
namespace EKeyIterator
{
class InCursor : public table_iterator
{
   private:
    int prev_dst;
    int prev_node = 0;
    int cur_node = 0;

   public:
    InCursor(WT_CURSOR *beg_cur, WT_SESSION *sess) { init(beg_cur, sess); }
    void set_key(key_pair key)
    {
        cursor->set_key(cursor, key.dst_id);  // In Nbd
    }

    bool has_more() { return has_next; }

    void reset()
    {
        cursor->reset(cursor);
        is_first = true;
        has_next = true;
    }
    void next(adjlist *found)
    {
        int ret = 0;
        int prev_node, cur_node = 0;
        while ((ret = cursor->next(cursor)) == 0)
        {
            int src, dst = 0;
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
        int src = 0, dst = 0;
        int ret = cursor->search(cursor);

        if ((ret == 0) && (has_next == true))
        {
            do
            {
                int temp = 0;
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
    void set_key(key_pair key)
    {
        cursor->set_key(cursor, key.dst_id);  // In Nbd
    }
    bool has_more() { return has_next; }

    void reset()
    {
        cursor->reset(cursor);
        is_first = true;
        has_next = true;
    }
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
}  // namespace EKeyIterator

class EdgeKey
{
   public:
    graph_opts opts;
    EdgeKey(graph_opts opt_params);
    EdgeKey();
    void create_new_graph();
    void add_node(node to_insert);

    bool has_node(int node_id);
    node get_node(int node_id);
    void delete_node(int node_id);
    node get_random_node();
    int get_in_degree(int node_id);
    int get_out_degree(int node_id);
    std::vector<node> get_nodes();
    int get_num_nodes();
    int get_num_edges();
    void add_edge(edge to_insert, bool is_bulk);
    bool has_edge(int src_id, int dst_id);
    void delete_edge(int src_id, int dst_id);
    edge get_edge(int src_id, int dst_id);
    std::vector<edge> get_edges();
    std::vector<edge> get_out_edges(int node_id);
    std::vector<node> get_out_nodes(int node_id);
    std::vector<edge> get_in_edges(int node_id);
    std::vector<node> get_in_nodes(int node_id);
    void close();
    std::string get_db_name() const { return opts.db_name; };

    void create_indices();

    EKeyIterator::OutCursor get_outnbd_cursor();
    EKeyIterator::InCursor get_innbd_cursor();
    // internal cursor operations:
    void init_cursors();  // todo <-- implement this
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_src_idx_cur();
    WT_CURSOR *get_dst_idx_cur();
    WT_CURSOR *get_node_iter();
    WT_CURSOR *get_edge_iter();

   private:
    WT_CONNECTION *conn;
    WT_SESSION *session;

    // Cursors
    WT_CURSOR *edge_cursor = nullptr;
    WT_CURSOR *metadata_cursor = nullptr;
    WT_CURSOR *src_idx_cursor = nullptr;
    WT_CURSOR *dst_idx_cursor = nullptr;

    // structure of the graph
    vector<string> edge_columns = {SRC, DST, ATTR};
    string edge_key_format = "ii";   // SRC DST
    string edge_value_format = "S";  // Packed binary
    string node_count = "nNodes";
    string edge_count = "nEdges";

    // internal methods
    void delete_related_edges(WT_CURSOR *idx_cursor,
                              WT_CURSOR *edge_cur,
                              int node_id);
    void update_node_degree(int node_id, int indeg, int outdeg);
    node get_next_node(WT_CURSOR *n_iter);
    edge get_next_edge(WT_CURSOR *e_iter);
    int _get_table_cursor(string table, WT_CURSOR **cursor, bool is_random);
    int _get_index_cursor(std::string table_name,
                          std::string idx_name,
                          std::string projection,
                          WT_CURSOR **cursor);
    void drop_indices();
    void set_num_nodes(int num_nodes);
    void set_num_edges(int num_edges);

    // metadata and restore operations
    void insert_metadata(string key, char *value);
    string get_metadata(string key);
    void __restore_from_db(string db_name);

    void close_all_cursors();
};
#endif