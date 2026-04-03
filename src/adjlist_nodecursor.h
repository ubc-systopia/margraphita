#ifndef ADJLIST_NODECURSOR_H
#define ADJLIST_NODECURSOR_H
#include "common_util.h"
#include "graph.h"
class AdjNodeCursor : public NodeCursor
{
private:
  // Used when iterating via adjlist (either !MK_NEDGES, or MK_NEDGES+!read_opt).
  WT_CURSOR *in_cur = nullptr;
  degree_t get_in_degree(node_id_t id)
  {
    if (in_cur == nullptr) return 0;
    CommonUtil::set_key(in_cur, id);
    int ret = in_cur->search(in_cur);
    if (ret == 0)
    {
      adjlist temp;
      CommonUtil::record_to_adjlist(in_cur, &temp);
      return temp.degree;
    }
    return 0;
  }
 public:
  AdjNodeCursor(WT_CURSOR *cur, WT_SESSION *sess)
  {
    cursor = cur;
    session = sess;
    // in_cur is needed when iterating via adjlist (either !MK_NEDGES or
    // MK_NEDGES+!read_opt). Without read_opt we can't tell here, so open it
    // for directed graphs unconditionally when the simple constructor is used.
    if (directed) {
      GraphBase::_get_table_cursor(IN_ADJLIST, &in_cur, session, false, false, "");
    }
  }
  AdjNodeCursor(WT_CURSOR *cur,
                WT_SESSION *sess,
                bool is_directed,
                bool is_read_optimized)
  {
    cursor = cur;
    session = sess;
    directed = is_directed;
    read_opt = is_read_optimized;
    // Open in_cur for directed graphs when using adjlist-based iteration:
    // always when !MK_NEDGES, and also when MK_NEDGES+!read_opt (node table
    // has no degree data in that mode).
    if (directed && !read_opt) {
      GraphBase::_get_table_cursor(IN_ADJLIST, &in_cur, session, false, false, "");
    }
  }
  ~AdjNodeCursor() override = default;



  void set_key_range(key_range _keys) override
  {
    keys = _keys;
    is_first = false;

    // Advances the cursor to the first valid record in range
    if (keys.start != OutOfBand_ID_MAX)
    {
      int status;
      CommonUtil::set_key(cursor, keys.start);
      int ret = cursor->search_near(cursor, &status);
      if (ret != 0)
      {
        // search_near failed, cursor is not positioned
        this->has_next = false;
      }
      else if (status < 0)
      {
        // search_near succeeded but found a record before the search key
        // Advances the cursor
        if (cursor->next(cursor) != 0)
        {
          this->has_next = false;
        }
      }
      // If ret == 0 and status >= 0, cursor is positioned at or after the search key
    }
    else
    {
      // Advances the cursor to the first position in the table.
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

  // use key_pair to define start and end keys.
  void next(node *found) override
  {
    if (!has_next)
    {
      no_next(found);
      return;
    }

    CommonUtil::get_key(cursor, &found->id);
    if (keys.end != OutOfBand_ID_MAX &&
        found->id > keys.end)  // gone beyond the end of range
    {
      no_next(found);
      return;
    }
#ifdef MK_NEDGES
    if (read_opt)
    {
      // read_optimize=true: degrees are in the node table; decode them directly.
      CommonUtil::record_to_node(cursor, found, read_opt, directed);
    }
    else
#endif
    {
      // !read_opt (or !MK_NEDGES): cursor is over out_adjlist; decode the blob.
      adjlist temp;
      CommonUtil::record_to_adjlist(cursor, &temp);
      found->out_degree = temp.degree;
      directed ? found->in_degree = get_in_degree(found->id)
               : found->in_degree = temp.degree;
    }
    if (cursor->next(cursor) != 0)
    {
      has_next = false;
    }
  }
  //! TEST ME
  void next(node *found, node_id_t key) override
  {
    // Must reset if already no_next or if requested key is out of range
    if ((!has_next) || (keys.end != OutOfBand_ID_MAX && key > keys.end) ||
        (keys.start != OutOfBand_ID_MAX && key < keys.start))
    {
      no_next(found);
      return;
    }

    CommonUtil::set_key(cursor, key);
    int status;
    int ret = cursor->search_near(cursor, &status);
    if (ret != 0)
    {
      // search_near failed
      has_next = false;
      no_next(found);
      return;
    }
    else if (status < 0)
    {
      if (cursor->next(cursor) != 0)
      {
        has_next = false;
        no_next(found);
        return;
      }
    }
    node_id_t curr_key;
    CommonUtil::get_key(cursor, &curr_key);

    if (curr_key == key)
    {
#ifdef MK_NEDGES
      if (read_opt)
      {
        CommonUtil::record_to_node(cursor, found, read_opt, directed);
      }
      else
#endif
      {
        adjlist temp;
        CommonUtil::record_to_adjlist(cursor, &temp);
        found->out_degree = temp.degree;
        directed ? found->in_degree = get_in_degree(found->id)
                 : found->in_degree = temp.degree;
      }
      found->id = curr_key;
    }

    // no relevant node was found.
    if (keys.end != OutOfBand_ID_MAX && curr_key > keys.end)
    {
      has_next = false;
    }
  }
};

#endif