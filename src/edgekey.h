#ifndef EDGE_KEY
#define EDGE_KEY

#include "common_util.h"
#include "graph.h"
#include "graph_exception.h"

using namespace std;

class EkeyInCursor : public InCursor
{
    node_id_t curr_node, curr_edge_dst;
    bool more_edges = true;
    WT_CURSOR *node_ptr = nullptr;

    // This accepts a dst_src_cursor
   public:
    EkeyInCursor(WT_CURSOR *cur, WT_SESSION *sess) : InCursor(cur, sess) {}

    // void set_key_range(key_range _keys) final
    // {
    //     keys.start = _keys.start;
    //     if (_keys.end == OutOfBand_ID)
    //     {
    //         keys.end = INT32_MAX;
    //     }
    //     else
    //     {
    //         keys.end = _keys.end;
    //     }

    //     // We need to position this cursor to the first record where dst >
    //     // OutOfBand_ID (i.e. 1) and src >= keys.start reversed because (dst,
    //     // src)
    //     CommonUtil::ekey_set_key(cursor, (OutOfBand_ID + 1), keys.start);
    //     int status;
    //     cursor->search_near(cursor, &status);
    //     if (status < 0)
    //     {
    //         // Advances the cursor
    //         if (cursor->next(cursor) != 0)
    //         {
    //             has_next = false;
    //             return;
    //         }
    //     }
    //     node_id_t temp_src;
    //     CommonUtil::ekey_get_key(cursor, &curr_node, &temp_src);
    //     // std::cout << "curr_node: " << curr_node << " temp_src: " <<
    //     temp_src
    //     //           << std::endl;
    // }

    void set_key_range(key_range _keys) override
    {
        keys.start = _keys.start;
        if (_keys.end == OutOfBand_ID)
        {
            keys.end = INT32_MAX;
        }
        else
        {
            keys.end = _keys.end;
        }
        session->open_cursor(session,
                             "index:edge:IX_edge_dstsrc(attr_fst,attr_scnd)",
                             nullptr,
                             nullptr,
                             &node_ptr);
        
        node_id_t temp_dst, temp_src;

        CommonUtil::ekey_set_key(node_ptr, OutOfBand_ID, keys.start);
        int status;
        node_ptr->search_near(node_ptr, &status);
        std::cout << "status is "<<status << std::endl;
        if (status < 0)
        {
            // Advances the cursor
            if (node_ptr->next(node_ptr) != 0)
            {
                has_next = false;
                return;
            }
        }

        CommonUtil::ekey_get_key(node_ptr, &temp_dst, &curr_node); // temp_dst = OutOfBand_ID for nodes
        // std::cout << "from setting node (dst, src): " << curr_node << ", " << temp1 << std::endl;
        
        
        // We need to position this cursor to the first record where dst >
        // OutOfBand_ID (i.e. 1) and src >= keys.start reversed because (dst,
        // src)
        CommonUtil::ekey_set_key(cursor, (OutOfBand_ID + 1), keys.start);
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
        
        CommonUtil::ekey_get_key(cursor, &temp_dst, &temp_src);
        // std::cout << "From settin edge (dst,src): " << temp_dst << ", " << temp_src<< std::endl;
    }

