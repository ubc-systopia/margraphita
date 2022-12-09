#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"
#ifndef BASE_COMMON
#define BASE_COMMON

#include <stdarg.h>
#include <stdio.h>
#include <wiredtiger.h>

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "graph_exception.h"

// These are the string constants
const std::string METADATA = "metadata";
const std::string DB_NAME = "db_name";
const std::string DB_DIR = "db_dir";
const std::string IS_WEIGHTED = "is_weighted";
const std::string READ_OPTIMIZE = "read_optimize";
const std::string IS_DIRECTED = "is_directed";

// Read Optimize columns
const std::string IN_DEGREE = "in_degree";
const std::string OUT_DEGREE = "out_degree";

// Shared column names
const std::string SRC = "src";
const std::string DST = "dst";
const std::string ID = "id";
const std::string ATTR_FIRST =
    "attr_fst";  // Used in EdgeKey as the first attribute.
const std::string ATTR_SECOND =
    "attr_scnd";  // Used in EdgeKey as the second attribute.
const std::string WEIGHT = "weight";
const std::string NODE_TABLE = "node";
const std::string EDGE_TABLE = "edge";
const std::string SRC_INDEX = "IX_edge_" + SRC;
const std::string DST_INDEX = "IX_edge_" + DST;
const std::string SRC_DST_INDEX = "IX_edge_" + SRC + DST;
const std::string DST_SRC_INDEX = "IX_edge_" + DST + SRC;
// specific to AdjList implementation
const std::string OUT_ADJLIST = "adjlistout";
const std::string IN_ADJLIST = "adjlistin";
const std::string node_count = "nNodes";
const std::string edge_count = "nEdges";

typedef int32_t node_id_t;
typedef int32_t edgeweight_t;
typedef uint32_t degree_t;
const node_id_t OutOfBand_ID =
    0;  // Used to be -1. Changed to 0 to avoid issues with unsigned types.
const degree_t OutOfBand_Val = UINT32_MAX;

typedef enum GraphType
{
    Std,
    Adj,
    EKey,
} GraphType;

struct graph_opts
{
    bool create_new = true;
    bool read_optimize = true;
    bool is_directed = true;
    bool is_weighted = false;
    std::string db_name;
    std::string db_dir;
    bool optimize_create;  // directs when the index should be created
    std::string conn_config;
    std::string stat_log;
    GraphType type;
};

typedef struct node
{
    node_id_t id;  // node ID
    degree_t in_degree = 0;
    degree_t out_degree = 0;

} node;

typedef struct edge
{
    // int32_t id;
    node_id_t src_id;
    node_id_t dst_id;
    edgeweight_t edge_weight;

} edge;

typedef struct edge_index
{
    node_id_t src_id;
    node_id_t dst_id;
} edge_index;

typedef struct edge_index key_pair;

typedef struct key_range
{
    node_id_t start;
    node_id_t end;
} key_range;

typedef struct edge_range
{
    key_pair start;
    key_pair end;
} edge_range;

typedef struct adjlist
{
    node_id_t node_id;
    degree_t degree;
    std::vector<node_id_t> edgelist;
} adjlist;

