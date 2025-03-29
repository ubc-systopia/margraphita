#ifndef BASE_COMMON
#define BASE_COMMON

#include <wiredtiger.h>

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

#include "fmt/core.h"
// #ifdef LINUX
// #include <source_location>
// #define SRC_LOC std::source_location::current()
// #endif
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common_defs.h"
#include "graph_exception.h"
#include "iterator.h"

#ifdef DEBUG
#define DEBUG_MSG(msg)                                        \
  std::cerr << "DEBUG: " << msg << " (Function: " << __func__ \
            << ", Line: " << __LINE__ << ")" << std::endl;
#define LOG_MSG(msg, ...)                                                      \
  do                                                                           \
  {                                                                            \
    fmt::print("DEBUG: [{}:{}] " msg "\n", __func__, __LINE__, ##__VA_ARGS__); \
  } while (0)
#else
#define DEBUG_MSG(msg) \
  do                   \
  {                    \
  } while (0)
#define LOG_MSG(msg, ...) \
  do                      \
  {                       \
  } while (0)
#endif

class CommonUtil
{
 public:
  static void create_dir(const std::string &path);

  static bool check_dir_exists(const std::string &path);

  static void set_table(WT_SESSION *session,
                        const std::string &prefix,
                        std::vector<std::string> columns,
                        const std::string &key_fmt,
                        const std::string &val_fmt);

  static std::string get_db_name(const std::string &prefix,
                                 const std::string &name);

  static void check_graph_params(const graph_opts &params);

  // WT Session and Cursor wrangling operations
  [[maybe_unused]] static int open_cursor(WT_SESSION *session,
                                          WT_CURSOR **cursor,
                                          const std::string &uri,
                                          WT_CURSOR *to_dup,
                                          const std::string &config);
  static int close_cursor(WT_CURSOR *cursor);
  static int close_connection(WT_CONNECTION *conn);
  static int open_connection(const char *db_name,
                             const std::string &logdir,
                             const std::string &config,
                             WT_CONNECTION **conn);
  static int dup_cursor(WT_SESSION *session,
                        WT_CURSOR *to_dup,
                        WT_CURSOR **cursor);
  static void set_key(WT_CURSOR *cursor, node_id_t key);
  static void set_key(WT_CURSOR *cursor, node_id_t key1, node_id_t key2);

  static int get_key(WT_CURSOR *cursor, node_id_t *key);
  static int get_key(WT_CURSOR *cursor, node_id_t *key1, node_id_t *key2);

  static int open_session(WT_CONNECTION *conn, WT_SESSION **session);
  static void check_return(int retval, const std::string &mesg);
  static void dump_node(node to_print, std::ostream &os);
  static void dump_edge(edge to_print, std::ostream &os);
  static void dump_adjlist(const adjlist &to_print, std::ostream &os);
  [[maybe_unused]] static void dump_edge_index(edge_index to_print,
                                               std::ostream &os);

  static int node_to_record(WT_CURSOR *cursor,
                            node to_insert,
                            bool read_optimize);
  static void record_to_edge(WT_CURSOR *cursor, edge *found);
  static void read_from_edge_idx(WT_CURSOR *idx_cursor, edge *e_idx);
  static int adjlist_to_record(WT_SESSION *session,
                               WT_CURSOR *cursor,
                               const adjlist &to_insert);
  static void record_to_adjlist(WT_CURSOR *cursor, adjlist *found);

  static void ekey_set_key(WT_CURSOR *cursor, node_id_t key1, node_id_t key2);
  static int ekey_get_key(WT_CURSOR *cursor, node_id_t *key1, node_id_t *key2);
  //  static void record_to_node_ekey_new(WT_CURSOR *cur,
  //                                      node *found,
  //                                      bool directed);
  static void record_to_node_ekey(WT_CURSOR *cur, node *found);
  static void record_to_node_ekeyidx(WT_CURSOR *idx_cursor, node *found);
  static void record_to_edge_ekey(WT_CURSOR *cur, edge *found);
  static void get_val_idx(WT_CURSOR *idx_cursor, node_id_t *a, node_id_t *b);
  static void record_to_node(WT_CURSOR *cursor,
                             node *found,
                             bool read_optimize,
                             bool is_directed);
};

/***************************************************************************
 *            Serialization/ Deserialization methods common to all
 *                             *representations*
 **************************************************************************/

/**
 * @brief This fucntion updates a node with the given attribute vector.
 * This function assumes that only the node attributes are updated here.
 * Node data is modified using get/set_node_data() and degrees are
 * modified automatically on edge insertions/deletions.
 * @param node_id The Node ID to be updated
 * @param new_attrs The new node attribute vector.
 */

inline void CommonUtil::set_key(WT_CURSOR *cursor, node_id_t key)
{
#ifdef B64
  uint64_t a = __builtin_bswap64(key);
#else
  uint32_t a = __builtin_bswap32(key);
#endif
  WT_ITEM k = {.data = &a, .size = sizeof(a)};
  cursor->set_key(cursor, &k);
}

inline void CommonUtil::set_key(WT_CURSOR *cursor,
                                node_id_t key1,
                                node_id_t key2)
{
#ifdef B64
  uint64_t a = __builtin_bswap64(key1);
  uint64_t b = __builtin_bswap64(key2);
#else
  uint32_t a = __builtin_bswap32(key1);
  uint32_t b = __builtin_bswap32(key2);
#endif
  WT_ITEM k1 = {.data = &a, .size = sizeof(a)};
  WT_ITEM k2 = {.data = &b, .size = sizeof(b)};
  cursor->set_key(cursor, &k1, &k2);
}

inline int CommonUtil::get_key(WT_CURSOR *cursor, node_id_t *key)
{
  WT_ITEM k = {0};
  int ret = cursor->get_key(cursor, &k);

#ifdef B64
  uint64_t a = *(uint64_t *)k.data;
  *key = __builtin_bswap64(a);
#else
  uint32_t a = *(uint32_t *)k.data;
  *key = __builtin_bswap32(a);
#endif
  return ret;
}

inline int CommonUtil::get_key(WT_CURSOR *cursor,
                               node_id_t *key1,
                               node_id_t *key2)
{
  WT_ITEM k1, k2;
  int ret = cursor->get_key(cursor, &k1, &k2);

#ifdef B64
  uint64_t a = *(uint64_t *)k1.data;
  uint64_t b = *(uint64_t *)k2.data;
  *key1 = __builtin_bswap64(a);
  *key2 = __builtin_bswap64(b);
#else
  uint32_t a = *(uint32_t *)k1.data;
  uint32_t b = *(uint32_t *)k2.data;
  *key1 = __builtin_bswap32(a);
  *key2 = __builtin_bswap32(b);
#endif
  return ret;
}

inline int CommonUtil::node_to_record(WT_CURSOR *cursor,
                                      node to_insert,
                                      bool read_optimize)
{
  // cursor cannot be null
  int ret;
  CommonUtil::set_key(cursor, to_insert.id);
  if ((ret = cursor->search(cursor)) != 0)
  {
    return ret;
  }
  if (read_optimize)
  {
    cursor->set_value(cursor, to_insert.in_degree, to_insert.out_degree);
  }
  else
  {
    cursor->set_value(cursor, "");
  }
  return cursor->update(cursor);
}

/**
 * @brief This function reads the record currently being pointed to by the
 * cursor, and returns the values in the node struct passed to it.
 * @param cursor the cursor to the node tableq
 * @param node* found to populate from the get_value call
 * @param bool read_optimize whether the node is read optimized or not
 * @param bool is_directed whether the graph is directed or not
 * @return node
 */
inline void CommonUtil::record_to_node(WT_CURSOR *cursor,
                                       node *found,
                                       bool read_optimize,
                                       bool is_directed)
{
  found->in_degree = 0;
  found->out_degree = 0;

  if (read_optimize)
  {
    is_directed
        ? cursor->get_value(cursor, &found->in_degree, &found->out_degree)
        : cursor->get_value(cursor, &found->out_degree);
  }
}

inline void CommonUtil::record_to_edge(WT_CURSOR *cursor, edge *found)
{
  if (cursor->get_value(cursor, &found->edge_weight) != 0)
  {
    throw GraphException("Could not get the value from the edge table");
  }
}

//! todo : fix this
inline void CommonUtil::read_from_edge_idx(WT_CURSOR *idx_cursor, edge *e_idx)
{
  WT_ITEM src, dst;
  idx_cursor->get_value(idx_cursor, &src, &dst, &e_idx->edge_weight);

#ifdef B64
  uint64_t src_id = *(uint64_t *)src.data;
  uint64_t dst_id = *(uint64_t *)dst.data;
  e_idx->src_id = __builtin_bswap64(src_id);
  e_idx->dst_id = __builtin_bswap64(dst_id);
#else
  uint32_t src_id = *(uint32_t *)src.data;
  uint32_t dst_id = *(uint32_t *)dst.data;
  e_idx->src_id = __builtin_bswap32(src_id);
  e_idx->dst_id = __builtin_bswap32(dst_id);
#endif
}

inline void CommonUtil::get_val_idx(WT_CURSOR *idx_cursor,
                                    node_id_t *a,
                                    node_id_t *b)
{
  WT_ITEM x, y;
  idx_cursor->get_value(idx_cursor, &x, &y);
#ifdef B64
  uint64_t src_id = *(uint64_t *)x.data;
  uint64_t dst_id = *(uint64_t *)y.data;
  *a = __builtin_bswap64(src_id);
  *b = __builtin_bswap64(dst_id);
#else
  uint32_t src_id = *(uint32_t *)x.data;
  uint32_t dst_id = *(uint32_t *)y.data;
  *a = __builtin_bswap32(src_id);
  *b = __builtin_bswap32(dst_id);
#endif
}

/**
 * @brief This function accepts a cursor to the adjlist table and an adjlist
 * sturct to insert.
 * @param cursor A cursor to the in/out adjlist table
 * @param to_insert The adjlist struct to be inserted into the table pointed
 * to by the cursor.
 * @throws GraphException If insertion into the table fails
 */
inline int CommonUtil::adjlist_to_record(WT_SESSION *session,
                                         WT_CURSOR *cursor,
                                         const adjlist &to_insert)
{
  cursor->reset(cursor);
  CommonUtil::set_key(cursor, to_insert.node_id);
  int ret = cursor->search(cursor);

  WT_ITEM item;
  item.data = to_insert.edgelist.data();
  item.size = to_insert.edgelist.size() * sizeof(node_id_t);

  cursor->set_value(cursor, to_insert.degree, &item);

  ret = cursor->update(cursor);
  if (ret)
  {
    DEBUG_MSG("Error inserting into adjlist table" + std::to_string(ret) +
              " - " + wiredtiger_strerror(ret));
  }
  cursor->reset(cursor);
  return 0;
}

/**
 * @brief This function converts the record the cursor passed points to into
 * a adjlist struct
 *
 * @param cursor the cursor set to the record which needs to be read
 * @return adjlist the found adjlist struct.
 */
inline void CommonUtil::record_to_adjlist(WT_CURSOR *cursor, adjlist *found)
{
  int32_t degree;
  WT_ITEM item;
  cursor->get_value(cursor, &degree, &item);
  found->edgelist.assign(
      (node_id_t *)item.data,
      (node_id_t *)item.data + item.size / sizeof(node_id_t));
  if (degree == 1 && found->edgelist.empty())
  {
    found->degree = 0;
  }
  else
  {
    found->degree = degree;
  }
}
// Get and Set data from/to an edge table cursor
/**
 * @brief This function converts the record the cursor points to into a node
 * struct. In this representation, we know that the Value can be :
 * 1. indeg, out_deg for opts.read_optimize node entry
 * 2. "" for non opts.read_optimize node entry
 * 3. edge_weight for a weighted edge entry
 * 4. "" for an unweighted graph.
 *
 * Seeing this, it is wasteful to have a serialization/deserialization call.
 * Save -1 for any of these entries that don't exist.
 * @param cur the pointer -- already pointing to the record to extract
 * @param found the node to insert the in and outdegrees for.
 */
inline void CommonUtil::record_to_node_ekey(WT_CURSOR *cur, node *found)
{
  // std::cout << cur->value_format << std::endl;
  //! checked that it works for negative int32_t values.
  int a = 0, b = 0;
  int ret = cur->get_value(cur, &a, &b);
  if (ret != 0)
  {
    throw GraphException("Failed to get node attributes");
  }
  found->in_degree = a;
  found->out_degree = b;
}

// inline void CommonUtil::record_to_node_ekey_new(WT_CURSOR *cur,
//                                                 node *found,
//                                                 bool directed)
//{
//   // std::cout << cur->value_format << std::endl;
//   //! checked that it works for negative int32_t values.
//   degree_t a = 0, b = 0;
//   int ret = cur->get_value(cur, &a, &b);
//   if (ret != 0)
//   {
//     throw GraphException("Failed to get node attributes");
//   }
//   if (directed)
//   {
//     found->in_degree = a;
//     found->out_degree = b;
//   }
//   else
//   {
//     found->out_degree = b;
//     found->in_degree = 0;
//   }
// }

inline void CommonUtil::record_to_edge_ekey(WT_CURSOR *cur, edge *found)
{
  int a = 0, b = 0;
  int ret = cur->get_value(cur, &a, &b);
  if (ret != 0)
  {
    throw GraphException("Failed to get edge val");
  }
  found->edge_weight = a;
}

inline void CommonUtil::ekey_set_key(WT_CURSOR *cursor,
                                     node_id_t key1,
                                     node_id_t key2)
{
  if (key1 != OutOfBand_ID_MIN) key1 = MAKE_EKEY(key1);
  if (key2 != OutOfBand_ID_MIN) key2 = MAKE_EKEY(key2);
  CommonUtil::set_key(cursor, key1, key2);
}

inline int CommonUtil::ekey_get_key(WT_CURSOR *cursor,
                                    node_id_t *key1,
                                    node_id_t *key2)
{
  int ret = CommonUtil::get_key(cursor, key1, key2);
  if (*key1 != OutOfBand_ID_MIN) *key1 = OG_KEY(*key1);
  if (*key2 != OutOfBand_ID_MIN) *key2 = OG_KEY(*key2);
  return ret;
}

inline void CommonUtil::dump_node(node to_print, std::ostream &os = std::cout)
{
  os << "ID is: \t" << to_print.id << std::endl;
  os << "in_degree is:\t" << to_print.in_degree << std::endl;
  os << "out_degree is:\t" << to_print.out_degree << "\n\n";
}

inline void CommonUtil::dump_edge(edge to_print, std::ostream &os = std::cout)
{
  os << "SRC id is:\t" << to_print.src_id << std::endl;
  os << "DST id is:\t" << to_print.dst_id << std::endl;
  os << "Weight is:\t" << to_print.edge_weight << "\n\n";
}

inline void CommonUtil::dump_edge_index(edge_index to_print,
                                        std::ostream &os = std::cout)
{
  os << "SRC id is:\t" << to_print.src_id << std::endl;
  os << "DST id is:\t" << to_print.dst_id << std::endl;
}

inline void CommonUtil::dump_adjlist(const adjlist &to_print,
                                     std::ostream &os = std::cout)
{
  os << "Node ID is: \t" << to_print.node_id << std::endl;
  os << "degree is:\t" << to_print.degree << std::endl;
  os << "Adjacency List is:\t {";
  for (node_id_t n : to_print.edgelist)
  {
    os << n << " ";
  }
  os << "}"
     << "\n\n";
}

#endif
