#ifndef EDGE_KEY
#define EDGE_KEY

#include "common_util.h"
#include "graph.h"
#include "graph_exception.h"

using namespace std;

class EkeyInCursor : public InCursor
{
    //    node_id_t next_expected = 1;
    //    bool data_remaining = true;

    // This accepts a dst_src_cursor
   public:
    EkeyInCursor(WT_CURSOR *cur, WT_SESSION *sess) : InCursor(cur, sess) {}

    void set_key_range(key_range _keys) final
    {
        keys = _keys;
        is_first = false;

        // Now we set the cursor to the first relevant record in range
        if (keys.start != OutOfBand_ID)  // the user DID set a start key
        {
            CommonUtil::set_key(cursor, MAKE_EKEY(keys.start), OutOfBand_ID);
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
        else  // the user did not set a start key
        {
            // Advance the cursor to the first record
            if (cursor->next(cursor) != 0)
            {
                this->has_next = false;
            }
        }
    }

    void next(adjlist *found) final
    {
        node_id_t src;
        node_id_t dst;
        edge curr_edge;
        int ret = 0;

        if (!has_next) goto no_next;

        // dst_src_cursor. pay attention to the (dst,src) order here
        CommonUtil::get_key(cursor, &dst, &src);
        // check if we are past the valid range
        if (keys.end != OutOfBand_ID && OG_KEY(dst) > keys.end)
        {
            goto no_next;
        }
        found->degree = 0;
        found->edgelist.clear();
        found->node_id = OG_KEY(dst);

        do
        {
            // dst_src_cursor. pay attention to the (dst,src) order here
            CommonUtil::get_key(cursor, &curr_edge.dst_id, &curr_edge.src_id);
            if (dst == curr_edge.dst_id)
            {
                found->degree++;
                found->edgelist.push_back(OG_KEY(curr_edge.src_id));
            }
            else
            {
                return;
            }

        } while ((ret = cursor->next(cursor) == 0));
        if (ret != 0) has_next = false;
        return;

    no_next:
        found->degree = -1;
        found->edgelist.clear();
        found->node_id = -1;
        has_next = false;
    }

    void next(adjlist *found, node_id_t key) final
    {
        edge curr_edge;
        int ret = 0;
        if (!has_next)
        {
            goto no_next;
        }

        // Access outside of range not permitted
        if ((keys.end != OutOfBand_ID && key > keys.end) ||
            (keys.start != OutOfBand_ID && key < keys.start))
        {
            goto no_next;
        }

        CommonUtil::set_key(cursor, MAKE_EKEY(key), OutOfBand_ID);

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
            CommonUtil::get_key(cursor, &curr_edge.dst_id, &curr_edge.src_id);
            if (OG_KEY(curr_edge.dst_id) != key)
            {
                if (keys.end != OutOfBand_ID &&
                    OG_KEY(curr_edge.dst_id) > keys.end)
                {
                    has_next = false;
                }
                return;
            }
            found->edgelist.push_back(OG_KEY(curr_edge.src_id));
            found->degree++;
        } while ((ret = cursor->next(cursor)) == 0);
        if (ret != 0) has_next = false;
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
    bool data_remaining = true;
    node_id_t last_node_id = 1;
    // bool is_weighted = false;

   public:
    EkeyOutCursor(WT_CURSOR *cur, WT_SESSION *sess) : OutCursor(cur, sess)
    {
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys) override
    {
        keys = _keys;
        CommonUtil::set_key(cursor, MAKE_EKEY(keys.start), OutOfBand_ID);
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
        // advance the cursor again to hit the adjacency list
        if (cursor->next(cursor) != 0)
        {
            data_remaining = false;
            return;
        }
        // At the end of this call, the cursor points to the adjacency list of
        // the first node >= start
    }

    void next(adjlist *found) override
    {
        //      node_id_t src;
        //      node_id_t dst;
        edge curr_edge;

        if (!has_next)
        {
            goto no_next;
        }

        // edgeweight_t tmp;
        do
        {
            CommonUtil ::get_key(cursor, &curr_edge.src_id, &curr_edge.dst_id);
            // if (is_weighted) cursor->get_value(cursor,
            // &curr_edge.edge_weight, &tmp);
            found->node_id = OG_KEY(curr_edge.src_id);
            if (curr_edge.src_id > last_node_id &&
                curr_edge.src_id <= MAKE_EKEY(keys.end))
            {
                if (curr_edge.dst_id != 0)
                {
                    found->degree++;
                    found->edgelist.push_back(OG_KEY(curr_edge.dst_id));
                }
            }
            else
            {
                // We have advanced to the next node
                last_node_id = curr_edge.src_id;
                if (cursor->next(cursor) != 0)
                    has_next = false;  // advance to the next node
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
