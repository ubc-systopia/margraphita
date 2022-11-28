#include "common.h"

#include <stdlib.h>
#include <wiredtiger.h>

#include <cstring>
#include <iostream>
#include <string>
#include <variant>

#include "graph_exception.h"
#ifdef MACOSX
#include <experimental/filesystem>
#endif
#ifdef LINUX
#include <filesystem>
#endif

// const std::string METADATA

// Read Optimize columns

// Shared column names

void CommonUtil::create_dir(std::string path)
{
#ifdef MACOSX
    std::experimental::filesystem::path dirname = path;
    if (std::experimental::filesystem::exists(dirname))
    {
        std::experimental::filesystem::remove_all(
            dirname);  // remove if exists;
    }
    std::experimental::filesystem::create_directories(dirname);
#endif
#ifdef LINUX
    std::filesystem::path dirname = path;
    if (std::filesystem::exists(dirname))
    {
        std::filesystem::remove_all(dirname);  // remove if exists;
    }
    std::filesystem::create_directories(dirname);
#endif
}

void CommonUtil::remove_dir(std::string path)
{
#ifdef MACOSX
    std::experimental::filesystem::remove_all(path);
#endif
#ifdef LINUX
    std::filesystem::remove_all(path);
#endif
}

bool CommonUtil::check_dir_exists(std::string path)
{
#ifdef MACOSX
    if (std::experimental::filesystem::exists(path))
    {
        return true;
    }
    else
    {
        return false;
    }
#endif
#ifdef LINUX
    if (std::filesystem::exists(path))
    {
        return true;
    }
    else
    {
        return false;
    }
#endif
    return false;
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

/**
 * @brief This function is used to construct the default values based on format
 * string.
 *
 * @param fmt This is the format string.
 * @return std::vector<int> This is a vector that contains the default values
 * for format types that are represented as ints. (0)
 */
std::vector<int> CommonUtil::get_default_nonstring_attrs(const std::string &fmt)
{
    std::vector<int> ret;
    std::string zero_attr = "iIlLqQsShHt";

    for (const char &x : fmt)
    {
        if (zero_attr.find(x) != std::string::npos)
        {
            ret.emplace_back(0);
        }
        else
        {
            throw GraphException(
                "Found a non-int type in format string. Abort!");
        }
    }
    return ret;
}

/**
 * @brief This function is used to construct the default values based on format
 * string.
 *
 * @param fmt This is the format string.
 * @return std::vector<std::string> This is a vector that contains the default
 * values for format types that are represented as strings. ("")
 */
std::vector<std::string> CommonUtil::get_default_string_attrs(
    const std::string &fmt)
{
    std::vector<std::string> ret;
    std::string zero_attr = "iIlLqQsShHt";

    for (const char &x : fmt)
    {
        if (zero_attr.find(x) != std::string::npos)
        {
            throw GraphException(
                "Found a non-int type in format string. Abort!");
        }
        else
        {
            ret.emplace_back("");
        }
    }
    return ret;
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
/**
 * @brief This function is used to compute the format string that are expected
 * by the wiredtiger_pack family of functions. This is also useful to get the
 * value of buffer needed to persist such a string.
 *
 * @param to_pack vector<string> This is the vectorthat needs to be packed
 * @param total_size int* This is a pointer to the variable that holds the size
 * that must be used to alloc a buffer to persist the vector
 * @return fmt std::string this is the format string. The first char in the
 * format string is S to indicate that the first string being packed is the
 * format string.
 */
std::string CommonUtil::create_string_format(
    const std::vector<std::string> &to_pack, size_t *total_size)
{
    std::string fmt = "Si";  // The first element of the packed string contains
                             // the format string. Second element contains the
                             // buffer size needed to unpack this vector

    for (const std::string &item : to_pack)
    {
        fmt += 'S';  // std::to_string(item.length()) + 'S';
        *total_size += (item.length() + 1);  // for \0
    }
    *total_size += (fmt.length() + 1 + sizeof(int) +
                    1);  // Add the size of the first element + \0 + size of int
                         // + another \0
    return fmt;
}

// todo: What I don't understand is that given the size of the strings/ints that
// need to be packed (sum of all string/int sizes in the vector), should I
// create a buf of total_size*sizeof(int/char) or should it be total_size bytes
// in. size?
/**
 * @brief This function is used to compute the format string that is expected by
 * the wiredtiger_pack family of fucntions. This is used to crearte an
 * appropriately sized buffer to store the packed vector<int>
 *
 * @param to_pack vector<int> - the vector that has to be packed.
 * @param total_size his is a pointer to the variable that holds the size
 * that must be used to alloc a buffer to persist the vector.
 * @return std::string std::string this is the format string. The first char in
 * the format string is S to indicate that the first string being packed in to
 * the stream will be the format string itself.
 */
std::string CommonUtil::create_intvec_format(std::vector<int> to_pack,
                                             size_t *total_size)
{
    std::string fmt = "S";

    for (auto iter = to_pack.begin(); iter != to_pack.end(); ++iter)
    {
        fmt += 'i';
        *total_size = *total_size + 1;
    }
    return fmt;
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
    if (session->close(session, NULL) != 0)
    {
        fprintf(stderr, "Failed to close session\n");
        return (-1);
    }
    return 0;
}

int CommonUtil::close_connection(WT_CONNECTION *conn)
{
    if (conn->close(conn, NULL) != 0)
    {
        fprintf(stderr, "Failed to close connection\n");
        return (-1);
    }
    return 0;
}

int CommonUtil::open_connection(char *db_name,
                                std::string log_dir,
                                std::string conn_config,
                                WT_CONNECTION **conn)
{
    char config[1024] = "create";
    // add the config string

#ifdef STAT
    if (conn_config.length() > 0)
    {
        conn_config += ",";
    }
    conn_config +=
        "statistics=(all),statistics_log=(wait=0,on_close=true,path=" +
        log_dir + ")";
#endif
    if (conn_config.size() != 0)
    {
        sprintf(config + strlen("create"), ",%s", conn_config.c_str());
    }
    // std::cout << config << std::endl;
    // exit(1);
    if (wiredtiger_open(db_name, NULL, config, conn) != 0)
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
    if (conn->open_session(conn, NULL, "isolation=snapshot", session) != 0)
    {
        fprintf(stderr, "Failed to open session\n");
        return (-1);
    }
    return 0;
}

int CommonUtil::open_cursor(WT_SESSION *session,
                            WT_CURSOR **cursor,
                            std::string uri,
                            WT_CURSOR *to_dup,
                            std::string config)
{
    if (session->open_cursor(
            session, uri.c_str(), to_dup, config.c_str(), cursor) != 0)
    {
        fprintf(stderr, "Failed to open the cursor on URI %s", uri.c_str());
    }
    return 0;
}

void CommonUtil::check_return(int retval, std::string mesg)
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
    std::cout << "out_degree is:\t" << to_print.out_degree << std::endl;
}

void CommonUtil::dump_edge(edge to_print)
{
    std::cout << "SRC id is:\t" << to_print.src_id << std::endl;
    std::cout << "DST id is:\t" << to_print.dst_id << std::endl;
    std::cout << "Weight is:\t" << to_print.edge_weight << std::endl;
}

void CommonUtil::dump_edge_index(edge_index to_print)
{
    std::cout << "SRC id is:\t" << to_print.src_id << std::endl;
    std::cout << "DST id is:\t" << to_print.dst_id << std::endl;
}

void CommonUtil::dump_adjlist(adjlist to_print)
{
    std::cout << "Node ID is: \t" << to_print.node_id << std::endl;
    std::cout << "degree is:\t" << to_print.degree << std::endl;
    std::cout << "Adjacency List is:\t {";
    for (int n : to_print.edgelist)
    {
        std::cout << n << " ";
    }
    std::cout << "}" << std::endl;
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
