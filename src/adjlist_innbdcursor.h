#ifndef ADJLIST_INNBDCURSOR_H
#define ADJLIST_INNBDCURSOR_H
#include "common_util.h"

class AdjInCursor : public InCursor
{
 private:
  bool all_nodes = false;

 public:
  void setAllNodes(bool allNodes) { all_nodes = allNodes; }

 public:
  AdjInCursor(WT_CURSOR *cur, WT_SESSION *sess)
  {
    cursor = cur;
    session = sess;
  }
  AdjInCursor(WT_CURSOR *cur,
              WT_SESSION *sess,
              bool is_directed,
              bool read_optimized)
  {
    cursor = cur;
    session = sess;
    directed = is_directed;
    read_opt = read_optimized;
  }
  ~AdjInCursor() override = default;

  void set_key_range(key_range _key) override
  {
    keys = _key;
    is_first = false;

    // Advances the cursor to the first valid record in range
    if (keys.start != OutOfBand_ID_MAX)
    {
      int status;
      CommonUtil::set_key(cursor, keys.start);
      // search_near can fail (e.g. empty table) — must check return value
      // before using `status`, which is undefined on failure.
      int ret = cursor->search_near(cursor, &status);
      if (ret != 0)
      {
        this->has_next = false;
      }
      else if (status < 0)
      {
        // Positioned before the search key; advance to first valid record
        if (cursor->next(cursor) != 0)
        {
          this->has_next = false;
        }
      }
    }
    else
    {
      // Advances the cursor to first position
      if (cursor->next(cursor) != 0)
      {
        this->has_next = false;
      }
    }
  }
  void no_next(adjlist *found)
  {
    found->degree = UINT32_MAX;
    found->edgelist.clear();
    found->node_id = OutOfBand_ID_MAX;
    has_next = false;
  }

  void next(adjlist *found) override
  {
    if (!has_next)
    {
      no_next(found);
      return;
    }

    node_id_t curr_key;
    do
    {
      CommonUtil::get_key(cursor, &curr_key);

      if (keys.end != OutOfBand_ID_MAX &&
          curr_key > keys.end)  // there is an end key and we have passed it
      {
        no_next(found);
        return;
      }

      CommonUtil::record_to_adjlist(cursor, found);
      found->node_id = curr_key;

      if (cursor->next(cursor) != 0)
      {
        has_next = false;
      }
    // Guard: stop skipping zero-degree nodes if cursor is exhausted,
    // otherwise this loop spins forever when all remaining nodes have degree 0.
    } while (found->degree == 0 && all_nodes == false && has_next);

    // If we exited because has_next became false while still on a zero-degree
    // node, signal end-of-iteration to the caller.
    if (found->degree == 0 && !all_nodes)
    {
      return no_next(found);
    }
  }

  void next(adjlist *found, node_id_t key) override {}
};

#endif  // ADJLIST_INNBDCURSOR_H