    void next(adjlist *found) final
    {
        node_id_t temp_src, temp_dst;
        int res = 0;

        if (!has_next)
        {
            found->degree = -1;
            found->edgelist.clear();
            found->node_id = -1;
            has_next = false;
            return;
        }

        // get edge
        if (!more_edges) goto no_more_edges;
        CommonUtil::ekey_get_key(cursor, &curr_edge_dst, &temp_src);
        if (curr_edge_dst == curr_node)
        {
            found->node_id = curr_edge_dst;
            found->edgelist.push_back(temp_src);
            found->degree++;
        }
        else
        {
            no_more_edges:
            //this branch will be hit for all nodes that don't have an incoming edge

            found->node_id = curr_node;
            found->degree = 0;
            found->edgelist.clear();
            
            node_ptr->next(node_ptr);
            CommonUtil::ekey_get_key(node_ptr, &temp_src, &curr_node);
            if(curr_node > keys.end || temp_src != OutOfBand_ID)
            {
                has_next = false;
            }
            return;
        }

        // now advance till we hit another dst
        while (cursor->next(cursor) == 0)
        {
            CommonUtil::ekey_get_key(cursor, &curr_edge_dst, &temp_src);
            if (curr_edge_dst == curr_node)
            {
                found->edgelist.push_back(temp_src);
                found->degree++;
            }
            else
            {
                
                //We have hit a new dst, so we need to update the node cursor
                node_ptr->next(node_ptr);
                CommonUtil::ekey_get_key(node_ptr, &temp_src, &curr_node);
                if(curr_node == curr_edge_dst)
                // next node matches the node we are on the in-edge for
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
                    //next node is not the same as the node we are on the in-edge for
                    //nothing to do here, just return. this is handled above.
                    return;
                }
                
            }
        }
        //no more edges. check if there are any more nodes
        node_ptr->next(node_ptr);
        CommonUtil::ekey_get_key(node_ptr, &temp_src, &curr_node);
        if(curr_node <= keys.end && temp_src == OutOfBand_ID) // valid, in-range node.
        {
            //we still have work to do.
             more_edges = false;
            return;
        }
        else{
            has_next = false;
        }
       
    }

    /**
     * @brief What I'm trying here is to duplicate the cursor, and then keep one
     * cursor at the nodes part of the index, and use the other cursor to
     * iterate over the edges. This is a hacky solution, but I'm not sure how
     * else to do it.
     *
     * @param found
     */
    void next_original(adjlist *found)
    {
        node_id_t src;
        node_id_t dst;
        edge curr_edge;
        int res = 0;

        if (!has_next)
        {
            found->degree = -1;
            found->edgelist.clear();
            found->node_id = -1;
            has_next = false;
            return;
        }

        // get edge
        CommonUtil::ekey_get_key(cursor, &dst, &src);
        found->node_id = dst;
        if (dst == curr_node)
        {
            found->edgelist.push_back(src);
            found->degree++;
        }

        // now advance till we hit another dst
        while (res = cursor->next(cursor) == 0)
        {
            CommonUtil::ekey_get_key(cursor, &dst, &src);
            if (dst == curr_node)
            {
                found->edgelist.push_back(src);
                found->degree++;
            }
            else
            {
                curr_node = dst;
                if (curr_node > keys.end)
                {
                    has_next = false;
                    return;
                }
                return;
            }
        }
        has_next = false;
    }

    void next(adjlist *found, node_id_t key) final {}
};

class EkeyOutCursor : public OutCursor
{
   private:
    bool data_remaining = true;
    // bool is_weighted = false;
    node_id_t curr_node;

   public:
    EkeyOutCursor(WT_CURSOR *cur, WT_SESSION *sess) : OutCursor(cur, sess) {}

    void set_key_range(key_range _keys) override
    {
        keys.start = _keys.start;
        if (_keys.end == OutOfBand_ID)
        {
            keys.end = INT32_MAX;
        }
        else
        {
            keys.end = _keys.end;
        }

        CommonUtil::ekey_set_key(cursor, keys.start, OutOfBand_ID);
        // Advance the cursor to the first record >= start

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
        node_id_t temp_dst;
        CommonUtil::ekey_get_key(cursor, &curr_node, &temp_dst);
    }

    void next(adjlist *found) override
    {
        node_id_t src, dst;

        if (!has_next)
        {
            found->node_id = -1;
            found->degree = -1;
            found->edgelist.clear();
            return;
        }

        // get edge
        CommonUtil::ekey_get_key(cursor, &src, &dst);
        if (dst == OutOfBand_ID) curr_node = src;
        int res = 0;
        while ((res = cursor->next(cursor)) == 0)
        {
            CommonUtil::ekey_get_key(cursor, &src, &dst);
            found->node_id = curr_node;
            if (src == curr_node && dst != OutOfBand_ID)
            {
                found->edgelist.push_back(dst);
                found->degree++;
            }
            else
            {
                curr_node = src;
                if (curr_node > keys.end)
                {
                    has_next = false;
                    return;
                }
                return;
            }
        }
        found->node_id = src;
        has_next = false;
    }

    void next(adjlist *found, node_id_t key) override {}
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

