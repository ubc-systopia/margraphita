#ifndef ADJ_LIST
#define ADJ_LIST

#include <wiredtiger.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "adjlist_edgecursor.h"
#include "adjlist_innbdcursor.h"
#include "adjlist_nodecursor.h"
#include "adjlist_outnbdcursor.h"
#include "common_util.h"
#include "graph.h"
#include "graph_exception.h"

// forward declarations for the cursor classes
class AdjNodeCursor;
class AdjOutCursor;
class AdjInCursor;
class AdjEdgeCursor;

using namespace std;

class AdjList : public GraphBase
{
 public:
  AdjList() = default;
  AdjList(graph_opts &opt_params,
          WT_CONNECTION *connection);  // TODO: merge the 2 constructors
  static void create_wt_tables(
      graph_opts &opts, WT_CONNECTION *conn);  // Need this to init graph db
  int add_node(node to_insert, bool is_bulk = false) override;
  [[maybe_unused]] int add_node(node_id_t to_insert,
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

  // internal cursor operations:
  //! Check if these should be public:
  void init_cursors();
  WT_CURSOR *get_node_cursor();
  WT_CURSOR *get_edge_cursor();
  WT_CURSOR *get_in_adjlist_cursor();
  WT_CURSOR *get_out_adjlist_cursor();

  // WT_CURSOR *get_new_node_cursor();
  // WT_CURSOR *get_new_edge_cursor();
  // WT_CURSOR *get_new_in_adjlist_cursor();
  // WT_CURSOR *get_new_out_adjlist_cursor();
  WT_CURSOR *get_new_random_outadj_cursor();
  // making this public because needed for graferee
  int add_adjlist(WT_CURSOR *cursor,
                  node_id_t node_id,
                  std::vector<node_id_t> &list);
  [[maybe_unused]] void dump_table(const std::string &table_name, int num_records = 0);

  bool update_edge(edge to_update) override;
  void set_ro_num_nodes(node_id_t num) override
  {
    if(opts.read_only == false)
    {
      throw GraphException("set_ro_num_nodes can only be called on a read-only graph");
    }
    opts.num_nodes = num;
  };

  void set_node_properties(node_id_t id, const uint8_t* data, size_t size) override;
  prop_blob get_node_properties(node_id_t id) override;
  void set_edge_properties(node_id_t src, node_id_t dst, const uint8_t* data, size_t size) override;
  prop_blob get_edge_properties(node_id_t src, node_id_t dst) override;

  static constexpr char const *NODE_PROPS_TABLE = "node_props";
  static constexpr char const *EDGE_PROPS_TABLE = "edge_props";

 private:
  friend class AdjNodeCursor;
  friend class AdjOutCursor;
  friend class AdjInCursor;
  friend class AdjEdgeCursor;
  // structure of the graph
  WT_CURSOR *node_cursor = nullptr;
  WT_CURSOR *random_node_cursor = nullptr;
  WT_CURSOR *edge_cursor = nullptr;
  WT_CURSOR *in_adjlist_cursor = nullptr;
  WT_CURSOR *out_adjlist_cursor = nullptr;
  WT_CURSOR *node_props_cursor = nullptr;
  WT_CURSOR *edge_props_cursor = nullptr;

  // AdjList specific internal methods:
  [[maybe_unused]] node get_next_node(WT_CURSOR *n_cur);
  [[maybe_unused]] edge get_next_edge(WT_CURSOR *e_cur);
  int add_adjlist(WT_CURSOR *cursor, node_id_t node_id);

  int delete_adjlist(WT_CURSOR *cursor,
                     node_id_t node_id,
                     degree_t *num_edges_deleted);
  [[maybe_unused]] void delete_node_from_adjlists(node_id_t node_id);
  int add_to_adjlists(WT_CURSOR *cursor,
                      node_id_t node_id,
                      node_id_t to_insert,
                      bool &node_exits);
  int delete_from_adjlists(WT_CURSOR *cursor,
                           node_id_t node_id,
                           node_id_t to_delete);
  int delete_related_edges_and_adjlists(node_id_t to_delete,
                                        degree_t *num_edges_deleted);
  int update_node_degree(WT_CURSOR *cursor,
                         node_id_t node_id,
                         int32_t indeg_change,
                         int32_t outdeg_change);

  int add_node_in_txn(node to_insert);
  int delete_edge_in_txn(node_id_t src_id,
                         node_id_t dst_id,
                         WT_CURSOR *nbd2prune);

  int error_check_insert_txn(int return_val);
  int error_check_read_txn(int return_val);
  inline void close_all_cursors() override
  {
    CommonUtil::close_cursor(node_cursor);
    CommonUtil::close_cursor(edge_cursor);
    CommonUtil::close_cursor(in_adjlist_cursor);
    CommonUtil::close_cursor(out_adjlist_cursor);
    if (node_props_cursor) CommonUtil::close_cursor(node_props_cursor);
    if (edge_props_cursor) CommonUtil::close_cursor(edge_props_cursor);
  }

  inline void get_edge_wt(WT_CURSOR *e_cur, edgeweight_t *edge_weight)
  {
    WT_ITEM item;
    e_cur->get_value(e_cur, &item);
    *edge_weight = *(edgeweight_t *)item.data;
  }

  inline void set_edge_wt(WT_CURSOR *e_cur, edgeweight_t edge_weight)
  {
    WT_ITEM item;
    item.data = reinterpret_cast<const unsigned*>(&edge_weight);
    item.size = sizeof(edgeweight_t);
    e_cur->set_value(e_cur, &item);
  }
};

#endif