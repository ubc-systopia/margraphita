#ifndef ADJ_LIST
#define ADJ_LIST

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common.h"
#include "graph.h"
#include "graph_exception.h"

using namespace std;
namespace AdjIterator
{
class InCursor : public table_iterator
{
   public:
    InCursor(WT_CURSOR *cur, WT_SESSION *sess) { init(cur, sess); }

    void next(adjlist *found)
    {
        if (cursor->next(cursor) == 0)
        {
            CommonUtil::__record_to_adjlist(session, cursor, found);
            cursor->get_key(cursor, &found->node_id);
        }
        else
        {
            found->node_id = -1;
            has_next = false;
        }
    }

    void next(adjlist *found, node_id_t key)
    {
        cursor->set_key(cursor, key);
        if (cursor->search(cursor) == 0)
        {
            CommonUtil::__record_to_adjlist(session, cursor, found);
            found->node_id = key;
            cursor->reset(cursor);
        }
        else
        {
            found->node_id = -1;
            // check for out-of-band values in application program.
            has_next = false;
        }
    }
};

class OutCursor : public table_iterator
{
   public:
    OutCursor(WT_CURSOR *cur, WT_SESSION *sess) { init(cur, sess); }

    void next(adjlist *found)
    {
        if (cursor->next(cursor) == 0)
        {
            CommonUtil::__record_to_adjlist(session, cursor, found);
            cursor->get_key(cursor, &found->node_id);
        }
        else
        {
            found->node_id = -1;
            has_next = false;
        }
    }

    void next(adjlist *found, node_id_t key)
    {
        if (cursor->next(cursor) == 0)
        {
            cursor->get_key(cursor, &found->node_id);
            if (found->node_id == key)
            {
                CommonUtil::__record_to_adjlist(session, cursor, found);
            }
            else
            {
                found->node_id = -1;
                has_next = false;
            }
        }
        else
        {
            found->node_id = -1;
            has_next = false;
        }
    }
};

class NodeCursor : public table_iterator
{
   private:
    key_range keys;

   public:
    NodeCursor(WT_CURSOR *cur, WT_SESSION *sess) { init(cur, sess); }

    /**
     * @brief Set the key range object
     *
     * @param _keys the key range object. Set the end key to INT_MAX if you want
     * to get all the nodes from start node.
     */
    void set_key_range(key_range _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, keys.start);
    }

    // use key_pair to define start and end keys.
    void next(node *found)
    {
        if (cursor->next(cursor) == 0)
        {
            int temp_key;
            cursor->get_key(cursor, &temp_key);
            if (temp_key > keys.end)
            {
                found->id = -1;
                has_next = false;
            }
            else
            {
                CommonUtil::__record_to_node(cursor, found, true);
                found->id = temp_key;
            }
        }
        else
        {
            found->id = -1;
            // check for out-of-band values in application program.
            has_next = false;
        }
    }
};

class EdgeCursor : public table_iterator
{
   private:
    key_pair keys;

   public:
    EdgeCursor(WT_CURSOR *cur, WT_SESSION *sess) { init(cur, sess); }

    void set_key(int key) = delete;
    void set_key(key_pair _keys)
    {
        keys = _keys;
        cursor->set_key(cursor, keys.src_id, keys.dst_id);
    }

    void next(edge *found)
    {
        if (cursor->next(cursor) == 0)
        {
            cursor->get_key(cursor, &found->src_id, &found->dst_id);
            CommonUtil::__record_to_edge(cursor, found);
        }
        else
        {
            found->src_id = -1;
            found->dst_id = -1;
            found->edge_weight = -1;
            has_next = false;
        }
    }
};

}  // namespace AdjIterator

class AdjList : public GraphBase
{
   public:
    AdjList(graph_opts &opt_params);
    void create_new_graph();
    void add_node(node to_insert);
    void add_node(node_id_t to_insert,
                  std::vector<node_id_t> &inlist,
                  std::vector<node_id_t> &outlist);
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
    std::string get_db_name() const { return opts.db_name; };
    std::vector<node_id_t> get_adjlist(WT_CURSOR *cursor, node_id_t node_id);
    AdjIterator::OutCursor get_outnbd_iter();
    AdjIterator::InCursor get_innbd_iter();
    AdjIterator::NodeCursor get_node_iter();
    AdjIterator::EdgeCursor get_edge_iter();

    edgeweight_t get_edge_weight(node_id_t src_id, node_id_t dst_id);
    void update_edge_weight(
        node_id_t src_id,
        node_id_t dst_id,
        edgeweight_t edge_weight);  // todo <-- is this implemented?

    // internal cursor operations:
    //! Check if these should be public:
    void init_cursors();
    WT_CURSOR *get_node_cursor();
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_in_adjlist_cursor();
    WT_CURSOR *get_out_adjlist_cursor();
    void make_indexes() { return; };

   private:
    // structure of the graph
    int node_attr_size = 0;  // set on checking the list len

    vector<string> node_columns = {ID};  // Always there :)
    vector<string> edge_columns = {SRC, DST};
    vector<string> in_adjlist_columns = {ID, IN_DEGREE, IN_ADJLIST};
    vector<string> out_adjlist_columns = {ID, OUT_DEGREE, OUT_ADJLIST};

    string node_value_format;
    string node_key_format = "q";
    string edge_key_format = "qq";  // SRC DST in the edge table
    string edge_value_format = "";  // Make I if weighted , x otherwise
    string adjlist_key_format = "q";
    string adjlist_value_format =
        "Iu";  // This HAS to be u. S does not work. s needs the number.

    WT_CURSOR *node_cursor = NULL;
    WT_CURSOR *random_node_cursor = NULL;
    WT_CURSOR *edge_cursor = NULL;
    WT_CURSOR *in_adjlist_cursor = NULL;
    WT_CURSOR *out_adjlist_cursor = NULL;
    WT_CURSOR *metadata_cursor = NULL;

    // AdjList specific internal methods:
    node get_next_node(WT_CURSOR *n_cur);
    edge get_next_edge(WT_CURSOR *e_cur);
    void add_adjlist(WT_CURSOR *cursor, node_id_t node_id);
    void add_adjlist(WT_CURSOR *cursor,
                     node_id_t node_id,
                     std::vector<node_id_t> &list);
    void delete_adjlist(WT_CURSOR *cursor, node_id_t node_id);
    void delete_node_from_adjlists(node_id_t node_id);
    void add_to_adjlists(WT_CURSOR *cursor,
                         node_id_t node_id,
                         node_id_t to_insert);
    void delete_from_adjlists(WT_CURSOR *cursor,
                              node_id_t node_id,
                              node_id_t to_delete);
    void delete_related_edges_and_adjlists(node_id_t node_id);
    void update_node_degree(WT_CURSOR *cursor,
                            node_id_t node_id,
                            degree_t indeg,
                            degree_t outdeg);

    void dump_tables();
    void create_indices() { return; }  // here because defined in interface
};

/**
 * iterator design:
 *
 * In the application, we need access to in and out neighbouhoods.
 * Ideally, we need an iterator class, on object of which can be returned to the
 * app these objects should have set_key(src, dst) and std::vector<int>
 * get_value()
 *
 */
#endif