    void set_key_range(key_range _keys) override
    {
        keys = _keys;
        int status;
        node_id_t temp1, temp2;
        // set the cursor to the first relevant record in range
        if (keys.start != OutOfBand_ID)
        {
            CommonUtil::ekey_set_key(cursor, OutOfBand_ID, keys.start);
            // flipped because (dst, src)
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
            // Advance the cursor to the first record
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

    void next(node *found) override
    {
        edge curr_edge;
        if (!has_next)
        {
            no_next(found);
        }

        CommonUtil::ekey_get_key(cursor, &curr_edge.dst_id, &curr_edge.src_id);
        found->id = curr_edge.src_id;
        cursor->get_value(cursor, &found->in_degree, &found->out_degree);

        if (keys.end != OutOfBand_ID && curr_edge.src_id > keys.end)
        {
            no_next(found);
        }

        if (curr_edge.dst_id != OutOfBand_ID)
        {
            no_next(found);
        }
        int ret = cursor->next(cursor);
        if (ret != 0)
        {
            std::cout << wiredtiger_strerror(ret) << std::endl;
            has_next = false;
            return;
        }
    }

    void next(node *found, node_id_t key) override {}
};

class EkeyEdgeCursor : public EdgeCursor
{
   private:
    bool at_node = true;  // initial state always points here
   public:
    EkeyEdgeCursor(WT_CURSOR *cur, WT_SESSION *sess) : EdgeCursor(cur, sess) {}

    void set_key_range(edge_range range) override
    {
        start_edge = range.start;
        end_edge = range.end;

        // set the cursor to the first relevant record in range
        if (range.start.src_id != -1 &&
            range.start.dst_id != -1)  // the range is not empty
        {
            CommonUtil::ekey_set_key(
                cursor, range.start.src_id, range.start.dst_id);
            int status;
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
        else  // the range is empty
        {
            // Advance the cursor to the first record
            if (cursor->next(cursor) != 0)
            {
                this->has_next = false;
            }
        }
    }

    void next(edge *found) override
    {
        if (!has_next)
        {
            goto no_next;
        }

        if (cursor->next(cursor) != 0)
        {
            goto no_next;
        }

        while (true)
        {
            if (found->dst_id != OutOfBand_ID)
            {
                break;  // found an edge
            }
            else
            {
                if (cursor->next(cursor) != 0)
                {
                    goto no_next;
                }  // advance to the next edge
            }
        }

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
            CommonUtil::record_to_edge_ekey(cursor, found);
        }
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
    EdgeKey(graph_opts &opt_params,
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
    edge get_edge(node_id_t src_id, node_id_t dst_id) override;
    std::vector<edge> get_edges() override;
    std::vector<edge> get_out_edges(node_id_t node_id) override;
    std::vector<node> get_out_nodes(node_id_t node_id) override;
    std::vector<node_id_t> get_out_nodes_id(node_id_t node_id) override;
    std::vector<edge> get_in_edges(node_id_t node_id) override;
    std::vector<node> get_in_nodes(node_id_t node_id) override;
    std::vector<node_id_t> get_in_nodes_id(node_id_t node_id) override;

    OutCursor *get_outnbd_iter() override;
    InCursor *get_innbd_iter() override;
    NodeCursor *get_node_iter() override;
    EdgeCursor *get_edge_iter() override;

    // internal cursor operations:
    void init_cursors();  // todo <-- implement this
    [[maybe_unused]] WT_CURSOR *get_node_cursor();
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_new_edge_cursor();
    [[maybe_unused]] WT_CURSOR *get_new_node_cursor();
    [[maybe_unused]] WT_CURSOR *get_new_random_node_cursor();
    WT_CURSOR *get_dst_src_idx_cursor();
    WT_CURSOR *get_new_dst_src_idx_cursor();
    static void create_indices(WT_SESSION *session);

   private:
    // Cursors
    WT_CURSOR *edge_cursor = nullptr;
    // WT_CURSOR *dst_idx_cursor = nullptr;
    WT_CURSOR *dst_src_idx_cursor = nullptr;
    WT_CURSOR *random_cursor = nullptr;

    // internal methods
    [[maybe_unused]] WT_CURSOR *get_metadata_cursor();
    int delete_node_and_related_edges(node_id_t node_id, int *num_edges_to_del);
    int update_node_degree(node_id_t node_id, degree_t indeg, degree_t outdeg);
    int add_edge_only(edge to_insert);
    int add_node_txn(node to_insert);
    int error_check_add_edge(int ret);

    [[maybe_unused]] void drop_indices();
    [[maybe_unused]] inline void close_all_cursors() override
    {
        edge_cursor->close(edge_cursor);
        dst_src_idx_cursor->close(dst_src_idx_cursor);
        random_cursor->close(random_cursor);
    }
};
#endif
