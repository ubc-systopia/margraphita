#include "common.h"
#include "graph_exception.h"
#include <iostream>
#include <string>
#include <cstring>
#include <variant>
#include <cstring>
#include <stdlib.h>
#include <variant>
#include <wiredtiger.h>
#ifdef MACOSX
#include <experimental/filesystem>
#endif
#ifdef LINUX
#include <filesystem>>
#endif

const std::string METADATA = "metadata";
const std::string DB_NAME = "db_name";
const std::string DB_DIR = "db_dir";
const std::string NODE_COLUMNS = "node_columns";
const std::string EDGE_COLUMNS = "edge_columns";
const std::string NODE_ATTR_FORMAT = "node_attr_format";
const std::string EDGE_ATTR_FORMAT = "edge_attr_format";
const std::string HAS_NODE_ATTR = "has_node_attr";
const std::string HAS_EDGE_ATTR = "has_edge_attr";
const std::string NODE_VALUE_COLUMNS = "node_value_columns";
const std::string EDGE_VALUE_COLUMNS = "edge_value_columns";
const std::string NODE_VALUE_FORMAT = "node_value_format";
const std::string EDGE_VALUE_FORMAT = "edge_value_format";
const std::string READ_OPTIMIZE = "read_optimize";
const std::string EDGE_ID = "edge_id";
const std::string NODE_DATA = "data";
const std::string IS_DIRECTED = "is_directed";
const std::string IS_WEIGHTED = "is_weighted";
const std::string CREATE_NEW = "create_new";

// Read Optimize columns
const std::string IN_DEGREE = "in_degree";
const std::string OUT_DEGREE = "out_degree";

// Shared column names
const std::string SRC = "src";
const std::string DST = "dst";
const std::string ID = "id";
const std::string ATTR = "attr"; // Used in EdgeKey as the packed binary.
const std::string WEIGHT = "weight";
const std::string NODE_TABLE = "node";
const std::string EDGE_TABLE = "edge";
const std::string OUT_ADJLIST = "adjlistout";
const std::string IN_ADJLIST = "adjlistin";
const std::string SRC_INDEX = "IX_edge_" + SRC;
const std::string DST_INDEX = "IX_edge_" + DST;
const std::string SRC_DST_INDEX = "IX_edge_" + SRC + DST;

void CommonUtil::create_dir(std::string path)
{
#ifdef MACOSX
    std::experimental::filesystem::path dirname = path;
    if (std::experimental::filesystem::exists(dirname))
    {
        std::experimental::filesystem::remove_all(dirname); // remove if exists;
    }
    std::experimental::filesystem::create_directories(dirname);
#endif
#ifdef LINUX
    std::filesystem::path dirname = path;
    if (std::filesystem::exists(dirname))
    {
        std::filesystem::remove_all(dirname); // remove if exists;
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
}

void CommonUtil::set_table(WT_SESSION *session, std::string prefix,
                           std::vector<std::string> columns,
                           std::string key_fmt, std::string val_fmt)
{
    if (columns.size() != 0)
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
                                       ",value_format=" + val_fmt + ",columns=(" +
                                       concat + ")";
        session->create(session, table_name.c_str(), wt_format_string.c_str());
    }
    else
    {
        std::string table_name = "table:" + prefix;
        session->create(session, table_name.c_str(), "key_format=I,value_format=I");
    }
}

/**
 * @brief This function is used to construct the default values based on format 
 * string.  
 * 
 * @param fmt This is the format string. 
 * @return std::vector<int> This is a vector that contains the default values for format types that are represented as ints. (0)
 */
std::vector<int>
CommonUtil::get_default_nonstring_attrs(std::string fmt)
{
    std::vector<int> ret;
    std::string zero_attr = "iIlLqQsShHt";

    for (const char &x : fmt)
    {
        if (zero_attr.find(x) != std::string::npos)
        {
            ret.push_back(0);
        }
        else
        {
            throw GraphException("Found a non-int type in format string. Abort!");
        }
    }
    return ret;
}

/**
 * @brief This function is used to construct the default values based on format 
 * string.  
 * 
 * @param fmt This is the format string. 
 * @return std::vector<std::string> This is a vector that contains the default values for format types that are represented as strings. ("")
 */
