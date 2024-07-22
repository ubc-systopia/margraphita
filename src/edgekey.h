#ifndef EDGE_KEY
#define EDGE_KEY

#include "common_util.h"
#include "graph.h"
#include "graph_exception.h"

using namespace std;

class EkeyInCursor : public InCursor
{
    node_id_t curr_node{};
    // This accepts a dst_src_cursor
   public:
    /**
     * @brief Construct a new Ekey In Cursor object
     * @param cur A cursor to the dst_src index
     * @param sess A session object to which the cursor belongs. Used to open
     * the node cursor.
     */
    EkeyInCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        cursor = cur;
        session = sess;
        set_key_range({OutOfBand_ID_MIN, OutOfBand_ID_MAX});
    }
    ~EkeyInCursor() override = default;
    /** This is the initializer function for setting the range of valid keys an
     * iterator must operate within. Set the cursor keys and advance the cursor
     * to the first valid position in the range.
     *
     * @param _keys : The user can specify a start and end node ID in a
     * `key_range` struct, or it gets initialized to
     * `{OutOfBand_ID_MIN,OutOfBand_ID_MIN}`
     */
    void set_key_range(key_range _keys) final
    {
        keys.start = _keys.start;
        if (_keys.end == OutOfBand_ID_MIN)
        {
            keys.end = OutOfBand_ID_MAX;
        }
        else
        {
            keys.end = _keys.end;
        }

        // We need to position this cursor to the first record where dst >
        // OutOfBand_ID_MIN (i.e. 1) and src >= keys.start
        // reversed because (dst, src)
        CommonUtil::ekey_set_key(cursor, (OutOfBand_ID_MIN + 1), keys.start);
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
        node_id_t temp_src;
        CommonUtil::ekey_get_key(cursor, &curr_node, &temp_src);
    }

    void next(adjlist *found) final
    {
        node_id_t src;
        node_id_t dst;
        // edge curr_edge;

        if (!has_next)
        {
            found->degree = UINT32_MAX;  // always u32
            found->edgelist.clear();
            found->node_id = OutOfBand_ID_MAX;
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
        while (cursor->next(cursor) == 0)
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
    // bool is_weighted = false;
    node_id_t curr_node{};

   public:
    EkeyOutCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        cursor = cur;
        session = sess;
        set_key_range({OutOfBand_ID_MIN, OutOfBand_ID_MAX});
    }
    ~EkeyOutCursor() override = default;
    void set_key_range(key_range _keys) override
    {
        keys.start = _keys.start;
        if (_keys.end == OutOfBand_ID_MIN)
        {
            keys.end = OutOfBand_ID_MAX;
        }
        else
        {
            keys.end = _keys.end;
        }

        CommonUtil::ekey_set_key(cursor, keys.start, OutOfBand_ID_MIN);
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
            found->node_id = OutOfBand_ID_MAX;
            found->degree = UINT32_MAX;  // always u32
            found->edgelist.clear();
            return;
        }

        // get edge
        CommonUtil::ekey_get_key(cursor, &src, &dst);
        if (dst == OutOfBand_ID_MIN) curr_node = src;
        while (cursor->next(cursor) == 0)
        {
            CommonUtil::ekey_get_key(cursor, &src, &dst);
            found->node_id = curr_node;
            if (src == curr_node && dst != OutOfBand_ID_MIN)
            {
                found->edgelist.push_back(dst);
                found->degree++;
            }
            else
            {
                curr_node = src;
                if (found->degree == 0) continue;  // don't return empty nodes
                if (curr_node > keys.end)
                {
                    has_next = false;
                    return;
                }
                return;
            }
        }
        // found->node_id = src;
        found->node_id = OutOfBand_ID_MAX;
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
    EkeyNodeCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        cursor = cur;
        session = sess;
        set_key_range(
            {OutOfBand_ID_MIN, OutOfBand_ID_MAX});  // min and max node id
    }
    ~EkeyNodeCursor() override = default;

    void set_key_range(key_range _keys) override
    {
        keys = _keys;
        int status;
        // set the cursor to the first relevant record in range
        if (keys.start != OutOfBand_ID_MIN)
        {
            CommonUtil::ekey_set_key(cursor, OutOfBand_ID_MIN, keys.start);
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
        found->id = OutOfBand_ID_MAX;
        found->in_degree = UINT32_MAX;
        found->out_degree = UINT32_MAX;
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

        if (keys.end != OutOfBand_ID_MIN && curr_edge.src_id > keys.end)
        {
            no_next(found);
        }

        if (curr_edge.dst_id != OutOfBand_ID_MIN)
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
    // bool at_node = true;  // initial state always points here
   public:
    EkeyEdgeCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        cursor = cur;
        session = sess;
        set_key_range({{OutOfBand_ID_MIN, OutOfBand_ID_MIN},
                       {OutOfBand_ID_MAX, OutOfBand_ID_MAX}});
    }
    ~EkeyEdgeCursor() override = default;

    void set_key_range(edge_range range) override
    {
        start_edge = range.start;
        end_edge = range.end;

        // set the cursor to the first relevant record in range
        if (range.start.src_id != OutOfBand_ID_MIN &&
            range.start.dst_id != OutOfBand_ID_MIN)  // the range is not empty
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
            std::cout << "here" << std::endl;
            // Advance the cursor to the first record
            if (cursor->next(cursor) != 0)
            {
                this->has_next = false;
            }
            //            node_id_t temp_src, temp_dst;
            //            CommonUtil::ekey_get_key(cursor, &temp_src,
            //            &temp_dst); std::cout << "first edge (src,dst): " <<
            //            temp_src << ", "
            //                      << temp_dst << std::endl;
        }
    }

    void no_next(edge *found)
    {
        found->src_id = OutOfBand_ID_MAX;
        found->dst_id = OutOfBand_ID_MAX;
        found->edge_weight = UINT32_MAX;
        has_next = false;
    }

    void next(edge *found) override
    {
        if (!has_next)
        {
            no_next(found);
            return;
        }

        while (true)
        {
            CommonUtil::ekey_get_key(cursor, &found->src_id, &found->dst_id);
            if (found->dst_id != OutOfBand_ID_MIN)
            {
                break;  // found an edge
            }
            else
            {
                if (cursor->next(cursor) != 0)
                {
                    no_next(found);
                    return;
                }  // advance to the next edge
            }
        }

        // If end_edge is set
        if (end_edge.src_id != OutOfBand_ID_MAX)
        {
            // If found > end edge
            if (!(found->src_id < end_edge.src_id ||
                  ((found->src_id == end_edge.src_id) &&
                   (found->dst_id <= end_edge.dst_id))))
            {
                no_next(found);
                return;
            }
        }
        if (get_weight)
        {
            CommonUtil::record_to_edge_ekey(cursor, found);
        }

        if (cursor->next(cursor) != 0)
        {
            has_next = false;
            return;
        }
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
    void get_random_node_ids(std::vector<node_id_t> &ids,
                             int num_nodes) override;
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

    node_id_t get_max_node_id() override;
    node_id_t get_min_node_id() override;

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