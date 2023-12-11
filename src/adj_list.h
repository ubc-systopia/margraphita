#ifndef ADJ_LIST
#define ADJ_LIST

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common_util.h"
#include "graph.h"
#include "graph_exception.h"

using namespace std;

class AdjInCursor : public InCursor
{
   public:
    AdjInCursor(WT_CURSOR *cur, WT_SESSION *sess) : InCursor(cur, sess) {}

    void next(adjlist *found)
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

        node_id_t curr_key;
        CommonUtil::get_key(cursor, &curr_key);

        if (keys.end != -1 && curr_key > keys.end)
        {
            goto no_next;
        }

        CommonUtil::record_to_adjlist(session, cursor, found);
        found->node_id = curr_key;

        if (cursor->next(cursor) != 0)
        {
            has_next = false;
        }
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
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

        CommonUtil::set_key(cursor, key);

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

        int curr_key;
        CommonUtil::get_key(cursor, &curr_key);

        if (curr_key == key)
        {
            CommonUtil::record_to_adjlist(session, cursor, found);
        }

        if (keys.end != -1 && curr_key > keys.end)
        {
            has_next = false;
        }

        if (cursor->next(cursor) != 0)
        {
            has_next = false;
        }
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }
};

class AdjOutCursor : public OutCursor
{
   public:
    AdjOutCursor(WT_CURSOR *cur, WT_SESSION *sess) : OutCursor(cur, sess) {}

    void next(adjlist *found)
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

        int curr_key;
        CommonUtil::get_key(cursor, &curr_key);

        if (keys.end != -1 && curr_key > keys.end)
        {
            goto no_next;
        }

        CommonUtil::record_to_adjlist(session, cursor, found);
        found->node_id = curr_key;

        if (cursor->next(cursor) != 0)
        {
            has_next = false;
        }
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
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

        CommonUtil::set_key(cursor, key);

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

        int curr_key;
        CommonUtil::get_key(cursor, &curr_key);

        if (curr_key == key)
        {
            CommonUtil::record_to_adjlist(session, cursor, found);
        }

        if (keys.end != -1 && curr_key > keys.end)
        {
            has_next = false;
        }

        if (cursor->next(cursor) != 0)
        {
            has_next = false;
        }
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }
};

class AdjNodeCursor : public NodeCursor
{
   public:
    AdjNodeCursor(WT_CURSOR *cur, WT_SESSION *sess) : NodeCursor(cur, sess) {}

    // use key_pair to define start and end keys.
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

class AdjEdgeCursor : public EdgeCursor
{
   public:
    AdjEdgeCursor(WT_CURSOR *cur, WT_SESSION *sess) : EdgeCursor(cur, sess) {}

    AdjEdgeCursor(WT_CURSOR *cur, WT_SESSION *sess, bool get_weight)
        : EdgeCursor(cur, sess, get_weight)
    {
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
            if (get_weight)
            {
                CommonUtil::record_to_edge(cursor, found);
            }
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

class AdjList : public GraphBase
{
   public:
    AdjList() = default;
    AdjList(graph_opts &opt_params,
            WT_CONNECTION *connection);  // TODO: merge the 2 constructors
    static void create_wt_tables(
        graph_opts &opts, WT_CONNECTION *conn);  // Need this to init graph db
    int add_node(node to_insert);
    void add_node(node_id_t to_insert,
                  std::vector<node_id_t> &inlist,
                  std::vector<node_id_t> &outlist);
    bool has_node(node_id_t node_id);
    node get_node(node_id_t node_id);
    int delete_node(node_id_t node_id);
    node get_random_node();
    degree_t get_in_degree(node_id_t node_id);
    degree_t get_out_degree(node_id_t node_id);
    std::vector<node> get_nodes();

    int add_edge(edge to_insert, bool is_bulk);
    bool has_edge(node_id_t src_id, node_id_t dst_id);
    int delete_edge(node_id_t src_id, node_id_t dst_id);
    edge get_edge(node_id_t src_id, node_id_t dst_id);
    std::vector<edge> get_edges();
    std::vector<edge> get_out_edges(node_id_t node_id);
    std::vector<node> get_out_nodes(node_id_t node_id);
    std::vector<node_id_t> get_out_nodes_id(node_id_t node_id);
    std::vector<edge> get_in_edges(node_id_t node_id);
    std::vector<node> get_in_nodes(node_id_t node_id);
    std::vector<node_id_t> get_in_nodes_id(node_id_t node_id);
    std::string get_db_name() const { return opts.db_name; };
    std::vector<node_id_t> get_adjlist(WT_CURSOR *cursor, node_id_t node_id);
    OutCursor *get_outnbd_iter();
    InCursor *get_innbd_iter();
    NodeCursor *get_node_iter();
    EdgeCursor *get_edge_iter();

    // edgeweight_t get_edge_weight(node_id_t src_id, node_id_t dst_id);
    [[maybe_unused]] void update_edge_weight(
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

    WT_CURSOR *get_new_node_cursor();
    WT_CURSOR *get_new_edge_cursor();
    WT_CURSOR *get_new_in_adjlist_cursor();
    WT_CURSOR *get_new_out_adjlist_cursor();
    WT_CURSOR *get_new_random_outadj_cursor();

   private:
    // structure of the graph
    int node_attr_size = 0;  // set on checking the list len

    WT_CURSOR *node_cursor = NULL;
    WT_CURSOR *random_node_cursor = NULL;
    WT_CURSOR *edge_cursor = NULL;
    WT_CURSOR *in_adjlist_cursor = NULL;
    WT_CURSOR *out_adjlist_cursor = NULL;

    // AdjList specific internal methods:
    [[maybe_unused]] node get_next_node(WT_CURSOR *n_cur);
    [[maybe_unused]] edge get_next_edge(WT_CURSOR *e_cur);
    int add_adjlist(WT_CURSOR *cursor, node_id_t node_id);
    void add_adjlist(WT_CURSOR *cursor,
                     node_id_t node_id,
                     std::vector<node_id_t> &list);
    int delete_adjlist(WT_CURSOR *cursor, node_id_t node_id);
    [[maybe_unused]] void delete_node_from_adjlists(node_id_t node_id);
    int add_to_adjlists(WT_CURSOR *cursor,
                        node_id_t node_id,
                        node_id_t to_insert);
    int delete_from_adjlists(WT_CURSOR *cursor,
                             node_id_t node_id,
                             node_id_t to_delete);
    int delete_related_edges_and_adjlists(node_id_t node_id,
                                          int *num_edges_deleted);
    int update_node_degree(WT_CURSOR *cursor,
                           node_id_t node_id,
                           degree_t indeg,
                           degree_t outdeg);

    [[maybe_unused]] void dump_tables();
    void create_indices() { return; }  // here because defined in interface

    int add_one_node_degree(WT_CURSOR *cursor,
                            node_id_t to_update,
                            bool is_out_degree);
    int remove_one_node_degree(node_id_t to_update, bool is_out_degree);

    int add_node_in_txn(node to_insert);
    int delete_edge_in_txn(node_id_t src_id, node_id_t dst_id);

    int error_check_insert_txn(int return_val, bool ignore_duplicate_key);
    int error_check_update_txn(int return_val);
    int error_check_read_txn(int return_val);
    int error_check_remove_txn(int return_val);
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