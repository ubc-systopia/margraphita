#ifndef BASE_COMMON
#define BASE_COMMON

#include <stdarg.h>
#include <stdio.h>
#include <wiredtiger.h>

#include <bitset>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
// #ifdef LINUX
// #include <source_location>
// #define SRC_LOC std::source_location::current()
// #endif
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "common_defs.h"
#include "graph_exception.h"
#include "iterator.h"

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

    static void log_msg(std::string_view message,
                        std::string_view file,
                        int location);

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
    static int dup_cursor(WT_SESSION *session,
                          WT_CURSOR *to_dup,
                          WT_CURSOR **cursor);
    static void set_key(WT_CURSOR *cursor, node_id_t key);
    static void set_key(WT_CURSOR *cursor, node_id_t key1, node_id_t key2);
    static void set_key(WT_CURSOR *cursor,
                        int32_t id,
                        node_id_t key1,
                        node_id_t key2);
    static int get_key(WT_CURSOR *cursor, node_id_t *key);
    static int get_key(WT_CURSOR *cursor, node_id_t *key1, node_id_t *key2);
    static int get_key(WT_CURSOR *cursor,
                       int32_t *id,
                       node_id_t *key1,
                       node_id_t *key2);
    static node_id_t extract_flip_endian(WT_ITEM id);
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
    static void get_val_idx(WT_CURSOR *idx_cursor, node_id_t *a, node_id_t *b);
    static void get_ekey_dst_src_val(WT_CURSOR *idx_cursor,
                                     node_id_t *a,
                                     node_id_t *b,
                                     degree_t *in,
                                     degree_t *out);
};

void CommonUtil::create_dir(const std::string &path)
{
    std::filesystem::path dirname = path;
    if (std::filesystem::exists(dirname))
    {
        std::filesystem::remove_all(dirname);  // remove if exists;
    }
    std::filesystem::create_directories(dirname);
}

