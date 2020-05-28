#ifndef BASE_COMMON
#define BASE_COMMON

#include <wiredtiger.h>
#include <unordered_map>
#include <vector>
#define METADATA "metadata"
#define DB_NAME "db_name"
#define NODE_COLUMNS  "node_columns"
#define EDGE_COLUMNS  "edge_columns"
#define HAS_NODE_ATTR  "has_node_attr"
#define HAS_EDGE_ATTR  "has_edge_attr"
#define NODE_VALUE_COLUMNS  "node_value_columns"
#define EDGE_VALUE_COLUMNS  "edge_value_columns"
#define NODE_VALUE_FORMAT  "node_value_format"
#define EDGE_VALUE_FORMAT  "edge_value_format"
#define READ_OPTIMIZE  "read_optimize"
#define EDGE_ID  "edge_id"
#define NODE_DATA  "data"
#define IS_DIRECTED  "is_directed"
#define CREATE_NEW  "create_new"

//Read Optimize columns
#define IN_DEGREE  "in_degree"
#define OUT_DEGREE  "out_degree"

//Shared column names
const std::string SRC =  "src";
const std::string DST =  "dst";
const std::string ID  = "id";
const std::string NODE_TABLE = "node";
const std::string EDGE_TABLE = "edge";
const std::string SRC_INDEX = "IX_edge_" + SRC;
const std::string DST_INDEX = "IX_edge_" + DST;
const std::string SRC_DST_INDEX = "IX_edge_" + SRC + DST;

class CommonUtil{
    public:
            static void set_table(WT_SESSION *session, std::string prefix, std::vector<std::string> columns, std::string key_fmt, std::string val_fmt);
            std::string get_db_name(std::string prefix, std::string name);
            static void check_graph_params(std::unordered_map<std::string, std::vector<std::string>> params);
};


#endif