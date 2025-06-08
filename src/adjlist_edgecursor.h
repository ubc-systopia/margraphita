#ifndef ADJLIST_EDGECURSOR_H
#define ADJLIST_EDGECURSOR_H
#include "common_util.h"

class AdjEdgeCursor : public EdgeCursor
{
  private:
  #ifndef MK_NEDGES
    adjlist current_adjlist;
    int pos = 0;
  #endif
 public:
  AdjEdgeCursor(WT_CURSOR *cur, WT_SESSION *sess)
  {
    cursor = cur;
    session = sess;
  }
  AdjEdgeCursor(WT_CURSOR *cur,
                WT_SESSION *sess,
                bool is_directed,
                bool is_read_optimized)
  {
    cursor = cur;
    session = sess;
    directed = is_directed;
    read_opt = is_read_optimized;
  }
  ~AdjEdgeCursor() override = default;

  void set_key_range(edge_range range) override
  {
    std::cout << "setting keys" << std::endl;
    start_edge = range.start;
    end_edge = range.end;
    is_first = false;

    // Advances the cursor to the first valid record in range
    if (start_edge.src_id != OutOfBand_ID_MAX &&
        start_edge.dst_id != OutOfBand_ID_MAX)
    {
      std::cout << "HERE: " << start_edge.src_id << " "
                << start_edge.dst_id << std::endl;
      int status;
      #ifdef MK_NEDGES
      CommonUtil::set_key(cursor, start_edge.src_id, start_edge.dst_id);
      cursor->search_near(cursor, &status);
      if (status < 0)
      {
        // Advances the cursor
        if (cursor->next(cursor) != 0)
        {
          this->has_next = false;
        }
      }
      #else
      //The cursor is to the out_adj table.
      CommonUtil::set_key(cursor, start_edge.src_id);
      cursor->search_near(cursor, &status);
      if (status < 0)
      {
        // Advances the cursor to the first position in the table.
        if (cursor->next(cursor) != 0)
        {
          this->has_next = false;
        }
        else
        {
          CommonUtil::record_to_adjlist(cursor, &current_adjlist);
          CommonUtil::get_key(cursor, &current_adjlist.node_id);
          if (current_adjlist.node_id != start_edge.src_id)
          {
            this->has_next = false;
          }
          pos = 0;
        }
        //now advance the position to the first edge in range.
        std::cout << "start_edge.dst_id: " << start_edge.dst_id
                  << " current_adjlist size: "
                  << current_adjlist.edgelist.size() 
                  << "pos = " << pos 
                  << " has value: "<< current_adjlist.edgelist[pos] << std::endl;
        while (pos < current_adjlist.edgelist.size() &&
                current_adjlist.edgelist[pos] < start_edge.dst_id)
        {
          pos++;
        }
        std::cout << "pos: " << pos
                  << " current_adjlist size: "
                  << current_adjlist.edgelist.size() << std::endl;
      }
      #endif
    }
    else
    {
      std::cout <<"HEREHERE: " << start_edge.src_id << " "
                << start_edge.dst_id << std::endl;
      // Advances the cursor to the first position in the table.
      if (cursor->next(cursor) != 0)
      {
        this->has_next = false;
      }
      else
      {
        #ifndef MK_NEDGES
        CommonUtil::record_to_adjlist(cursor, &current_adjlist);
        CommonUtil::get_key(cursor, &current_adjlist.node_id);
        pos = 0;
        #endif
      }
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
    #ifdef  MK_NEDGES
    CommonUtil::get_key(cursor, &found->src_id, &found->dst_id);

    // If end_edge is set
    if (end_edge.src_id != OutOfBand_ID_MAX)
    {
      // If found.src > end_edge.src or the edge is such that the source
      // is less than end.src but the destination is greater than end.dst
      if ((found->src_id > end_edge.src_id) ||
          ((found->src_id == end_edge.src_id) &&
           (found->dst_id > end_edge.dst_id)))
        no_next(found);
    }
    if (get_weight)
    {
      CommonUtil::record_to_edge(cursor, found);
    }
    if (cursor->next(cursor) != 0)
    {
      has_next = false;
    }
    #else
    if (pos >= current_adjlist.edgelist.size())
    {
      if (cursor->next(cursor) != 0)
      {
        no_next(found);
        return;
      }
      CommonUtil::record_to_adjlist(cursor, &current_adjlist);
      CommonUtil::get_key(cursor, &current_adjlist.node_id);
      pos = 0;
    }
    found->src_id = current_adjlist.node_id;
    found->dst_id = current_adjlist.edgelist[pos++];
    if (end_edge.src_id != OutOfBand_ID_MAX)
    {
      // If found.src > end_edge.src or the edge is such that the source
      // is less than end.src but the destination is greater than end.dst
      if ((found->src_id > end_edge.src_id) ||
          ((found->src_id == end_edge.src_id) &&
           (found->dst_id > end_edge.dst_id)))
      {
        no_next(found);
        return;
      }
    }
    found->edge_weight = 0;  // No edge weight in adjlist
    #endif
  }
};
#endif  // SRC_ADJLIST_EDGECURSOR_H