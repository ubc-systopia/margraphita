#ifndef BASE_COMMON
#define BASE_COMMON

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <stdarg.h>
#include <stdio.h>
#include <wiredtiger.h>

//These are the string constants
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
extern const std::string ATTR; // Used in EdgeKey as the packed binary.
extern const std::string WEIGHT;
extern const std::string NODE_TABLE;
extern const std::string EDGE_TABLE;
extern const std::string SRC_INDEX;
extern const std::string DST_INDEX;
extern const std::string SRC_DST_INDEX;
//specific to AdjList implementation
extern const std::string IN_ADJLIST;  // New
extern const std::string OUT_ADJLIST; // New

struct graph_opts
{
    bool create_new = true;
    bool read_optimize = true;
    bool is_directed = true;
    bool is_weighted = false;
    std::string db_name;
    std::string db_dir;
    bool optimize_create; // directs when the index should be created
    WT_CONNECTION *conn;
    WT_SESSION *session;
    graph_opts() : create_new(false), read_optimize(true), is_directed(true), is_weighted(false), db_name(""), db_dir(""), optimize_create(true){};
};

typedef struct node
{
    int id; // node ID
    int in_degree = 0;
    int out_degree = 0;
} node;

typedef struct edge
{
    int id;
    int src_id;
    int dst_id;
    int edge_weight; // uint8_t should get transparently cast to int (unweighted). We need two fields for weights if the graph is undirected.
} edge;

typedef struct edge_index
{
    int edge_id;
    int src_id;
    int dst_id;
} edge_index;

typedef struct adjlist
{
    int node_id;
    int degree;
    std::vector<int> edgelist;
} adjlist;

class CommonUtil
{
public:
    static void set_table(WT_SESSION *session, std::string prefix,
                          std::vector<std::string> columns, std::string key_fmt,
                          std::string val_fmt);

    static std::vector<int> get_default_nonstring_attrs(std::string fmt);

    static std::vector<std::string> get_default_string_attrs(std::string fmt);

    static std::string get_db_name(std::string prefix, std::string name);

    static void check_graph_params(graph_opts params);

    static std::string create_string_format(std::vector<std::string> to_pack,
                                            size_t *size);
    static std::string create_intvec_format(std::vector<int> to_pack,
                                            size_t *total_size);
    static char *pack_string_vector_wt(std::vector<std::string>, WT_SESSION *session,
                                       size_t *total_size, std::string *fmt);

    static std::vector<std::string> unpack_string_vector_wt(const char *to_unpack,
                                                            WT_SESSION *session);

    static char *pack_int_vector_wt(std::vector<int>, WT_SESSION *session,
                                    size_t *total_size, std::string *fmt);
    static std::vector<int> unpack_int_vector_wt(const char *to_unpack,
                                                 WT_SESSION *session);

    static char *pack_string_wt(std::string, WT_SESSION *session, std::string *fmt);
    static std::string unpack_string_wt(const char *to_unpack, WT_SESSION *session);

    static char *pack_int_wt(int to_pack, WT_SESSION *session);
    static int unpack_int_wt(const char *to_unpack, WT_SESSION *session);

    static char *pack_bool_wt(bool, WT_SESSION *session, std::string *fmt);
    static bool unpack_bool_wt(const char *to_unpack, WT_SESSION *session);

    static std::string pack_string_vector_std(std::vector<std::string> to_pack,
                                              size_t *size);
    static std::vector<std::string> unpack_string_vector_std(std::string to_unpack);

    static std::string pack_int_vector_std(std::vector<int> to_pack, size_t *size);
    static std::vector<int> unpack_int_vector_std(std::string packed_str);

    // WT Session and Cursor wrangling operations
    static int open_cursor(WT_SESSION *session, WT_CURSOR **cursor,
                           std::string uri, WT_CURSOR *to_dup,
                           std::string config);
    static int close_cursor(WT_CURSOR *cursor);
    static int close_session(WT_SESSION *session);
    static int close_connection(WT_CONNECTION *conn);
    static int open_connection(char *db_name, WT_CONNECTION **conn);
    static int open_session(WT_CONNECTION *conn, WT_SESSION **session);
    static void check_return(int retval, std::string mesg);
    static void dump_node(node to_print);
    static void dump_edge(edge to_print);
    static void dump_adjlist(adjlist to_print);

    //Experimental WT_ITEM packing interface
    static WT_ITEM pack_vector_string(WT_SESSION *session, std::vector<std::string> to_pack, std::string *fmt);
    static std::vector<std::string> unpack_vector_string(WT_SESSION *session, WT_ITEM packed, std::string format);

    static WT_ITEM pack_vector_int(WT_SESSION *session, std::vector<int> to_pack, std::string *fmt);
    static std::vector<int> unpack_vector_int(WT_SESSION *, WT_ITEM packed, std::string format);

    static WT_ITEM pack_items(WT_SESSION *session, std::string fmt, ...);
    //static std::variant<int, std::string> unpack_items(WT_SESSION *session, std::string fmt, WT_ITEM packed);
};

#endif
