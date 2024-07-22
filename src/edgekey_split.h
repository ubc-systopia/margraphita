#ifndef EDGELIST_H
#define EDGELIST_H

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common_util.h"
#include "graph.h"
#include "graph_exception.h"

class SplitEdgeKey : public GraphBase
{
   public:
    SplitEdgeKey(graph_opts &opt_params,
                 WT_CONNECTION *connection);  // TODO: merge the 2 constructors
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

    [[nodiscard]] WT_CURSOR *get_out_edge_cursor() const
    {
        return out_edge_cursor;
    }
    WT_CURSOR *get_new_out_cursor();

    [[nodiscard]] WT_CURSOR *get_random_node_cursor() const
    {
        return random_node_cursor;
    }
    [[nodiscard]] WT_CURSOR *get_in_edge_cursor() const
    {
        return in_edge_cursor;
    }
    WT_CURSOR *get_new_in_cursor();
    [[nodiscard]] WT_CURSOR *get_node_index_cursor() const
    {
        return dst_src_idx_cursor;
    }
    WT_CURSOR *get_new_node_index_cursor();

    static void create_indices(WT_SESSION *session);

   private:
    WT_CURSOR *out_edge_cursor = nullptr;
    WT_CURSOR *random_node_cursor = nullptr;
    WT_CURSOR *in_edge_cursor = nullptr;
    WT_CURSOR *dst_src_idx_cursor = nullptr;

    // internal methods
    [[maybe_unused]] WT_CURSOR *get_metadata_cursor();
    int delete_node_and_related_edges(node_id_t node_id, int *num_edges_to_del);
    int update_node_degree(node_id_t node_id, int in_change, int out_change);
    int add_edge_only(edge to_insert);
    int add_node_txn(node to_insert);
    int error_check_add_edge(int ret);
    int error_check_insert_txn(int return_val);

    [[maybe_unused]] inline void close_all_cursors() override
    {
        out_edge_cursor->close(out_edge_cursor);
        random_node_cursor->close(random_node_cursor);
        in_edge_cursor->close(in_edge_cursor);
        dst_src_idx_cursor->close(dst_src_idx_cursor);
    }
};

class SplitEkeyInCursor : public InCursor
{
   private:
    // bool is_weighted = false;
    node_id_t curr_node{};

   public:
    SplitEkeyInCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        cursor = cur;
        session = sess;
        set_key_range({OutOfBand_ID_MIN, OutOfBand_ID_MAX});
    }
    ~SplitEkeyInCursor() override = default;

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
        if (status <= 0)
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
            found->degree = UINT32_MAX;
            found->edgelist.clear();
            return;
        }

        // get edge
        CommonUtil::ekey_get_key(cursor, &dst, &src);
        if (src == OutOfBand_ID_MIN) curr_node = dst;
        while (cursor->next(cursor) == 0)
        {
            CommonUtil::ekey_get_key(cursor, &dst, &src);
            found->node_id = curr_node;
            if (dst == curr_node && src != OutOfBand_ID_MIN)
            {
                found->edgelist.push_back(src);
                found->degree++;
            }
            else
            {
                curr_node = dst;
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

class SplitEKeyOutCursor : public OutCursor
{
   private:
    // bool is_weighted = false;
    node_id_t curr_node{};

   public:
    SplitEKeyOutCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        cursor = cur;
        session = sess;
        set_key_range({OutOfBand_ID_MIN, OutOfBand_ID_MAX});
    }
    ~SplitEKeyOutCursor() override = default;
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
            found->degree = UINT32_MAX;
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

class SplitEKeyNodeCursor : public NodeCursor
{
   public:
    // Takes a composite index cursor on (dst, src)
    SplitEKeyNodeCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        cursor = cur;
        session = sess;
        set_key_range(
            {OutOfBand_ID_MIN, OutOfBand_ID_MAX});  // min and max node id
    }
    ~SplitEKeyNodeCursor() override = default;

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

class SplitEKeyEdgeCursor : public EdgeCursor
{
   private:
    // bool at_node =true; //initial state
   public:
    SplitEKeyEdgeCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        cursor = cur;
        session = sess;
        set_key_range({{OutOfBand_ID_MIN, OutOfBand_ID_MIN},
                       {OutOfBand_ID_MAX, OutOfBand_ID_MAX}});
    }
    ~SplitEKeyEdgeCursor() override = default;

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
            // node_id_t temp_src, temp_dst;
            // CommonUtil::ekey_get_key(cursor, &temp_src,&temp_dst);
            // std::cout << "first edge (src,dst): " <<
            // temp_src << ", " <<temp_dst << std::endl;
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

#endif