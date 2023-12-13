//
// Created by puneet on 12/12/23.
//
#include "common_util.h"

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

int CommonUtil::close_connection(WT_CONNECTION *conn)
{
    int ret = conn->close(conn, nullptr);
    if (ret != 0)
    {
        throw GraphException("failed to close the connection");
    }
    return 0;
}

int CommonUtil::open_connection(const char *db_name,
                                const std::string &logdir,
                                const std::string &conn_config,
                                WT_CONNECTION **conn)
{
    char config[1024] = "create";
    std::string _config;
    _config = conn_config;
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
    std::cout << "conn_config is: " << conn_config << std::endl;

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

///**
// * @brief This function unpacks the buffer into a vector<int>. This assumes
// the
// * buffer was packed using pack_int_vector_std()
// * @param to_unpack string that contains the packed vector
// * @return std::vector<int> unpacked buffer
// */
// std::vector<node_id_t> CommonUtil::unpack_int_vector_wti(WT_SESSION *session,
//                                                         size_t size,
//                                                         char *packed_str) {
//  WT_PACK_STREAM *psp;
//  WT_ITEM unpacked;
//  size_t used;
//
//  wiredtiger_unpack_start(session, "u", packed_str, size, &psp);
//  wiredtiger_unpack_item(psp, &unpacked);
//  wiredtiger_pack_close(psp, &used);
//
//  int vec_size = (int) size / sizeof(node_id_t);
//  std::vector<node_id_t> unpacked_vec(vec_size);
//  for (int i = 0; i < vec_size; i++)
//    unpacked_vec[i] = ((node_id_t *) unpacked.data)[i];
//
//  return unpacked_vec;
//}

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
