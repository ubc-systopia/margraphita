#ifndef ITERATOR_H
#define ITERATOR_H
#include <wiredtiger.h>

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
    void init(WT_CURSOR *cursor_, WT_SESSION *sess_)
    {
        cursor = cursor_;
        session = sess_;
    }

   public:
    //    virtual void ekey_set_key(
    //        node_id_t key) = 0;  //{ CommonUtil::ekey_set_key(cursor, key); }
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
};

class OutCursor : public table_iterator
{
   protected:
    key_range
        keys{};  // keyrange because out_nbd is defined for a node id range
    int num_nodes{};

   public:
    OutCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        init(cur, sess);
        keys = {-1, -1};  // default key range
    }

    virtual void set_key_range(key_range _keys) = 0;
    void set_num_nodes(int num) { num_nodes = num; }

    virtual void next(adjlist *found) = 0;
    virtual void next(adjlist *found, node_id_t key) = 0;
};

class InCursor : public table_iterator
{
   protected:
    key_range keys{};
    int num_nodes{};

   public:
    InCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        init(cur, sess);
        keys = {-1, -1};
    }

    //! DELETE THIS LIKE IN OUTCURSOR
    virtual void set_key_range(key_range _keys) = 0;
    void set_num_nodes(int num) { num_nodes = num; }

    virtual void next(adjlist *found) = 0;
    virtual void next(adjlist *found, node_id_t key) = 0;
};

class NodeCursor : public table_iterator
{
   protected:
    key_range keys{};

   public:
    NodeCursor(WT_CURSOR *node_cur, WT_SESSION *sess)
    {
        init(node_cur, sess);
        keys = {-1, -1};
    }

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
    EdgeCursor(WT_CURSOR *composite_edge_cur, WT_SESSION *sess)
    {
        init(composite_edge_cur, sess);
        start_edge = {-1, -1};
        end_edge = {-1, -1};
    }

//    EdgeCursor(WT_CURSOR *composite_edge_cur, WT_SESSION *sess, bool get_weight)
//    {
//        init(composite_edge_cur, sess);
//        start_edge = {-1, -1};
//        end_edge = {-1, -1};
//        this->get_weight = get_weight;
//    } // for edgekey
//    //! This is not necessary.

    // Overwrites ekey_set_key(int key) implementation in table_iterator
    //    void ekey_set_key(int key) = delete;

    virtual void set_key_range(edge_range range) = 0;

    virtual void next(edge *found) = 0;
};
#endif