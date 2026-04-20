#ifndef EDGELIST_H
#define EDGELIST_H

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "common_util.h"
#include "edgekey_split.h"
#include "graph.h"
#include "graph_exception.h"

class SplitEdgeKey : public GraphBase
{
 public:
  SplitEdgeKey(graph_opts &opt_params,
               WT_CONNECTION *connection);  // TODO: merge the 2 constructors
  static void create_wt_tables(graph_opts &opts, WT_CONNECTION *conn);
  int add_node(node to_insert, bool is_bulk = false) override;

  bool has_node(node_id_t node_id) override;
  node get_node(node_id_t node_id) override;
  int delete_node(node_id_t node_id) override;
  node get_random_node() override;
  void get_random_node_ids(std::vector<node_id_t> &ids, int num_nodes) override;
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

  bool update_edge(edge to_update) override;

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
  [[nodiscard]] WT_CURSOR *get_in_edge_cursor() const { return in_edge_cursor; }
  WT_CURSOR *get_new_in_cursor();
  static void create_indices(WT_SESSION *session);
  void dump_table(const std::string &table_name, int num_records);
  void set_ro_num_nodes(node_id_t num) override
  {
    if(opts.read_only == false)
    {
      throw GraphException("set_ro_num_nodes can only be called on a read-only graph");
    }
    opts.num_nodes = num;
  };

 private:
  WT_CURSOR *out_edge_cursor = nullptr;
  WT_CURSOR *random_node_cursor = nullptr;
  WT_CURSOR *in_edge_cursor = nullptr;
  // WT_CURSOR *dst_src_idx_cursor = nullptr; // Removed dependency

  [[maybe_unused]] WT_CURSOR *get_metadata_cursor();
  int delete_node_and_related_edges(node_id_t node_id, int *num_edges_to_del);
  int update_node_degree(node_id_t node_id, int in_change, int out_change);
  int add_node_txn(node to_insert,
                   int *num_nodes_added,
                   int32_t indeg_change,
                   int32_t outdeg_change);
  int error_check_insert_txn(int return_val);
  int error_check_read_txn(int return_val);

  [[maybe_unused]] inline void close_all_cursors() override
  {
    out_edge_cursor->close(out_edge_cursor);
    random_node_cursor->close(random_node_cursor);
    in_edge_cursor->close(in_edge_cursor);
    // dst_src_idx_cursor->close(dst_src_idx_cursor); // Removed dependency
  }

 public:
  static void ekey_set_edge_value(WT_CURSOR *cursor, edgeweight_t weight);
  static void ekey_get_edge_value(WT_CURSOR *cursor, edgeweight_t *weight);
  static void ekey_set_node_value(WT_CURSOR *cursor,
                                  degree_t in_degree,
                                  degree_t out_degree);
  static void ekey_get_node_value(WT_CURSOR *cursor,
                                  degree_t *in_degree,
                                  degree_t *out_degree);
};

// internal methods
template <typename T, typename... Args>
static inline int pack_values(WT_ITEM *item,
                              const T &first,
                              const Args &...args)
{
  constexpr size_t count = 1 + sizeof...(Args);
  // Use thread-local storage: avoids heap allocation and is safe because
  // the cursor insert/update always completes before the next pack_values call
  // on the same thread.  Max 4 elements covers all current uses.
  static_assert(count <= 4, "pack_values: too many arguments");
  thread_local T buffer[4];

  buffer[0] = first;
  size_t idx = 1;
  ((buffer[idx++] = args), ...);

  item->data = reinterpret_cast<const unsigned *>(buffer);
  item->size = sizeof(T) * count;
  return 0;
}

template <typename T, typename... Args>
static inline int unpack_values(const WT_ITEM *item, T *first, Args... args)
{
  constexpr size_t count = 1 + sizeof...(Args);
  if (item->size != sizeof(T) * count)
  {
    return -1;
  }
  size_t offset = 0;
  auto unpack = [&](auto *value)
  {
    memcpy(
        value, reinterpret_cast<const char *>(item->data) + offset, sizeof(T));
    offset += sizeof(T);
  };
  unpack(first);
  (unpack(args), ...);
  return 0;
}

inline void SplitEdgeKey::ekey_set_edge_value(WT_CURSOR *cursor,
                                              edgeweight_t weight)
{
  WT_ITEM item;
  pack_values(&item, weight);
  cursor->set_value(cursor, &item);
}
inline void SplitEdgeKey::ekey_set_node_value(WT_CURSOR *cursor,
                                              degree_t in_deg,
                                              degree_t out_deg)
{
  WT_ITEM item;
  pack_values(&item, in_deg, out_deg);
  cursor->set_value(cursor, &item);
}

