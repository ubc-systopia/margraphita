#ifndef BASE_COMMON
#define BASE_COMMON

#include <wiredtiger.h>
#include <unordered_map>
#include <vector>


extern const std::string METADATA;
extern const std::string DB_NAME;
extern const std::string NODE_COLUMNS;
extern const std::string EDGE_COLUMNS;
extern const std::string HAS_NODE_ATTR;
extern const std::string HAS_EDGE_ATTR;
extern const std::string NODE_VALUE_COLUMNS;
extern const std::string EDGE_VALUE_COLUMNS;
extern const std::string NODE_VALUE_FORMAT;
extern const std::string EDGE_VALUE_FORMAT;
extern const std::string READ_OPTIMIZE;
extern const std::string EDGE_ID;
extern const std::string NODE_DATA;
extern const std::string IS_DIRECTED;
extern const std::string CREATE_NEW;

//Read Optimize columns
extern const std::string IN_DEGREE;
extern const std::string OUT_DEGREE;

//Shared column names
extern const std::string SRC;
extern const std::string DST;
extern const std::string ID;
extern const std::string NODE_TABLE;
extern const std::string EDGE_TABLE;
extern const std::string SRC_INDEX;
extern const std::string DST_INDEX;
extern const std::string SRC_DST_INDEX;


class CommonUtil{
    public:
        static void set_table(WT_SESSION *session, std::string prefix, std::vector<std::string> columns, std::string key_fmt, std::string val_fmt);
        static std::string get_db_name(std::string prefix, std::string name);
        static void check_graph_params(std::unordered_map<std::string, std::vector<std::string>> params);
        static std::string create_string_format(std::vector<std::string> to_pack);
        
        //WT Session and Cursor wrangling operations
        static int open_cursor(WT_CURSOR **cursor);
        static int close_cursor(WT_CURSOR *cursor);
        static int close_session(WT_SESSION *session);
        static int close_connection(WT_CONNECTION *conn);
        static int open_connection(char *db_name, WT_CONNECTION **conn);
        static int open_session(WT_CONNECTION *conn, WT_SESSION **session);
};

#endif