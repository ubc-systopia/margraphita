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
extern const std::string METADATA;
extern const std::string DB_NAME;
extern const std::string DB_DIR;
extern const std::string IS_WEIGHTED;
extern const std::string READ_OPTIMIZE;
extern const std::string IS_DIRECTED;
extern const std::string CREATE_NEW;
extern const std::string EDGE_ID;

// Read Optimize columns
extern const std::string IN_DEGREE;
extern const std::string OUT_DEGREE;

// Shared column names
extern const std::string SRC;
extern const std::string DST;
extern const std::string ID;
extern const std::string ATTR;  // Used in EdgeKey as the packed binary.
extern const std::string WEIGHT;
extern const std::string NODE_TABLE;
extern const std::string EDGE_TABLE;
extern const std::string SRC_INDEX;
extern const std::string DST_INDEX;
extern const std::string SRC_DST_INDEX;
extern const std::string DST_SRC_INDEX;
// specific to AdjList implementation
extern const std::string IN_ADJLIST;   // New
extern const std::string OUT_ADJLIST;  // New
extern const std::string node_count;
extern const std::string edge_count;

typedef int64_t node_id_t;
typedef int32_t edgeweight_t;
typedef uint32_t degree_t;
const node_id_t OutOfBand_ID = -1;

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

typedef struct wt_conn_info
{
    WT_SESSION *session;
    WT_CURSOR *cursor;
} wt_conn;

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

    edge_range(key_pair _start, key_pair _end) : start(_start), end(_end) {}
} edge_range;

typedef struct adjlist
{
    node_id_t node_id;
    degree_t degree;
    std::vector<node_id_t> edgelist;
} adjlist;

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
    void set_key(node_id_t key) { cursor->set_key(cursor, key); }
    bool has_more() { return has_next; };
    virtual void reset()
    {
        cursor->reset(cursor);
        is_first = true;
        has_next = true;
    }
};

// TODO: remove unused functions from this class.

class CommonUtil
{
   public:
    static void create_dir(std::string path);
    static void remove_dir(std::string path);

    static bool check_dir_exists(std::string path);

    static void set_table(WT_SESSION *session,
                          std::string prefix,
                          std::vector<std::string> columns,
                          std::string key_fmt,
                          std::string val_fmt);

    static std::vector<int> get_default_nonstring_attrs(std::string fmt);

    static std::vector<std::string> get_default_string_attrs(std::string fmt);

    static std::string get_db_name(std::string prefix, std::string name);

    static void check_graph_params(graph_opts params);

    static std::string create_string_format(std::vector<std::string> to_pack,
                                            size_t *size);
    static std::string create_intvec_format(std::vector<int> to_pack,
                                            size_t *total_size);
    static char *pack_string_vector_wt(std::vector<std::string>,
                                       WT_SESSION *session,
                                       size_t *total_size,
                                       std::string *fmt);

    static std::vector<std::string> unpack_string_vector_wt(
        const char *to_unpack, WT_SESSION *session);

    static char *pack_int_vector_wt(std::vector<int>,
                                    WT_SESSION *session,
                                    size_t *total_size,
                                    std::string *fmt);
    static std::vector<int> unpack_int_vector_wt(const char *to_unpack,
                                                 WT_SESSION *session);

    static char *pack_string_wt(std::string,
                                WT_SESSION *session,
                                std::string *fmt);
    static std::string unpack_string_wt(const char *to_unpack,
                                        WT_SESSION *session);

    static char *pack_int_wt(int to_pack, WT_SESSION *session);
    static int unpack_int_wt(const char *to_unpack, WT_SESSION *session);

    static char *pack_bool_wt(bool, WT_SESSION *session, std::string *fmt);
    static bool unpack_bool_wt(const char *to_unpack, WT_SESSION *session);

    static std::string pack_string_vector_std(std::vector<std::string> to_pack,
                                              size_t *size);
    static std::vector<std::string> unpack_string_vector_std(
        std::string to_unpack);

    static std::string pack_int_vector_std(std::vector<int> to_pack,
                                           size_t *size);
    static std::vector<int> unpack_int_vector_std(std::string packed_str);

    static char *pack_int_vector_wti(WT_SESSION *session,
                                     std::vector<node_id_t> to_pack,
                                     size_t *size);
    static std::vector<node_id_t> unpack_int_vector_wti(WT_SESSION *session,
                                                        size_t size,
                                                        char *packed_str);

    // WT Session and Cursor wrangling operations
    static int open_cursor(WT_SESSION *session,
                           WT_CURSOR **cursor,
                           std::string uri,
                           WT_CURSOR *to_dup,
                           std::string config);
    static int close_cursor(WT_CURSOR *cursor);
    static int close_session(WT_SESSION *session);
    static int close_connection(WT_CONNECTION *conn);
    static int open_connection(char *db_name,
                               std::string logdir,
                               std::string config,
                               WT_CONNECTION **conn);
    static int open_session(WT_CONNECTION *conn, WT_SESSION **session);
    static void check_return(int retval, std::string mesg);
    static void dump_node(node to_print);
    static void dump_edge(edge to_print);
    static void dump_adjlist(adjlist to_print);
    static void dump_edge_index(edge_index to_print);