inline void SplitEdgeKey::ekey_get_node_value(WT_CURSOR *cursor,
                                              degree_t *in_deg,
                                              degree_t *out_deg)
{
  WT_ITEM item;
  cursor->get_value(cursor, &item);
  if (item.size != sizeof(degree_t) * 2)
  {
    throw GraphException("Node degree size mismatch");
  }
  unpack_values(&item, in_deg, out_deg);
}

inline void SplitEdgeKey::ekey_get_edge_value(WT_CURSOR *cursor,
                                              edgeweight_t *weight)
{
  WT_ITEM item;
  cursor->get_value(cursor, &item);
  if (item.size != sizeof(edgeweight_t))
  {
    throw GraphException("Edge weight size mismatch");
  }
  unpack_values(&item, weight);
}

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
  SplitEkeyInCursor(WT_CURSOR *cur,
                    WT_SESSION *sess,
                    bool is_directed,
                    bool read_optimized)
  {
    cursor = cur;
    session = sess;
    directed = is_directed;
    read_opt = read_optimized;
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

    CommonUtil::ekey_set_node_key(cursor, keys.start);
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
    //    node_id_t temp_dst;
    //    CommonUtil::ekey_get_key(cursor, &curr_node, &temp_dst);
  }

  void next(adjlist *found) override
  {
    node_id_t src, dst;
    node_id_t curr_src;
    if (!has_next)
    {
      found->node_id = OutOfBand_ID_MAX;
      found->degree = UINT32_MAX;
      found->edgelist.clear();
      return;
    }

    // get edge
    CommonUtil::ekey_get_key(cursor, &dst, &src);
    if (directed)
    {
      curr_node = dst;
      curr_src = src;
    }
    else
    {
      if (src == OutOfBand_ID_MIN) curr_node = dst;
    }
    while (cursor->next(cursor) == 0)
    {
      CommonUtil::ekey_get_key(cursor, &dst, &src);
      found->node_id = curr_node;
      if (dst == curr_node && src != OutOfBand_ID_MIN)
      {
        found->edgelist.push_back(src);
        found->degree++;
      }
      else if (dst != curr_node && directed)
      {
        found->node_id = curr_node;
        found->edgelist.push_back(curr_src);
        found->degree++;
        return;
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

    CommonUtil::ekey_set_node_key(cursor, keys.start);
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
  // Takes a main edge table cursor, not an index cursor
  SplitEKeyNodeCursor(WT_CURSOR *cur, WT_SESSION *sess)
  {
    cursor = cur;
    session = sess;
    set_key_range({OutOfBand_ID_MIN, OutOfBand_ID_MAX});  // min and max node id
  }
  ~SplitEKeyNodeCursor() override = default;

  void set_key_range(key_range _keys) override
  {
    keys = _keys;
    int status;
    // Use main edge table with (src, OutOfBand_ID_MIN) pattern
    if (keys.start != OutOfBand_ID_MIN)
    {
      CommonUtil::ekey_set_node_key(cursor, keys.start);
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
      // Start from beginning with (OutOfBand_ID_MIN, OutOfBand_ID_MIN)
      CommonUtil::ekey_set_key(cursor, OutOfBand_ID_MIN, OutOfBand_ID_MIN);
      cursor->search_near(cursor, &status);
      if (status < 0)
      {
        if (cursor->next(cursor) != 0)
        {
          this->has_next = false;
        }
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
    node_id_t src, dst;
    if (!has_next)
    {
      no_next(found);
      return;
    }

    // Now using main edge table cursor (src, dst) format
    CommonUtil::ekey_get_key(cursor, &src, &dst);

    if (keys.end != OutOfBand_ID_MIN && src > keys.end)
    {
      no_next(found);
      return;
    }

    if (dst != OutOfBand_ID_MIN)
    {
      no_next(found);
      return;
    }

    // Found a valid node entry
    found->id = src;
    // cursor->get_value(cursor, &found->in_degree, &found->out_degree);
    SplitEdgeKey::ekey_get_node_value(
        cursor, &found->in_degree, &found->out_degree);

    // Advance to next node using search_near
    CommonUtil::ekey_set_key(cursor, src + 1, OutOfBand_ID_MIN);
    int search_exact;
    int ret = cursor->search_near(cursor, &search_exact);
    if (ret != 0)
    {
      has_next = false;
      return;
    }
    if (search_exact < 0)
    {
      if (cursor->next(cursor) != 0)
      {
        has_next = false;
        return;
      }
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
      CommonUtil::ekey_set_key(cursor, range.start.src_id, range.start.dst_id);
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
      // CommonUtil::record_to_edge_ekey(cursor, found);
      SplitEdgeKey::ekey_get_edge_value(cursor, &found->edge_weight);
    }

    if (cursor->next(cursor) != 0)
    {
      has_next = false;
      return;
    }
  }
};

#endif