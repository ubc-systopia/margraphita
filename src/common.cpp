#include<iostream>
#include<string>
#include <wiredtiger.h>
#include "common.h"
#include "graph_exception.h"


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