    // Experimental WT_ITEM packing interface
    static WT_ITEM pack_vector_string(WT_SESSION *session,
                                      std::vector<std::string> to_pack,
                                      std::string *fmt);
    static std::vector<std::string> unpack_vector_string(WT_SESSION *session,
                                                         WT_ITEM packed,
                                                         std::string format);

    static WT_ITEM pack_vector_int(WT_SESSION *session,
                                   std::vector<int> to_pack,
                                   std::string *fmt);
    static std::vector<int> unpack_vector_int(WT_SESSION *,
                                              WT_ITEM packed,
                                              std::string format);

    static WT_ITEM pack_items(WT_SESSION *session, std::string fmt, ...);

    /********************************************************************************************
     *            Serialization/ Deserialization methods common to all
     *                             *representations*
     ********************************************************************************************/

    /**
     * @brief This fucntion updates a node with the given attribute vector.
     * This function assumes that only the node attributes are updated here.
     * Node data is modified using get/set_node_data() and degrees are modified
     * automatically on edge insertions/deletions.
     * @param node_id The Node ID to be updated
     * @param new_attrs The new node attribute vector.
     */
    inline static void __node_to_record(WT_CURSOR *cursor,
                                        node to_insert,
                                        bool read_optimize)
    {
        // cursor cannot be null
        cursor->set_key(cursor, to_insert.id);
        if (cursor->search(cursor) != 0)
        {
            throw GraphException("Failed to find a node with node_id " +
                                 std::to_string(to_insert.id));
        }
        if (read_optimize)
        {
            cursor->set_value(
                cursor, to_insert.in_degree, to_insert.out_degree);
        }
        else
        {
            cursor->set_value(cursor, "");
        }
        if (cursor->update(cursor) != 0)
        {
            throw GraphException("Failed to update node_id " +
                                 std::to_string(to_insert.id));
        }
    }

    /**
     * @brief This function reads the record currently being pointed to by the
     * cursor, and returns the
     * @param cursor
     * @return node
     */
    inline static void __record_to_node(WT_CURSOR *cursor,
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

    inline static void __record_to_edge(WT_CURSOR *cursor, edge *found)
    {
        if (cursor->get_value(cursor, &found->edge_weight) != 0)
        {
            throw GraphException("Could not get the value from the edge table");
        }
    }

    inline static void __read_from_edge_idx(WT_CURSOR *idx_cursor, edge *e_idx)
    {
        idx_cursor->get_value(idx_cursor, &e_idx->src_id, &e_idx->dst_id);
    }

    /**
     * @brief This function accepts a cursor to the adjlist table and an adjlist
     * sturct to insert.
     * @param cursor A cursor to the in/out adjlist table
     * @param to_insert The adjlist struct to be inserted into the table pointed
     * to by the cursor.
     * @throws GraphException If insertion into the table fails
     */
    inline static void __adjlist_to_record(WT_SESSION *session,
                                           WT_CURSOR *cursor,
                                           adjlist to_insert)
    {
        cursor->reset(cursor);
        cursor->set_key(cursor, to_insert.node_id);
        size_t size;
        WT_ITEM item;
        char *buf =
            CommonUtil::pack_int_vector_wti(session, to_insert.edgelist, &size);
        item.data = buf;
        item.size = size;

        cursor->set_value(cursor, to_insert.degree, &item);
        int ret = cursor->insert(cursor);
        if (ret != 0)
        {
            throw GraphException("Could not insert adjlist for " +
                                 std::to_string(to_insert.node_id) +
                                 " into the adjlist table");
        }
    }

    /**
     * @brief This function converts the record the cursor passed points to into
     * a adjlist struct
     *
     * @param cursor the cursor set to the record which needs to be read
     * @return adjlist the found adjlist struct.
     */
    inline static void __record_to_adjlist(WT_SESSION *session,
                                           WT_CURSOR *cursor,
                                           adjlist *found)
    {
        int32_t degree;
        WT_ITEM item;
        cursor->get_value(cursor, &degree, &item);
        found->edgelist = CommonUtil::unpack_int_vector_wti(
            session, item.size, (char *)item.data);
        if (degree == 1 && found->edgelist.size() == 0)
        {
            found->degree = 0;
        }
        else
        {
            found->degree = degree;
        }
    }
    // Because we know that there can only be two. By design.
    inline static void extract_from_string(std::string packed_str,
                                           int *a,
                                           int *b)
    {
        std::stringstream strstream(packed_str);
        strstream >> *a >> *b;
        return;
    }

    inline static std::string pack_int_to_str(int a, int b)
    {
        std::stringstream sstream;
        sstream << a << " " << b;
        return sstream.str();
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
    inline static void __record_to_node_ekey(WT_CURSOR *cur, node *found)
    {
        char *packed_vec;
        int ret = cur->get_value(cur, &packed_vec);
        if (ret != 0)
        {
            throw GraphException("in here");
        }
        std::string str(packed_vec);
        int a = 0, b = 0;
        extract_from_string(packed_vec, &a, &b);
        found->in_degree = a;
        found->out_degree = b;
    }

    inline static void __record_to_edge_ekey(WT_CURSOR *cur, edge *found)
    {
        char *packed_vec;
        int ret = cur->get_value(cur, &packed_vec);
        if (ret != 0)
        {
            throw GraphException("in here");
        }
        std::string str(packed_vec);
        int a = 0, b = 0;
        extract_from_string(str, &a, &b);
        found->edge_weight = a;
    }
};

#endif
