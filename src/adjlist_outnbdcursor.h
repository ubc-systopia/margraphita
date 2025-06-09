#ifndef ADJLIST_OUTNBDCURSOR_H
#define ADJLIST_OUTNBDCURSOR_H
#include "common_util.h"

/** * @brief This class is used to iterate over the adjacency list in the out * direction.
 * The constructor accepts a cursor to the out table and a session object, and additionally
 * can take parameters to indicate if the graph is directed and if it is read optimized.
 * The setAllNodes method allows the user to specify if all nodes should be returned,
 * regardless of whether they have any outgoing edges or not. This method is useful only when the graph is created in a way that all nodes are present in the out adjacency list table, even if they have no outgoing edges. This is not always true.
 */
class AdjOutCursor : public OutCursor
{
 private:
  bool all_nodes = false;

 public:
  AdjOutCursor(WT_CURSOR *cur, WT_SESSION *sess)
  {
    cursor = cur;
    session = sess;
  }

  AdjOutCursor(WT_CURSOR *cur,
               WT_SESSION *sess,
               bool is_directed,
               bool read_optimized)
  {
    cursor = cur;
    session = sess;
    directed = is_directed;
    read_opt = read_optimized;
  }
  ~AdjOutCursor() override = default;
  void setAllNodes(bool allNodes) { all_nodes = allNodes; }

  // use key_pair to define start and end keys.
  // advance the cursor to the first valid record in range

  void no_next(adjlist *found)
  {
    found->degree = UINT32_MAX;
    found->edgelist.clear();
    found->node_id = OutOfBand_ID_MAX;
    has_next = false;
  }

  void set_key_range(key_range _keys) override
  {
    keys = _keys;
    is_first = false;

    if (keys.start != OutOfBand_ID_MAX)
    {
      int status;
      CommonUtil::set_key(cursor, keys.start);
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
      // Advances the cursor to the first position
      if (cursor->next(cursor) != 0)
      {
        this->has_next = false;
      }
    }
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

      if (keys.end != OutOfBand_ID_MAX && curr_key > keys.end)
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
    } while (found->degree == 0 && all_nodes == false);
  }

  void next(adjlist *found, node_id_t key) override {}
};
#endif  // ADJLIST_OUTNBDCURSOR_H