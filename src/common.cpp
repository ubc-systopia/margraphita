#include<iostream>
#include<string>
#include <wiredtiger.h>
#include "common.h"
#include "graph_exception.h"


const std::string METADATA  = "metadata";
const std::string DB_NAME = "db_name";
const std::string NODE_COLUMNS = "node_columns";
const std::string EDGE_COLUMNS = "edge_columns";
const std::string HAS_NODE_ATTR = "has_node_attr";
const std::string HAS_EDGE_ATTR = "has_edge_attr";
const std::string NODE_VALUE_COLUMNS = "node_value_columns";
const std::string EDGE_VALUE_COLUMNS = "edge_value_columns";
const std::string NODE_VALUE_FORMAT = "node_value_format";
const std::string EDGE_VALUE_FORMAT = "edge_value_format";
const std::string READ_OPTIMIZE = "read_optimize";
const std::string EDGE_ID = "edge_id";
const std::string NODE_DATA  ="data";
const std::string IS_DIRECTED = "is_directed";
const std::string CREATE_NEW  = "create_new";

//Read Optimize columns
const std::string IN_DEGREE = "in_degree";
const std::string OUT_DEGREE  = "out_degree";

//Shared column names
const std::string SRC =  "src";
const std::string DST =  "dst";
const std::string ID  = "id";
const std::string NODE_TABLE = "node";
const std::string EDGE_TABLE = "edge";
const std::string SRC_INDEX = "IX_edge_" + SRC;
const std::string DST_INDEX = "IX_edge_" + DST;
const std::string SRC_DST_INDEX = "IX_edge_" + SRC + DST;


void CommonUtil::set_table(WT_SESSION *session, std::string prefix, std::vector<std::string> columns, std::string key_fmt, std::string val_fmt){
    if(columns.size() !=0){
        std::vector<std::string>::iterator ptr;
        std::string concat = columns.at(0);
        for (ptr = columns.begin()+1; ptr < columns.end(); ptr++ ){
            concat += "," + *ptr;
        }
        //Now insert in WT
        std::string table_name  = "table:" + prefix;
        std::string wt_format_string = "key_format=" + key_fmt + "value_format=" + val_fmt + "columns=("+ concat +")";
        session->create(session, table_name.c_str(), wt_format_string.c_str());
    }else{
        std::string table_name  = "table:" + prefix; 
        session->create(session, table_name.c_str(), "key_format=I,value_format=I");
    }

}

 std::string CommonUtil::get_db_name(std::string prefix, std::string name){
    return (prefix + "-" + name);
}

 void CommonUtil::check_graph_params(std::unordered_map<std::string, std::vector<std::string>> params){
    std::vector<std::string> missing_params;
    
    auto find = params.find(NODE_VALUE_COLUMNS);
    {
        if (find == params.end()){
            missing_params.push_back(NODE_VALUE_COLUMNS);
        }
    } 
    find = params.find(NODE_VALUE_FORMAT);
    {
        if (find == params.end()){
            missing_params.push_back(NODE_VALUE_FORMAT);
        }
    }

    find = params.find(EDGE_VALUE_COLUMNS);
    {
        if (find == params.end()){
            missing_params.push_back(EDGE_VALUE_COLUMNS);
        }
    }

    find = params.find(EDGE_VALUE_FORMAT);
    {
        if (find == params.end()){
            missing_params.push_back(EDGE_VALUE_FORMAT);
        }
    }

    if(missing_params.size() > 0){
        std::vector<std::string>::iterator missing_param_ptr;
        std::string to_return = missing_params.at(0);
        for(missing_param_ptr = missing_params.begin()+1; missing_param_ptr < missing_params.end(); missing_param_ptr++)
        {
            to_return += "," + *missing_param_ptr;
        }
        throw GraphException(to_return);
    }

}

std::string CommonUtil::create_string_format(std::vector<std::string> to_pack){
    std::string fmt;

    for (std::vector<std::string>::iterator iter = to_pack.begin(); iter != to_pack.end(); ++ iter){
        std::string item = *iter;
        fmt = fmt + std::to_string(item.length()) + 'S';
    }
    return fmt;
}

int CommonUtil::close_cursor(WT_CURSOR *cursor) {
  if (int ret = cursor->close(cursor) != 0) {
    fprintf(stderr, "Failed to close the cursor\n ");
    return ret;
  }
}

int CommonUtil::close_session(WT_SESSION *session) {
  if (session->close(session, NULL) != 0) {
    fprintf(stderr, "Failed to close session\n");
    return (-1);
  }
}

int CommonUtil::close_connection(WT_CONNECTION *conn) {
  if (conn->close(conn, NULL) != 0) {
    fprintf(stderr, "Failed to close connection\n");
    return (-1);
  }
}

int CommonUtil::open_connection(char *db_name, WT_CONNECTION **conn) {
  if (wiredtiger_open(db_name, NULL, "create", conn) != 0) {
    fprintf(stderr, "Failed to open connection\n");
    return (-1);
  }
  return 0;
}

int CommonUtil::open_session(WT_CONNECTION *conn, WT_SESSION **session) {
  if (conn->open_session(conn, NULL, NULL, session) != 0) {
    fprintf(stderr, "Failed to open session\n");
    return (-1);
  }
  return 0;
}