std::vector<std::string>
CommonUtil::get_default_string_attrs(std::string fmt)
{
    std::vector<std::string> ret;
    std::string zero_attr = "iIlLqQsShHt";

    for (const char &x : fmt)
    {
        if (zero_attr.find(x) != std::string::npos)
        {
            throw GraphException("Found a non-int type in format string. Abort!");
        }
        else
        {
            ret.push_back("");
        }
    }
    return ret;
}

std::string CommonUtil::get_db_name(std::string prefix, std::string name)
{
    return (prefix + "-" + name);
}

void CommonUtil::check_graph_params(graph_opts params)
{
    std::vector<std::string> missing_params;

    //TODO: this needs to be updated
    if (missing_params.size() > 0)
    {
        std::vector<std::string>::iterator missing_param_ptr;
        std::string to_return = missing_params.at(0);
        for (missing_param_ptr = missing_params.begin() + 1;
             missing_param_ptr < missing_params.end(); missing_param_ptr++)
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
std::string CommonUtil::create_string_format(std::vector<std::string> to_pack,
                                             size_t *total_size)
{
    std::string fmt = "Si"; // The first element of the packed string contains the format string. Second element contains the buffer size needed to unpack this vector

    for (std::string item : to_pack)
    {

        fmt = fmt + 'S';                    //std::to_string(item.length()) + 'S';
        *total_size += (item.length() + 1); //for \0
    }
    *total_size += (fmt.length() + 1 + sizeof(int) + 1); //Add the size of the first element + \0 + size of int + another \0
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

    for (std::vector<int>::iterator iter = to_pack.begin(); iter != to_pack.end();
         ++iter)
    {
        fmt = fmt + 'i';
        *total_size = *total_size + 1;
    }
    return fmt;
}
/**
 * @brief This function is used to pack the vector<string> into the WT
 * compliant packing format that can be stored.
 *
 * @param session The current session to the WT database.
 * @param buffer This is the buffer that will hold the packed contents of the
 * vector.
 * @param size This will hold the size of the final packed buffer.
 * @param std::String fmt this holds the format string needed to pack the buf
 * The first char in this string is S to indicate that the first string being
 * saved to the packing stream is the format string itself.
 */
char *CommonUtil::pack_string_vector_wt(std::vector<std::string> to_pack,
                                        WT_SESSION *session, size_t *size,
                                        std::string *fmt)
{

    size_t _size = 0;
    std::string format = CommonUtil::create_string_format(to_pack, &_size);
    char *buffer = (char *)malloc((_size) * sizeof(char));

    WT_PACK_STREAM *psp;
    wiredtiger_pack_start(session, format.c_str(), buffer, _size, &psp);
    wiredtiger_pack_str(
        psp, format.c_str()); // save the format string. Used in unpacking.
    wiredtiger_pack_int(psp, _size);
    for (std::string x : to_pack)
    {
        wiredtiger_pack_str(psp, x.c_str());
    }
    *fmt = format;
    *size = _size; //_size;

    return buffer;
}

/**
 * @brief This function is the alternative way to pack a string vector.
 * Basically, construct one giant string out of all elements that is delimited
 * by "__". This might be potentially faster than WT. <-- timing tests needed
 * 
 */
std::string CommonUtil::pack_string_vector_std(std::vector<std::string> to_pack,
                                               size_t *size)
{

    size_t _size = 0;
    std::string buffer;
    size_t pos = 0;
    while (pos < to_pack.size() - 1)
    {
        buffer = buffer + to_pack.at(pos) + "__";
        _size = _size + to_pack.at(pos).size() + 2;
        pos++;
    }
    buffer = buffer + to_pack.at(pos);
    _size = _size + to_pack.at(pos).size();
    *size = _size;

    return buffer;
}

std::vector<std::string> CommonUtil::unpack_string_vector_std(std::string packed_str)
{
    std::vector<std::string> res;
    uint pos = 0;
    std::string delimiter = "__";
    std::string token;
    while ((packed_str != "") && ((pos = packed_str.find(delimiter)) != std::string::npos))
    {
        token = packed_str.substr(0, pos);

        res.push_back(token);
        packed_str.erase(0, pos + delimiter.length());
    }
    if ((packed_str != ""))
    {
        res.push_back(packed_str);
    }

    return res;
}
/**
 * @brief This function unpacks the buffer to_unpack into a vector<string>
 * This function is *very* brittle and I fully expect this to fail.
 *
 * @param to_unpack The buffer containing the packed string.
 * @param session The WT_SESSION object
 * @return std::vector<std::string> The unpacked vector of strings.
 */
std::vector<std::string> CommonUtil::unpack_string_vector_wt(const char *to_unpack,
                                                             WT_SESSION *session)
{
    std::vector<std::string> unpacked_vector;
    WT_PACK_STREAM *psp;
    const char *fmt = "Si";
    const char *fmt_new = (char *)malloc(100 * sizeof(char));
    size_t size = 1000; // for lack of something clever
    int64_t _size;
    (void)wiredtiger_unpack_start(session, fmt, to_unpack, size, &psp);
    (void)wiredtiger_unpack_str(psp, &fmt_new);
    (void)wiredtiger_unpack_int(psp, &_size);
    wiredtiger_pack_close(psp, &size); // To reset the unpacking stream.

    (void)wiredtiger_unpack_start(session, fmt_new, to_unpack, _size, &psp);
    size_t fmt_len = strlen(fmt_new);
    for (uint i = 0; i < fmt_len; i++)
    {
        const char *res = (char *)malloc(
            50 * sizeof(char)); //! I don't know a good size to use here and don't
                                //! know how to estimate this either.
        if (i == 1)
        {
            int64_t temp;
            (void)wiredtiger_unpack_int(psp, &temp);
        }
        else
        {
            (void)wiredtiger_unpack_str(psp, &res);
        }
        if (i > 1)
        {
            unpacked_vector.push_back(res);
        }
    }
    return unpacked_vector;
}

/**
 * @brief  This function is used to pack the vector<int> into the WT
 * compliant packing format that can be stored.
 *
 * @param session The current session to the WT database.
 * @param buffer This is the buffer that will hold the packed contents of the
 * vector.
 * @param size This will hold the size of the final packed buffer.
 * @param std::String fmt this holds the format string needed to pack the buf.
 * The first char in this string is S to indicate that the first string being
 * saved to the packing stream is the format string itself.
 */
char *CommonUtil::pack_int_vector_wt(std::vector<int> to_pack, WT_SESSION *session,
                                     size_t *size, std::string *fmt)
{

    size_t _size = 0;
    std::string format = CommonUtil::create_intvec_format(to_pack, &_size);

    char *buffer = (char *)malloc((_size) * sizeof(int));
    // TODO ^ this is a potential problem. Ask Keith.
    WT_PACK_STREAM *psp;

    (void)wiredtiger_pack_start(session, format.c_str(), buffer, *size, &psp);
    wiredtiger_pack_str(
        psp, format.c_str()); // save the format string. Used in unpacking.
    for (int x : to_pack)
    {
        (void)wiredtiger_pack_int(psp, x);
    }
    *fmt = format;
    *size = _size;
    return buffer;
}

/**
 * @brief This function is used to pack all integers in the integer vector by
 * concatenating their string representation that is delimited with "__"
 *
 * @param to_unpack The vector of ints to pack.
 * @return buffer the packed string.
 */
std::string CommonUtil::pack_int_vector_std(std::vector<int> to_pack, size_t
                                                                          *size)
{
    size_t _size = 0;
    std::string buffer;
    uint pos = 0;
    if (to_pack.size() == 0)
    {
        *size = _size;
        return buffer;
    }
    while (pos < to_pack.size() - 1)
    {
        buffer = buffer + std::to_string(to_pack.at(pos)) + "__";
        _size = 2; // add the 2 * number of "__" added
        pos++;
    }
    buffer = buffer + std::to_string(to_pack.at(pos));
    _size = _size + sizeof(int) * to_pack.size();
    *size = _size;

    return buffer;
}

/**
 * @brief This function unpacks the buffer into a vector<int>. This assumes the
 * buffer was packed using pack_int_vector_std()
 * @param to_unpack string that contains the packed vector
 * @return std::vector<int> unpacked buffer
 */
std::vector<int> CommonUtil::unpack_int_vector_std(std::string packed_str)
{
    std::vector<int> res;
    uint pos = 0;
    std::string delimiter = "__";
    int number;
    if (packed_str == " ")
    {
        return res;
    }
    while ((packed_str != "") && (pos = packed_str.find(delimiter)) != std::string::npos)
    {
        number = stoi(packed_str.substr(0, pos));

        res.push_back(number);
        packed_str.erase(0, pos + delimiter.length());
    }
    if (packed_str != "")
    {
        res.push_back(stoi(packed_str));
    }

    return res;
}

/**
 * @brief This function unpacks the buffer into a vector<int>. This is a very
 * brittle implementation, and the sizes of the buffers are random and will
 * probably break in production. //! THIS NEEDS REVIEWING AND FIXING
 *
 * @param to_unpack buffer that contains the packed vector
 * @param session WT_SESSION object
 * @return std::vector<int> unpacked buffer
 */
std::vector<int> CommonUtil::unpack_int_vector_wt(const char *to_unpack, WT_SESSION *session)
{
    std::vector<int> unpacked_vector;
    WT_PACK_STREAM *psp;
    const char *fmt = "S";
    size_t size = 1000; // for lack of something clever
    (void)wiredtiger_unpack_start(session, fmt, to_unpack, size, &psp);
    const char *format_str = (char *)malloc(size * sizeof(char));
    (void)wiredtiger_unpack_str(psp, &format_str);
    wiredtiger_pack_close(psp, &size); // To reset the unpacking stream.

    size = 1000;
    (void)wiredtiger_unpack_start(session, format_str, to_unpack, size, &psp);
    size_t fmt_len = strlen(format_str);
    for (uint i = 0; i < fmt_len; i++)
    {
        if (i == 0)
        {
            const char *ret_fmt = (char *)malloc(fmt_len * sizeof(fmt_len));
            (void)wiredtiger_unpack_str(psp, &ret_fmt);
            continue;
        }
        int64_t res;
        (void)wiredtiger_unpack_int(psp, &res);
        unpacked_vector.push_back(res);
    }
    return unpacked_vector;
}

char *CommonUtil::pack_string_wt(std::string to_pack, WT_SESSION *session,
                                 std::string *fmt)
{
    // std::string format = std::to_string(to_pack.length()) + "s";
    // ? Fix this if things go wrong. Does packing a string require the length? I
    // //? don't think so.
    std::string format = "S";
    size_t size;
    wiredtiger_struct_size(session, &size, "S", to_pack.c_str());
    //std::cout << "\n size needed is : " << size << std::endl;

    char *buffer = (char *)malloc(size);

    WT_PACK_STREAM *psp;

    (void)wiredtiger_pack_start(session, format.c_str(), buffer,
                                size, &psp);

    (void)wiredtiger_pack_str(psp, to_pack.c_str());

    *fmt = format;
    return buffer;
}
/**
 * @brief This function unpacks the string stored in the buffer and returns a
 * string object.
 *
 * @param to_unpack the buffer that needs to be unpacked
 * @param session WT_SESSION object
 * @return std::string the std::string object containing the unpacked string.
 */
std::string CommonUtil::unpack_string_wt(const char *to_unpack,
                                         WT_SESSION *session)
{
    const char *fmt = "S";
    const char *buffer =
        (char *)malloc(50 * sizeof(char)); // ! Fix this Again, random guess of buffer size

    WT_PACK_STREAM *ps;

    (void)wiredtiger_unpack_start(session, fmt, to_unpack, 50, &ps);

    (void)wiredtiger_unpack_str(ps, &buffer);

    return std::string(buffer);
}

/**
 * @brief Pack the integer into a char buffer
 *
 * @param to_pack int to be packed
 * @param session the WT_SESSION object
 * @return char* packed buffer
 */
char *CommonUtil::pack_int_wt(int to_pack, WT_SESSION *session)
{
    std::string format = "i";

    char *buffer = (char *)malloc(sizeof(int));

    WT_PACK_STREAM *psp;

    (void)wiredtiger_pack_start(session, format.c_str(), buffer, sizeof(int), &psp);

    (void)wiredtiger_pack_int(psp, to_pack);
    return buffer;
}

/**
 * @brief unpack a packed buffer to retrieve the stored int64_t value
 *
 * @param to_unpack the packed char* buffer
 * @param session WT_SESSION object
 * @return int the unpacked integer
 */
int CommonUtil::unpack_int_wt(const char *to_unpack, WT_SESSION *session)
{
    int64_t ret_val;
    WT_PACK_STREAM *ps;

    (void)wiredtiger_unpack_start(session, "i", to_unpack, sizeof(int), &ps);
    (void)wiredtiger_unpack_int(ps, &ret_val);
    return ret_val;
}

/**
 * @brief Pack a bool value into a char buffer. I'm treating bools as
 * uint64_t.
 *
 * @param to_pack bool value to be packed.
 * @param session WT_SESSION object
 * @param fmt format string
 * @return char* packed buffer.
 */
char *CommonUtil::pack_bool_wt(bool to_pack, WT_SESSION *session,
                               std::string *fmt)
{
    size_t size = sizeof(uint8_t);
    char *buffer = (char *)malloc(size);
    WT_PACK_STREAM *psp;

    (void)wiredtiger_pack_start(session, "B", buffer, size, &psp);

    if (to_pack)
    {
        (void)wiredtiger_pack_uint(psp, 1);
    }
    else
    {
        (void)wiredtiger_pack_uint(psp, 0);
    }

    *fmt = "i";
    return buffer;
}

/**
 * @brief Unpack the packed buffer to extract the boolean value
 *
 * @param to_unpack char* of the packed buffer
 * @param session WT_SESSION object
 * @return bool the boolean value unpacked from the buffer.
 */
bool CommonUtil::unpack_bool_wt(const char *to_unpack, WT_SESSION *session)
{

    uint64_t bool_val;
    WT_PACK_STREAM *ps;

    (void)wiredtiger_unpack_start(session, "i", to_unpack, sizeof(uint64_t), &ps);
    (void)wiredtiger_unpack_uint(ps, &bool_val);

    if (bool_val == 0)
    {
        return false;
    }
    else if (bool_val == 1)
    {
        return true;
    }
    else
    {
        fprintf(stderr, "failed to unpack bool");
        exit(-1);
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

int CommonUtil::open_connection(char *db_name, WT_CONNECTION **conn)
{
    if (wiredtiger_open(db_name, NULL, "create", conn) != 0)
    {
        fprintf(stderr, "Failed to open connection\n");
        return (-1);
    }
    return 0;
}

int CommonUtil::open_session(WT_CONNECTION *conn, WT_SESSION **session)
{
    if (conn->open_session(conn, NULL, NULL, session) != 0)
    {
        fprintf(stderr, "Failed to open session\n");
        return (-1);
    }
    return 0;
}

int CommonUtil::open_cursor(WT_SESSION *session, WT_CURSOR **cursor, std::string uri,
                            WT_CURSOR *to_dup, std::string config)
{
    if (session->open_cursor(session, uri.c_str(), to_dup, config.c_str(),
                             cursor) != 0)
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
    std::cout << "Edge ID is: \t" << to_print.id << std::endl;
    std::cout << "SRC id is:\t" << to_print.src_id << std::endl;
    std::cout << "DST id is:\t" << to_print.dst_id << std::endl;
    std::cout << "Weight is:\t" << to_print.edge_weight << std::endl;
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

/*
 * The following functions are used to pack values using the packing stream
 * interface and returns a WT_ITEM.
 * Do not use before testing it a LOT
 */

/**
 * @brief This function is used to pack a vector of strings into a buffer held
 * by in WT_ITEM buffer. 
 * 
 * @param session session to WT DB
 * @param to_pack the vector to pack
 * @param fmt this is a string* pointer that is set in this function to hold the
 * format descriptor of the vector.
 * @return WT_ITEM 
 */
WT_ITEM CommonUtil::pack_vector_string(WT_SESSION *session, std::vector<std::string> to_pack, std::string *fmt)
{
    size_t size;
    *fmt = CommonUtil::create_string_format(to_pack, &size);
    WT_ITEM packed;
    char *data_buf = (char *)malloc(size * sizeof(char));

    WT_PACK_STREAM *pack_stream;
    wiredtiger_pack_start(session, fmt->c_str(), data_buf, size, &pack_stream);
    for (std::string str : to_pack)
    {
        wiredtiger_pack_str(pack_stream, str.c_str());
    }
    wiredtiger_pack_close(pack_stream, &size); // get the bytes used.

    packed.data = data_buf;
    packed.size = size;
    return packed;
}

/**
 * @brief Pack a vector of integers into a buffer held in WT_ITEM
 * 
 * @param session session to the WT DB
 * @param to_pack the vector to pack
 * @param fmt format string will hold the vector description. This is set from this function.
 * @return WT_ITEM 
 */
WT_ITEM CommonUtil::pack_vector_int(WT_SESSION *session, std::vector<int> to_pack, std::string *fmt)
{
    size_t size = 0;
    std::string format = CommonUtil::create_intvec_format(to_pack, &size);
    format.erase(0, 1);
    WT_ITEM packed;
    char *data_buf = (char *)calloc(size, 2 * sizeof(int));

    WT_PACK_STREAM *pack_stream;
    wiredtiger_pack_start(session, format.c_str(), data_buf, size, &pack_stream);
    for (int item : to_pack)
    {
        wiredtiger_pack_int(pack_stream, item);
    }
    size_t used = 0;
    wiredtiger_pack_close(pack_stream, &used);
    packed.data = data_buf;
    packed.size = size;
    std::cout << "size of the packed buffer is " << packed.size << std::endl;
    *fmt = format;
    return packed;
}

/**
 * @brief This function accepts a variadic input to wrap into a WT_ITEM buffer. 
 * 
 * @param session session to the WT DB
 * @param fmt the format string that describes the variadic input
 * @param ... All the items that need to be packed. Strings should be passed as char*
 * @return WT_ITEM The struct containing the packed buffer. 
 */
WT_ITEM CommonUtil::pack_items(WT_SESSION *session, std::string fmt, ...)
{
    size_t size = 0; //!This will cause error in malloc for databuf
    WT_ITEM packed;
    char *data_buf = (char *)malloc(size * sizeof(char));
    const char *format = fmt.c_str();

    WT_PACK_STREAM *pack_stream;
    wiredtiger_pack_start(session, format, data_buf, size, &pack_stream);

    va_list args;
    va_start(args, format);
    while (*format != '\0')
    {
        if (*format == 'i' || *format == 'I')
        {
            int val = va_arg(args, int);
            wiredtiger_pack_int(pack_stream, val);
        }
        else if (*format == 'S' || *format == 's')
        {
            char *val = va_arg(args, char *);
            wiredtiger_pack_str(pack_stream, val);
        }
    }
    wiredtiger_pack_close(pack_stream, &size); // get the bytes used.

    packed.data = data_buf;
    packed.size = size;
    return packed;
}

std::vector<int> CommonUtil::unpack_vector_int(WT_SESSION *session, WT_ITEM packed, std::string format)
{
    std::vector<int> unpacked;
    WT_PACK_STREAM *pack_stream;

    (void)wiredtiger_unpack_start(session, format.c_str(), packed.data, packed.size, &pack_stream);
    for (int i = 0; i < format.length(); i++)
    {
        int64_t found;
        wiredtiger_unpack_int(pack_stream, &found);
        unpacked.push_back(found);
    }
    return unpacked;
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
std::vector<int> CommonUtil::unpack_int_vector_wti(WT_SESSION *session, size_t size, char *packed_str)
{
    WT_PACK_STREAM *psp;
    WT_ITEM unpacked;
    size_t used;
    wiredtiger_unpack_start(session, "u", packed_str, size, &psp);
    wiredtiger_unpack_item(psp, &unpacked);
    wiredtiger_pack_close(psp, &used);
    std::vector<int> unpacked_vec;

    int vec_size = (int)size / sizeof(int);
    for (int i = 0; i < vec_size; i++)
        unpacked_vec.push_back(((int *)unpacked.data)[i]);
    return unpacked_vec;
}

/**
 * @brief This function is used to pack all integers in the integer vector by
 * using the wiredtiger packing stream interface. There's nothing to it, really:
 * This packs it into a malloc'd buffer and sets the size variable passed as argument.
 *
 * @param session The WiredTiger Session variable
 * @param to_pack The vector of ints to pack.
 * @param size pointer to a size_t variable to store the size of buffer. THIS IS NEEDED TO UNPACK -> STORE THIS IN THE TABLE
 * @return buffer containing the packed array.
 */
char *CommonUtil::pack_int_vector_wti(WT_SESSION *session, std::vector<int> to_pack, size_t *size)
{
    WT_PACK_STREAM *psp;
    WT_ITEM item;
    item.data = to_pack.data();
    item.size = sizeof(int) * to_pack.size();

    void *pack_buf = malloc(sizeof(int) * to_pack.size());
    int ret = wiredtiger_pack_start(session, "u", pack_buf,
                                    item.size, &psp);

    wiredtiger_pack_item(psp, &item);
    wiredtiger_pack_close(psp, size);

    return (char *)pack_buf;
}
