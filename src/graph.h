#ifndef GRAPH_H
#define GRAPH_H
#include <wiredtiger.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common_util.h"
#include "graph_exception.h"

struct prop_blob {
    const uint8_t* data = nullptr;
    size_t size = 0;
};

class GraphBase
{
 public:
  GraphBase() = default;
  GraphBase(graph_opts &opt_params, WT_CONNECTION *conn);

  // GraphBase holds raw WT_SESSION and WT_CURSOR pointers that must not be
  // shared between instances.  Disable copy and move to prevent silent bugs.
  GraphBase(const GraphBase &) = delete;
  GraphBase &operator=(const GraphBase &) = delete;
  GraphBase(GraphBase &&) = delete;
  GraphBase &operator=(GraphBase &&) = delete;

  bool is_read_optimized() const { return opts.read_optimize; }

  static void insert_metadata(int key,
                              const char *value,
                              size_t size,
                              WT_CURSOR *cursor);
  void get_metadata(int key, WT_ITEM &item, WT_CURSOR *metadata_cursor);
  void dump_meta_data();
  virtual node get_node(node_id_t node_id) = 0;
  virtual node get_random_node() = 0;
  virtual void get_random_node_ids(std::vector<node_id_t> &randoms,
                                   int count) = 0;
  static void create_metadata_table(
      graph_opts &opts,
      WT_CONNECTION *conn);  // Used during first-time init of DB
  virtual int add_node(node to_insert, bool is_bulk) = 0;
  virtual bool has_node(node_id_t node_id) = 0;
  virtual int delete_node(node_id_t node_id) = 0;
  virtual int add_edge(edge to_insert, bool is_bulk) = 0;
  virtual int delete_edge(node_id_t src_id, node_id_t dst_id) = 0;
  virtual edge get_edge(node_id_t src_id, node_id_t dst_id) = 0;

  /**
   * @brief This function is used for graphalytics workloads. for each edge,
   * read its current weight. If the current weight exist, add e's weight with
   * current weight and update the edge weight otherwise insert e as the new
   * edge.
   *
   * @param to_update weighted edge to insert/update.
   * @return true if the operation succeeds
   */
  virtual bool update_edge(edge to_update) = 0;
  virtual std::vector<node> get_nodes() = 0;
  virtual std::vector<edge> get_edges() = 0;
  virtual bool has_edge(node_id_t src_id, node_id_t dst_id) = 0;

  virtual degree_t get_out_degree(node_id_t node_id) = 0;
  virtual degree_t get_in_degree(node_id_t node_id) = 0;
  virtual std::vector<edge> get_out_edges(node_id_t node_id) = 0;
  virtual std::vector<node> get_out_nodes(node_id_t node_id) = 0;

  virtual std::vector<node_id_t> get_out_nodes_id(node_id_t node_id) = 0;
  virtual std::vector<node_id_t> get_in_nodes_id(node_id_t node_id) = 0;

  virtual std::vector<edge> get_in_edges(node_id_t node_id) = 0;
  virtual std::vector<node> get_in_nodes(node_id_t node_id) = 0;

  virtual OutCursor *get_outnbd_iter() = 0;
  virtual InCursor *get_innbd_iter() = 0;
  virtual NodeCursor *get_node_iter() = 0;
  virtual EdgeCursor *get_edge_iter() = 0;

  // void set_locks(LockSet *locks_ptr);

  void close(bool synchronize = false);
  virtual node_id_t get_max_node_id() = 0;
  virtual node_id_t get_min_node_id() = 0;

  node_id_t get_num_nodes() const;
  edge_id_t get_num_edges() const;
  static void increment_nodes(int increment);
  static void increment_edges(int increment);

  // Public accessors for checkpoint operations
  static node_id_t get_atomic_nnodes()
  {
    return local_nnodes.load(std::memory_order_acquire);
  }
  static edge_id_t get_atomic_nedges()
  {
    return local_nedges.load(std::memory_order_acquire);
  }

  [[nodiscard]] std::string get_db_name() const { return opts.db_name; };
  static int _get_table_cursor(const std::string &table,
                               WT_CURSOR **cursor,
                               WT_SESSION *session,
                               bool is_random,
                               bool overwrite_allowed,
                               const std::string &checkpoint_name = "");
  virtual void dump_table(const std::string &table_name, int limit) = 0;
  virtual void set_ro_num_nodes(node_id_t num) = 0;

  // Property storage API
  virtual void set_node_properties(node_id_t id, const uint8_t* data, size_t size) = 0;
  virtual prop_blob get_node_properties(node_id_t id) = 0;
  virtual void set_edge_properties(node_id_t src, node_id_t dst, const uint8_t* data, size_t size) = 0;
  virtual prop_blob get_edge_properties(node_id_t src, node_id_t dst) = 0;

  // Columnar scan API: open a cursor on a specific column group for bulk reads.
  // Caller owns the cursor and must close it after use.
  // Only valid when prop_mode == COLUMNAR; throws otherwise.
  virtual WT_CURSOR* open_colgroup_cursor(const std::string& table,
                                           const std::string& colgroup) = 0;

  // Secondary multi-valued tables (COLUMNAR mode only; no-ops otherwise).
  // idx is the insertion-order index for the value (0, 1, 2, …).
  virtual void add_person_email(node_id_t person_id, uint64_t idx, const char* email) {}
  virtual void add_person_language(node_id_t person_id, uint64_t idx, const char* lang) {}
  virtual std::vector<std::string> get_person_emails(node_id_t person_id) { return {}; }
  virtual std::vector<std::string> get_person_languages(node_id_t person_id) { return {}; }

 protected:
  graph_opts opts;
  WT_CONNECTION *connection = nullptr;
  WT_SESSION *session = nullptr;
  WT_CURSOR *metadata_cursor = nullptr;

  static std::atomic<node_id_t> local_nnodes;
  static std::atomic<edge_id_t> local_nedges;

  [[maybe_unused]] WT_CONNECTION *get_db_conn() { return this->connection; }
  [[maybe_unused]] WT_SESSION *get_db_session() { return this->session; }

  int _get_index_cursor(const std::string &table_name,
                        const std::string &idx_name,
                        const std::string &projection,
                        const std::string &checkpoint_name,
                        WT_CURSOR **cursor) const;
  [[maybe_unused]] void _restore_from_db();
  [[maybe_unused]] void sync_metadata();
  virtual void close_all_cursors() = 0;

};

#endif