// TODO: remove unused functions from this class.

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

    static char *pack_int_vector_wti(WT_SESSION *session,
                                     std::vector<node_id_t> to_pack,
                                     size_t *size);
    static std::vector<node_id_t> unpack_int_vector_wti(WT_SESSION *session,
                                                        size_t size,
                                                        char *packed_str);

    // WT Session and Cursor wrangling operations
    [[maybe_unused]] static int open_cursor(WT_SESSION *session,
                                            WT_CURSOR **cursor,
                                            const std::string &uri,
                                            WT_CURSOR *to_dup,
                                            const std::string &config);
    static int close_cursor(WT_CURSOR *cursor);
    static int close_session(WT_SESSION *session);
    static int close_connection(WT_CONNECTION *conn);
    static int open_connection(char *db_name,
                               const std::string &logdir,
                               const std::string &config,
                               WT_CONNECTION **conn);
    static void set_key(WT_CURSOR *cursor, node_id_t key);
    static void set_key(WT_CURSOR *cursor, node_id_t key1, node_id_t key2);
    static int get_key(WT_CURSOR *cursor, node_id_t *key);
    static int get_key(WT_CURSOR *cursor, node_id_t *key1, node_id_t *key2);
    static int open_session(WT_CONNECTION *conn, WT_SESSION **session);
    static void check_return(int retval, const std::string &mesg);
    static void dump_node(node to_print);
    static void dump_edge(edge to_print);
    static void dump_adjlist(const adjlist &to_print);
    [[maybe_unused]] static void dump_edge_index(edge_index to_print);

    static int node_to_record(WT_CURSOR *cursor,
                              node to_insert,
                              bool read_optimize);
    static void record_to_node(WT_CURSOR *cursor,
                               node *found,
                               bool read_optimize);
    static void record_to_edge(WT_CURSOR *cursor, edge *found);
    static void read_from_edge_idx(WT_CURSOR *idx_cursor, edge *e_idx);
    static int adjlist_to_record(WT_SESSION *session,
                                 WT_CURSOR *cursor,
                                 const adjlist &to_insert);
    static void record_to_adjlist(WT_SESSION *session,
                                  WT_CURSOR *cursor,
                                  adjlist *found);
    static void record_to_node_ekey(WT_CURSOR *cur, node *found);
    static void record_to_node_ekeyidx(WT_CURSOR *idx_cursor, node *found);
    static void record_to_edge_ekey(WT_CURSOR *cur, edge *found);
    static void get_val_ekeyidx(WT_CURSOR *idx_cursor, node_id_t *a, node_id_t *b);

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
    uint32_t a = __builtin_bswap32(key);
    WT_ITEM k = {.data = &a, .size = sizeof(a)};
    cursor->set_key(cursor, &k);
}

inline void CommonUtil::set_key(WT_CURSOR *cursor,
                                node_id_t key1,
                                node_id_t key2)
{
    uint32_t a = __builtin_bswap32(key1);
    uint32_t b = __builtin_bswap32(key2);
    WT_ITEM k1 = {.data = &a, .size = sizeof(a)};
    WT_ITEM k2 = {.data = &b, .size = sizeof(b)};
    cursor->set_key(cursor, &k1, &k2);
}

inline int CommonUtil::get_key(WT_CURSOR *cursor, node_id_t *key)
{
    WT_ITEM k;
    int ret = cursor->get_key(cursor, &k);
    uint32_t a = *(uint32_t *)k.data;
    *key = __builtin_bswap32(a);
    return ret;
}

