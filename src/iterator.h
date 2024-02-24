#ifndef ITERATOR_H
#define ITERATOR_H
#include <wiredtiger.h>

#include <iostream>

#include "common_defs.h"
/**
 * @brief The following are iterator definitions.
 * 
 */
 
class table_iterator
{
   protected:
    WT_CURSOR *cursor = nullptr;
    WT_SESSION *session = nullptr;
    bool is_first = true;
    bool has_next = true;

   public:
    table_iterator() = default;
    [[nodiscard]] bool has_more() const { return has_next; };
    virtual void reset()
    {
        int ret = cursor->reset(cursor);
        if (ret != 0)
        {
            std::cout << "Error in resetting cursor" << wiredtiger_strerror(ret)
                      << std::endl;
        }
        is_first = true;
        has_next = true;
    }
    void close()
    {
        cursor->close(cursor);
        // session->close(session, nullptr);
    }
};

class OutCursor : public table_iterator
{
   protected:
    key_range keys{};
    // keyrange because out_nbd is defined for a node id range
    node_id_t num_nodes{};

   public:
    OutCursor() = default;
    virtual void set_key_range(key_range _keys) = 0;
    void set_num_nodes(uint32_t num) { num_nodes = num; }

    virtual void next(adjlist *found) = 0;
    virtual void next(adjlist *found, node_id_t key) = 0;
};

class InCursor : public table_iterator
{
   protected:
    key_range keys{};
    node_id_t num_nodes{};

   public:
    InCursor() = default;

    //! DELETE THIS LIKE IN OUTCURSOR
    virtual void set_key_range(key_range _keys) = 0;
    void set_num_nodes(node_id_t num) { num_nodes = num; }

    virtual void next(adjlist *found) = 0;
    virtual void next(adjlist *found, node_id_t key) = 0;
};

class NodeCursor : public table_iterator
{
   protected:
    key_range keys{};

   public:
    NodeCursor() = default;

    /**
     * @brief Set the key range object
     *
     * @param _keys the key range object. Set the end key to INT_MAX if you
     * want to get all the nodes from start node.
     */
    virtual void set_key_range(key_range _keys)  =0;// overrided by edgekey
//    {
//        keys = _keys;
    //        CommonUtil::ekey_set_key(cursor, keys.start);
    //    }

    virtual void next(node *found) = 0;
    virtual void next(node *found, node_id_t key) = 0;
};

class EdgeCursor : public table_iterator
{
   protected:
    key_pair start_edge{};
    key_pair end_edge{};
    bool get_weight = true;

   public:
    EdgeCursor() = default;
    virtual void set_key_range(edge_range range) = 0;

    virtual void next(edge *found) = 0;
    void dump_range()
    {
        std::cout << "start: " << start_edge.src_id << " " << start_edge.dst_id
                  << " end: " << end_edge.src_id << " " << end_edge.dst_id
                  << std::endl;
    }
};
#endif