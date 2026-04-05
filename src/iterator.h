#ifndef ITERATOR_H
#define ITERATOR_H
#include <wiredtiger.h>

#include <iostream>
#include <memory>

#include "common_defs.h"
#include "graph_exception.h"
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
  bool directed = false;
  bool read_opt = true;

 public:
  table_iterator() = default;
  virtual ~table_iterator() = default;
  [[nodiscard]] bool has_more() const { return has_next; };
  virtual void reset()
  {
    int ret = cursor->reset(cursor);
    if (ret != 0)
    {
      throw GraphException(
          "Error in resetting cursor: " + std::string(wiredtiger_strerror(ret)));
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
  ~OutCursor() override = default;
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
  ~InCursor() override = default;
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
  ~NodeCursor() override = default;

  /**
   * @brief Set the key range object
   *
   * @param _keys the key range object. Set the end key to INT_MAX if you
   * want to get all the nodes from start node.
   */
  virtual void set_key_range(key_range _keys) = 0;  // overrided by edgekey
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
  ~EdgeCursor() override = default;
  virtual void set_key_range(edge_range range) = 0;

  virtual void next(edge *found) = 0;
  void dump_range() const
  {
    std::cout << "start: " << start_edge.src_id << " " << start_edge.dst_id
              << " end: " << end_edge.src_id << " " << end_edge.dst_id
              << std::endl;
  }
};

// ─── NodePropCursor ───────────────────────────────────────────────────────────
// Abstracts over a property scan keyed by node_id_t.
// COLUMNAR: wraps a WiredTiger colgroup cursor.
// EMBEDDED: wraps get_node_properties() point lookups + NodeCursor iteration.
// Returned as unique_ptr — destructor releases all resources (RAII).
// No public close() method.
class NodePropCursor {
public:
  // Restrict sequential scan to [start, end).
  // Passing OutOfBand_ID_MAX for end = unbounded full-table scan.
  virtual void set_range(node_id_t start, node_id_t end) = 0;

  // Advance to the next row.  Returns false at EOF or past range end.
  // Must be called before accessing key()/get_uint64()/get_int32().
  virtual bool next() = 0;

  // Exact point lookup.  Returns false if not found.
  // Mutually exclusive with next()/set_range() per cursor lifetime.
  virtual bool seek(node_id_t id) = 0;

  virtual node_id_t key()             const = 0;
  virtual uint64_t  get_uint64(int n) const = 0;  // nth value column, 0-indexed
  virtual int32_t   get_int32 (int n) const = 0;

  virtual ~NodePropCursor() = default;
};

// ─── EdgePropCursor ───────────────────────────────────────────────────────────
// Abstracts over a property scan keyed by (src, dst).
// COLUMNAR: wraps a WiredTiger colgroup cursor.
// EMBEDDED: wraps get_out_nodes_id() + get_edge_properties() per neighbor.
// Same RAII contract as NodePropCursor.
class EdgePropCursor {
public:
  // Pin to a single src.  next() stops when src changes.
  // EMBEDDED: pre-fetches get_out_nodes_id(src) on this call.
  virtual void set_src(node_id_t src) = 0;

  // Restrict to edges with src in [src_start, src_end).
  // COLUMNAR only; EMBEDDED implementation throws GraphException.
  virtual void set_src_range(node_id_t src_start, node_id_t src_end) = 0;

  // Advance.  Returns false at EOF or when pin/range condition breaks.
  virtual bool next() = 0;

  // Exact point lookup on (src, dst).  Returns false if not found.
  virtual bool seek(node_id_t src, node_id_t dst) = 0;

  virtual node_id_t src()             const = 0;
  virtual node_id_t dst()             const = 0;
  virtual uint64_t  get_uint64(int n) const = 0;

  virtual ~EdgePropCursor() = default;
};

#endif