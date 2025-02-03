#ifndef ADJ_LIST
#define ADJ_LIST

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common_util.h"
#include "graph.h"
#include "graph_exception.h"

using namespace std;

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
    } while (found->degree == 0 && all_nodes == false);
  }

  void next(adjlist *found, node_id_t key) override {}
};

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

class AdjNodeCursor : public NodeCursor
{
 public:
  AdjNodeCursor(WT_CURSOR *cur, WT_SESSION *sess)
  {
    cursor = cur;
    session = sess;
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

    CommonUtil::record_to_node(cursor, found, read_opt, directed);
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
    cursor->search_near(cursor, &status);
    if (status < 0)
    {
      if (cursor->next(cursor) != 0)
      {
        has_next = false;
        no_next(found);
      }
    }
    node_id_t curr_key;
    CommonUtil::get_key(cursor, &curr_key);

    if (curr_key == key)
    {
      CommonUtil::record_to_node(cursor, found, read_opt, directed);
    }

    // no relevant node was found.
    if (keys.end != OutOfBand_ID_MAX && curr_key > keys.end)
    {
      has_next = false;
    }
  }
};

class AdjEdgeCursor : public EdgeCursor
{
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
    start_edge = range.start;
    end_edge = range.end;
    is_first = false;

    // Advances the cursor to the first valid record in range
    if (start_edge.src_id != OutOfBand_ID_MAX &&
        start_edge.dst_id != OutOfBand_ID_MAX)
    {
      int status;
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
  }
};

class AdjList : public GraphBase
{
 public:
  AdjList() = default;
  AdjList(graph_opts &opt_params,
          WT_CONNECTION *connection);  // TODO: merge the 2 constructors
  static void create_wt_tables(
      graph_opts &opts, WT_CONNECTION *conn);  // Need this to init graph db
  int add_node(node to_insert, bool is_bulk = false) override;
  //    int add_node(node to_insert) override;
  [[maybe_unused]] void add_node(node_id_t to_insert,
                                 std::vector<node_id_t> &inlist,
                std::vector<node_id_t> &outlist);
  bool has_node(node_id_t node_id) override;
  node get_node(node_id_t node_id) override;
  int delete_node(node_id_t to_delete) override;
  node get_random_node() override;
  void get_random_node_ids(std::vector<node_id_t> &nodes, int count) override;
  degree_t get_in_degree(node_id_t node_id) override;
  degree_t get_out_degree(node_id_t node_id) override;
  std::vector<node> get_nodes() override;

  int add_edge(edge to_insert, bool is_bulk) override;
  //    int add_edge(edge to_insert) override;
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
  std::vector<node_id_t> get_adjlist(WT_CURSOR *cursor, node_id_t node_id);

  node_id_t get_max_node_id() override;
  node_id_t get_min_node_id() override;

  OutCursor *get_outnbd_iter() override;
  InCursor *get_innbd_iter() override;
  NodeCursor *get_node_iter() override;
  EdgeCursor *get_edge_iter() override;
  // edgeweight_t get_edge_weight(node_id_t src_id, node_id_t dst_id);
  [[maybe_unused]] void update_edge_weight(
      node_id_t src_id,
      node_id_t dst_id,
      edgeweight_t edge_weight);  // todo <-- is this implemented?

  // internal cursor operations:
  //! Check if these should be public:
  void init_cursors();
  WT_CURSOR *get_node_cursor();
  WT_CURSOR *get_edge_cursor();
  WT_CURSOR *get_in_adjlist_cursor();
  WT_CURSOR *get_out_adjlist_cursor();

  WT_CURSOR *get_new_node_cursor();
  WT_CURSOR *get_new_edge_cursor();
  WT_CURSOR *get_new_in_adjlist_cursor();
  WT_CURSOR *get_new_out_adjlist_cursor();
  WT_CURSOR *get_new_random_outadj_cursor();
  // making this public because needed for graferee
  void add_adjlist(WT_CURSOR *cursor,
                   node_id_t node_id,
                   std::vector<node_id_t> &list);
  [[maybe_unused]] void dump_table(std::string &table_name, int limit = 0);

 private:
  // structure of the graph

  WT_CURSOR *node_cursor = nullptr;
  WT_CURSOR *random_node_cursor = nullptr;
  WT_CURSOR *edge_cursor = nullptr;
  WT_CURSOR *in_adjlist_cursor = nullptr;
  WT_CURSOR *out_adjlist_cursor = nullptr;

  // AdjList specific internal methods:
  [[maybe_unused]] node get_next_node(WT_CURSOR *n_cur);
  [[maybe_unused]] edge get_next_edge(WT_CURSOR *e_cur);
  int add_adjlist(WT_CURSOR *cursor, node_id_t node_id);

  int delete_adjlist(WT_CURSOR *cursor, node_id_t node_id);
  [[maybe_unused]] void delete_node_from_adjlists(node_id_t node_id);
  int add_to_adjlists(WT_CURSOR *cursor,
                      node_id_t node_id,
                      node_id_t to_insert);
  int delete_from_adjlists(WT_CURSOR *cursor,
                           node_id_t node_id,
                           node_id_t to_delete);
  int delete_related_edges_and_adjlists(node_id_t to_delete,
                                        int *num_edges_deleted);
  int update_node_degree(WT_CURSOR *cursor,
                         node_id_t node_id,
                         int32_t indeg_change,
                         int32_t outdeg_change);

  int add_node_in_txn(node to_insert, bool ignore_duplicate_key);
  int delete_edge_in_txn(node_id_t src_id,
                         node_id_t dst_id,
                         WT_CURSOR *nbd2prune);

  int error_check_insert_txn(int return_val, bool ignore_duplicate_key);
  int error_check_read_txn(int return_val);
  int error_check_remove_txn(int return_val);
  inline void close_all_cursors() override
  {
    CommonUtil::close_cursor(node_cursor);
    CommonUtil::close_cursor(edge_cursor);
    CommonUtil::close_cursor(in_adjlist_cursor);
    CommonUtil::close_cursor(out_adjlist_cursor);
  }
};

#endif