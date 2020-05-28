#include <wiredtiger.h>
#include <iostream>
#include<filesystem>
#include <string>
#include <unordered_map>
#include "common.h"
#include "graph_exception.h"

using namespace std;
const std::string GRAPH_PREFIX = "std";

class StandardGraph
{
public:
    bool create_new = true;
    bool read_optimize = true;
    bool is_directed = true;
    vector<string> node_value_cols;
    int node_attr_size = 0; //set on checking the list len
    string node_value_format;

    vector<string> edge_table_columns;
    vector<string> edge_columns;
    vector<string> edge_value_cols;
    bool has_edge_attrs;
    string edge_value_format;
    string node_key_format;
    string db_name;

    StandardGraph(bool create_new, bool read_optimize, bool is_directed,
                  std::string db_name,
                  std::unordered_map<string, vector<string>>);

    // WT privates
private:
    WT_CONNECTION *conn;
    WT_CURSOR *cursor;
    WT_SESSION *session;
    int edge_id;
    vector<string> node_columns;

    // Basic WT session management funcs
private:
    int open_connection(char*  db_name, WT_CONNECTION ** conn);
    int close_connection(WT_CONNECTION *conn);
    int open_cursor(WT_CURSOR **);
    int close_cursor(WT_CURSOR *);
    int open_session(WT_CONNECTION * conn, WT_SESSION ** session );
    int close_session(WT_SESSION *);

    // Now to create and delete the graph
    void create_new_graph();
    void insert_metadata(string col_name, string fmt, vector<string> );
};

StandardGraph::StandardGraph(bool create_new, bool read_optimize,
                             bool is_directed, std::string db_name,
                             std::unordered_map<std::string, vector<string>> params)

{   
    int ret; //general purpose hardy error int
    this->create_new = create_new;
    this->read_optimize = read_optimize;
    this->is_directed = is_directed;
    this->db_name = db_name;
    
    //Check which params are missing.
    try{
        CommonUtil::check_graph_params(params);
    }catch(GraphException G){
        std::cout << G.what() <<std::endl;
    }

    auto find = params.find(NODE_VALUE_COLUMNS);
    {
        if (find != params.end()){
            node_value_cols = find->second;
        }
    } 
    
    find = params.find(NODE_VALUE_FORMAT);
    {
        if (find != params.end()){
            node_value_format = find->second.front();
        }
    }

      find = params.find(EDGE_VALUE_COLUMNS);
    {
        if (find != params.end()){
            edge_value_cols = find->second;
        }
    }
    
    find = params.find(EDGE_VALUE_FORMAT);
    {
        if (find != params.end()){
            edge_value_format = find->second.front();
        }
    }

    //Create new directory for WT DB
    std::filesystem::path dirname = "./db/" + db_name;
    if(std::filesystem::exists(dirname)){
        filesystem::remove_all(dirname); //remove if exists;
    }
    std::filesystem::create_directory(dirname);
 
    //open connection to WT
    if( open_connection((char *) dirname.c_str(), &conn) < 0){
        exit (-1);
    };

    //Open a session handle for the database
    if(open_session(conn, &session) !=0 ){
        exit(-1);
    }

    //Initialize edge ID for the edge table
    edge_id = 1; 

    //Set up the node table
    node_columns = {ID};
    node_key_format = 'I';

    //Node Column Format: <attributes><data0><data1><in_degree><out_degree>
    if( ! node_value_cols.empty() ){
        node_attr_size = node_value_format.length(); //0 if the list len=0
        node_columns.insert(node_columns.end(), node_value_cols.begin(), node_value_cols.end());
    }
    //Add Data Column (packign fmt='I', int)
    node_columns.push_back(NODE_DATA + string("0"));
    node_columns.push_back(NODE_DATA + string("1"));
    node_value_format += "SS";

    //If the graph is read_optimized, add columns and format for in/out degrees
    if (read_optimize){
        vector<string> READ_OPTIMIZE_COLS  = {IN_DEGREE, OUT_DEGREE};
        node_columns.insert(node_columns.end(), READ_OPTIMIZE_COLS.begin(), node_value_cols.end());
        node_value_format += "II";
    }
    
    //Now Create the Node Table
    CommonUtil::set_table(session,NODE_TABLE, node_columns,node_key_format,node_value_format);

    //Now set up the Edge Table
    //Edge Column Format : <src><dst><attrs>
    edge_columns = {SRC,DST};
    string edge_key_format = "I";
    if(edge_value_cols.size() == 0){
        edge_value_cols.push_back("n/a");
        edge_value_format +="I";
        has_edge_attrs = false;
    }else{
        edge_columns.insert(edge_columns.end(), edge_value_cols.begin(), edge_value_cols.end());
        edge_value_format += "I";
        has_edge_attrs = true;        
    }
    edge_value_format +="II";
    edge_table_columns = {ID};
    edge_table_columns.insert(edge_table_columns.end(),edge_columns.begin(), edge_columns.end());

    //Create edge table
    CommonUtil::set_table(session, EDGE_TABLE, edge_table_columns, edge_key_format, edge_value_format);
    
    //Create table index on src, dst
    string edge_table_index_str, edge_table_index_conf_str;
    edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_DST_INDEX;
    edge_table_index_conf_str = "columns=("+ SRC + "," + DST + ")"; 
    if((ret = session -> create(session, edge_table_index_str.c_str(), edge_table_index_conf_str.c_str())) > 0){
        cerr<<"Failed to create index SRC_DST_INDEX in the edge table" << cursor->session->strerror(cursor->session, ret)<<endl;
    }
    //Create index on SRC column
    edge_table_index_str = "index:" + EDGE_TABLE + ":" + SRC_INDEX;
    edge_table_index_conf_str = "columns=(" + SRC + ")";
    if((ret = session -> create(session, edge_table_index_str.c_str(), edge_table_index_conf_str.c_str())) > 0){
        cerr<<"Failed to create index SRC_INDEX in the edge table" << cursor->session->strerror(cursor->session, ret)<<endl;
    }
    //Create index on DST column
    edge_table_index_str = "index:" + EDGE_TABLE + ":" + DST_INDEX;
    edge_table_index_conf_str = "columns=(" + DST + ")";
    if((ret = session -> create(session, edge_table_index_str.c_str(), edge_table_index_conf_str.c_str())) > 0){
        cerr<<"Failed to create index DST_INDEX in the edge table" << cursor->session->strerror(cursor->session, ret)<<endl;
    }

    

}

//void StandardGraph::insert_metadata(string col_name, string fmt,   )

int StandardGraph::open_connection(char*  db_name, WT_CONNECTION ** conn){
    if( wiredtiger_open(db_name, NULL, "create", conn) != 0 ){
        fprintf(stderr, "Failed to open connection\n");
       return (-1);
    }
}

int StandardGraph::open_session(WT_CONNECTION * conn, WT_SESSION ** session){
    if (conn->open_session(conn, NULL, NULL, session) != 0){
        fprintf(stderr, "Failed to open session\n");
        return (-1);
    }
}

int StandardGraph::close_connection(WT_CONNECTION * conn){
    if (conn->close(conn, NULL) != 0){
        fprintf(stderr, "Failed to close connection\n");
        return (-1);
    }
}

int StandardGraph::close_session (WT_SESSION * session){
    if (session -> close (session, NULL) != 0){
        fprintf(stderr, "Failed to close session\n");
        return (-1);
    }
}