bool CommonUtil::check_dir_exists(const std::string &path)
{
    if (std::filesystem::exists(path))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void CommonUtil::set_table(WT_SESSION *session,
                           const std::string &prefix,
                           std::vector<std::string> columns,
                           const std::string &key_fmt,
                           const std::string &val_fmt)
{
    if (!columns.empty())
    {
        std::vector<std::string>::iterator ptr;
        std::string concat = columns.at(0);
        for (ptr = columns.begin() + 1; ptr < columns.end(); ptr++)
        {
            concat += "," + *ptr;
        }

        // Now insert in WT
        std::string table_name = "table:" + prefix;
        std::string wt_format_string = "key_format=" + key_fmt +
                                       ",value_format=" + val_fmt +
                                       ",columns=(" + concat + ")";
        char *n = const_cast<char *>(table_name.c_str());
        char *f = const_cast<char *>(wt_format_string.c_str());
        session->create(session, n, f);
    }
    else
    {
        std::string table_name = "table:" + prefix;
        session->create(
            session, table_name.c_str(), "key_format=I,value_format=I");
    }
}

std::string CommonUtil::get_db_name(const std::string &prefix,
                                    const std::string &name)
{
    return (prefix + "-" + name);
}

void CommonUtil::check_graph_params(const graph_opts &params)
{
    std::vector<std::string> missing_params;

    // TODO: this needs to be updated
    if (!missing_params.empty())
    {
        std::vector<std::string>::iterator missing_param_ptr;
        std::string to_return = missing_params.at(0);
        for (missing_param_ptr = missing_params.begin() + 1;
             missing_param_ptr < missing_params.end();
             missing_param_ptr++)
        {
            to_return += "," + *missing_param_ptr;
        }
        throw GraphException(to_return);
    }
}

int CommonUtil::close_cursor(WT_CURSOR *cursor)
{
    if (int ret = cursor->close(cursor) != 0)
    {
        fprintf(stderr, "Failed to close the cursor\n ");
        return ret;
    }
    return 0;
}

int CommonUtil::close_session(WT_SESSION *session)
{
    if (session->close(session, nullptr) != 0)
    {
        fprintf(stderr, "Failed to close session\n");
        return (-1);
    }
    return 0;
}

int CommonUtil::close_connection(WT_CONNECTION *conn)
{
    if (conn->close(conn, nullptr) != 0)
    {
        fprintf(stderr, "Failed to close connection\n");
        return (-1);
    }
    return 0;
}

int CommonUtil::open_connection(char *db_name,
                                const std::string &log_dir,
                                const std::string &conn_config,
                                WT_CONNECTION **conn)
{
    char config[1024] = "create";
    std::string _config = conn_config;
    // add the config string
    std::cout << "conn_config is: " << conn_config << std::endl;
#ifdef STAT
    if (_config.length() > 0)
    {
        _config += ",";
    }
    _config += "statistics=(all),statistics_log=(wait=0,on_close=true,path=" +
               log_dir + ")";
#endif
    if (!_config.empty())
    {
        snprintf(config + strlen("create"), 1018, ",%s", _config.c_str());
    }
    std::cout << _config << std::endl;
    // exit(1);
    if (wiredtiger_open(db_name, nullptr, config, conn) != 0)
    {
        fprintf(stderr, "Failed to open connection\n");
        return (-1);
    }
    return 0;
}

int CommonUtil::open_session(WT_CONNECTION *conn, WT_SESSION **session)
{
    // if (conn->open_session(conn, NULL, NULL, session) != 0)
    // {
    //     fprintf(stderr, "Failed to open session\n");
    //     return (-1);
    // }
    // return 0;
    if (conn->open_session(conn, nullptr, "isolation=snapshot", session) != 0)
    {
        fprintf(stderr, "Failed to open session\n");
        return (-1);
    }
    return 0;
}

[[maybe_unused]] int CommonUtil::open_cursor(WT_SESSION *session,
                                             WT_CURSOR **cursor,
                                             const std::string &uri,
                                             WT_CURSOR *to_dup,
                                             const std::string &config)
{
    if (session->open_cursor(
            session, uri.c_str(), to_dup, config.c_str(), cursor) != 0)
    {
        fprintf(stderr, "Failed to open the cursor on URI %s", uri.c_str());
    }
    return 0;
}

[[maybe_unused]] int CommonUtil::dup_cursor(WT_SESSION *session,
                                            WT_CURSOR *to_dup,
                                            WT_CURSOR **cursor)
{
    if (session->open_cursor(session, nullptr, to_dup, nullptr, cursor) != 0)
    {
        fprintf(stderr, "Failed to duplicte the cursor on URI %s", to_dup->uri);
    }
    return 0;
}

void CommonUtil::check_return(int retval, const std::string &mesg)
{
    if (retval > 0)
    {
        throw GraphException(mesg);
    }
}

void CommonUtil::dump_node(node to_print)
{
    std::cout << "ID is: \t" << to_print.id << std::endl;
    std::cout << "in_degree is:\t" << to_print.in_degree << std::endl;
    std::cout << "out_degree is:\t" << to_print.out_degree << "\n\n";
}

void CommonUtil::dump_edge(edge to_print)
{
    std::cout << "SRC id is:\t" << to_print.src_id << std::endl;
    std::cout << "DST id is:\t" << to_print.dst_id << std::endl;
    std::cout << "Weight is:\t" << to_print.edge_weight << "\n\n";
}

[[maybe_unused]] void CommonUtil::dump_edge_index(edge_index to_print)
{
    std::cout << "SRC id is:\t" << to_print.src_id << std::endl;
    std::cout << "DST id is:\t" << to_print.dst_id << "\n\n";
}

void CommonUtil::dump_adjlist(const adjlist &to_print)
{
    std::cout << "Node ID is: \t" << to_print.node_id << std::endl;
    std::cout << "degree is:\t" << to_print.degree << std::endl;
    std::cout << "Adjacency List is:\t {";
    for (int n : to_print.edgelist)
    {
        std::cout << n << " ";
    }
    std::cout << "}"
              << "\n\n";
}

/**
 * We need to change how the int/string vector packing is implemented:
 * 1. pack_int_vector
 * 2. unpack_int_vector
 * 3. pack_string_vector
 * 4. unpack_string_vector
 *
 */

/**
 * @brief This function unpacks the buffer into a vector<int>. This assumes the
 * buffer was packed using pack_int_vector_std()
 * @param to_unpack string that contains the packed vector
 * @return std::vector<int> unpacked buffer
 */
std::vector<node_id_t> CommonUtil::unpack_int_vector_wti(WT_SESSION *session,
                                                         size_t size,
                                                         char *packed_str)
{
    WT_PACK_STREAM *psp;
    WT_ITEM unpacked;
    size_t used;

    wiredtiger_unpack_start(session, "u", packed_str, size, &psp);
    wiredtiger_unpack_item(psp, &unpacked);
    wiredtiger_pack_close(psp, &used);

    int vec_size = (int)size / sizeof(node_id_t);
    std::vector<node_id_t> unpacked_vec(vec_size);
    for (int i = 0; i < vec_size; i++)
        unpacked_vec[i] = ((node_id_t *)unpacked.data)[i];

    return unpacked_vec;
}

/**
 * @brief This function is used to pack all integers in the integer vector by
 * using the wiredtiger packing stream interface. There's nothing to it, really:
 * This packs it into a malloc'd buffer and sets the size variable passed as
 * argument.
 *
 * @param session The WiredTiger Session variable
 * @param to_pack The vector of ints to pack.
 * @param size pointer to a size_t variable to store the size of buffer. THIS IS
 * NEEDED TO UNPACK -> STORE THIS IN THE TABLE
 * @return buffer containing the packed array.
 */
char *CommonUtil::pack_int_vector_wti(WT_SESSION *session,
                                      std::vector<node_id_t> to_pack,
                                      size_t *size)
{
    WT_PACK_STREAM *psp;
    WT_ITEM item;
    item.data = to_pack.data();
    item.size = sizeof(node_id_t) * to_pack.size();

    void *pack_buf = malloc(sizeof(node_id_t) * to_pack.size());
    int ret = wiredtiger_pack_start(session, "u", pack_buf, item.size, &psp);
    if (ret == 0)
    {
        wiredtiger_pack_item(psp, &item);
        wiredtiger_pack_close(psp, size);
    }
    else
    {
        std::cerr << "Error in packing" << std::endl;
        exit(-1);
    }

    return (char *)pack_buf;
}

void CommonUtil::log_msg(const std::string_view message,
                         const std::string_view file,
                         int line)
{
    std::cerr << "file: " << file << "@" << line << ": " << message << '\n';
}

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

inline node_id_t CommonUtil::extract_flip_endian(WT_ITEM id)
{
    node_id_t a;
    memcpy(&a, id.data, sizeof(a));
    return __builtin_bswap32(a);
}

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

inline void CommonUtil::set_key(WT_CURSOR *cursor,
                                int32_t id,
                                node_id_t key1,
                                node_id_t key2)
{
    uint32_t a = __builtin_bswap32(key1);
    uint32_t b = __builtin_bswap32(key2);
    WT_ITEM k1 = {.data = &a, .size = sizeof(a)};
    WT_ITEM k2 = {.data = &b, .size = sizeof(b)};
    cursor->set_key(cursor, id, &k1, &k2);
}

inline int CommonUtil::get_key(WT_CURSOR *cursor, node_id_t *key)
{
    WT_ITEM k = {0};
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

inline int CommonUtil::get_key(WT_CURSOR *cursor,
                               int32_t *id,
                               node_id_t *key1,
                               node_id_t *key2)
{
    WT_ITEM k1, k2;
    int32_t found_id;
    int ret = cursor->get_key(cursor, &found_id, &k1, &k2);
    uint32_t a = *(uint32_t *)k1.data;
    uint32_t b = *(uint32_t *)k2.data;
    *key1 = __builtin_bswap32(a);
    *key2 = __builtin_bswap32(b);
    *id = found_id;
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

inline void CommonUtil::get_val_idx(WT_CURSOR *idx_cursor,
                                    node_id_t *a,
                                    node_id_t *b)
{
    WT_ITEM x, y;
    idx_cursor->get_value(idx_cursor, &x, &y);
    uint32_t src_id = *(uint32_t *)x.data;
    uint32_t dst_id = *(uint32_t *)y.data;
    *a = __builtin_bswap32(src_id);
    *b = __builtin_bswap32(dst_id);
}

inline void CommonUtil::get_ekey_dst_src_val(WT_CURSOR *idx_cursor,
                                             node_id_t *a,
                                             node_id_t *b,
                                             degree_t *in,
                                             degree_t *out)
{
    WT_ITEM x, y;
    degree_t in_degree, out_degree;
    idx_cursor->get_value(idx_cursor, &x, &y, &in_degree, &out_degree);

    uint32_t sid = *(uint32_t *)x.data;
    uint32_t did = *(uint32_t *)y.data;

    *a = __builtin_bswap32(sid);
    *b = __builtin_bswap32(did);
    *in = in_degree;
    *out = out_degree;
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
    int ret = cursor->search(cursor);  // <-- things fail if I remove this. Why?

    size_t size;
    WT_ITEM item;
    char *buf =
        CommonUtil::pack_int_vector_wti(session, to_insert.edgelist, &size);
    item.data = buf;
    item.size = size;

    cursor->set_value(cursor, to_insert.degree, &item);

    ret = cursor->update(cursor);
    if (ret)
    {
        CommonUtil::log_msg("Error inserting into adjlist table" +
                                std::to_string(ret) + " - " +
                                wiredtiger_strerror(ret),
                            __FILE__,
                            __LINE__);
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
inline void CommonUtil::record_to_adjlist(WT_SESSION *session,
                                          WT_CURSOR *cursor,
                                          adjlist *found)
{
    int32_t degree;
    WT_ITEM item;
    cursor->get_value(cursor, &degree, &item);
    found->edgelist.assign((int *)item.data,
                           (int *)item.data + item.size / sizeof(int));
    // found->edgelist = CommonUtil::unpack_int_vector_wti(
    //     session, item.size, (char *)item.data);
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

#endif