inline int CommonUtil::get_key(WT_CURSOR *cursor,
                                node_id_t *key1,
                                node_id_t *key2)
{
    WT_ITEM k1, k2;
    int ret = cursor->get_key(cursor, &k1, &k2);
    uint32_t a = *(uint32_t *)k1.data;
    uint32_t b = *(uint32_t *)k2.data;
    *key1 = __builtin_bswap32(a);
    *key2 = __builtin_bswap32(b);
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
 * cursor, and returns the
 * @param cursor
 * @return node
 */
inline void CommonUtil::record_to_node(WT_CURSOR *cursor,
                                       node *found,
                                       bool read_optimize)
{
    found->in_degree = 0;
    found->out_degree = 0;

    if (read_optimize)
    {
        cursor->get_value(cursor, &found->in_degree, &found->out_degree);
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
    uint32_t src_id = *(uint32_t *)src.data;
    uint32_t dst_id = *(uint32_t *)dst.data;
    e_idx->src_id = __builtin_bswap32(src_id);
    e_idx->dst_id = __builtin_bswap32(dst_id);
}

inline void CommonUtil::get_val_ekeyidx(WT_CURSOR *idx_cursor, node_id_t *a, node_id_t *b)
{
    WT_ITEM x, y;
    idx_cursor->get_value(idx_cursor, &x, &y);
    uint32_t src_id = *(uint32_t *)x.data;
    uint32_t dst_id = *(uint32_t *)y.data;
    *a = __builtin_bswap32(src_id);
    *b = __builtin_bswap32(dst_id);
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
    size_t size;
    WT_ITEM item;
    char *buf =
        CommonUtil::pack_int_vector_wti(session, to_insert.edgelist, &size);
    item.data = buf;
    item.size = size;

    cursor->set_value(cursor, to_insert.degree, &item);
    int ret = cursor->update(cursor);
    return ret;
}

/**
 * @brief This function converts the record the cursor passed points to into
 * a adjlist struct
 *
 * @param cursor the cursor set to the record which needs to be read
 * @return adjlist the found adjlist struct.
 */
inline void CommonUtil::record_to_adjlist(WT_SESSION *session,
                                          WT_CURSOR *cursor,
                                          adjlist *found)
{
    int32_t degree;
    WT_ITEM item;
    cursor->get_value(cursor, &degree, &item);
    found->edgelist = CommonUtil::unpack_int_vector_wti(
        session, item.size, (char *)item.data);
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
/****
 * The following are iterator definitions.
 */

class table_iterator
{
   protected:
    WT_CURSOR *cursor = nullptr;
    WT_SESSION *session = nullptr;
    bool is_first = true;
    bool has_next = true;
    void init(WT_CURSOR *cursor_, WT_SESSION *sess_)
    {
        cursor = cursor_;
        session = sess_;
    }
    // TODO: Change this to accept a Graph object reference and the table/index
    // name for which the cursor is sought. Current design needs the user to
    // know which table to provide a cursor for. This is guaranteed to cause
    // bugs.
   public:
    void set_key(node_id_t key) { CommonUtil::set_key(cursor, key); }
    bool has_more() { return has_next; };
    virtual void reset()
    {
        cursor->reset(cursor);
        is_first = true;
        has_next = true;
    }
};

class OutCursor : public table_iterator
{
   protected:
    key_range keys{};
    int num_nodes{};

   public:
    OutCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        init(cur, sess);
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        CommonUtil::set_key(cursor, keys.start);
    }

    void set_num_nodes(int num) { num_nodes = num; }

    virtual void next(adjlist *found) = 0;
    virtual void next(adjlist *found, node_id_t key) = 0;
};

class InCursor : public table_iterator
{
   protected:
    key_range keys{};
    int num_nodes{};

   public:
    InCursor(WT_CURSOR *cur, WT_SESSION *sess)
    {
        init(cur, sess);
        keys = {-1, -1};
    }

    void set_key_range(key_range _keys)
    {
        keys = _keys;
        CommonUtil::set_key(cursor, keys.start);
    }

    void set_num_nodes(int num) { num_nodes = num; }

    virtual void next(adjlist *found) = 0;
    virtual void next(adjlist *found, node_id_t key) = 0;
};

class NodeCursor : public table_iterator
{
   protected:
    key_range keys{};

   public:
    NodeCursor(WT_CURSOR *node_cur, WT_SESSION *sess)
    {
        init(node_cur, sess);
        keys = {-1, -1};
    }

    /**
     * @brief Set the key range object
     *
     * @param _keys the key range object. Set the end key to INT_MAX if you want
     * to get all the nodes from start node.
     */
    virtual void set_key_range(key_range _keys)  // overrided by edgekey
    {
        keys = _keys;
        CommonUtil::set_key(cursor, keys.start);
    }

    virtual void next(node *found) = 0;
};

class EdgeCursor : public table_iterator
{
   protected:
    key_pair start_edge{};
    key_pair end_edge{};
    bool get_weight = true;

   public:
    EdgeCursor(WT_CURSOR *composite_edge_cur, WT_SESSION *sess)
    {
        init(composite_edge_cur, sess);
        start_edge = {-1, -1};
        end_edge = {-1, -1};
    }

    EdgeCursor(WT_CURSOR *composite_edge_cur, WT_SESSION *sess, bool get_weight)
    {
        init(composite_edge_cur, sess);
        start_edge = {-1, -1};
        end_edge = {-1, -1};
        this->get_weight = get_weight;
    }

    // Overwrites set_key(int key) implementation in table_iterator
    void set_key(int key) = delete;

    void set_key(edge_range range)
    {
        start_edge = range.start;
        end_edge = range.end;
        CommonUtil::set_key(cursor, range.start.src_id, range.start.dst_id);
    }

    virtual void next(edge *found) = 0;
};

#endif

#pragma clang diagnostic pop