#ifndef ADJ_COMMON
#define ADJ_COMMON

#include "common.h"
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <wiredtiger.h>

//These are the string constants
extern const std::string METADATA;
extern const std::string DB_NAME;
extern const std::string IS_WEIGHTED;
extern const std::string READ_OPTIMIZE;
extern const std::string IS_DIRECTED;
extern const std::string CREATE_NEW;

// Read Optimize columns
extern const std::string IN_DEGREE;
extern const std::string OUT_DEGREE;

// Shared column names
extern const std::string SRC;
extern const std::string DST;
extern const std::string ID;
extern const std::string WEIGHT;
extern const std::string NODE_TABLE;
extern const std::string EDGE_TABLE;
extern const std::string ADJ_INLIST_TABLE;  // New
extern const std::string ADJ_OUTLIST_TABLE; // New
extern const std::string SRC_INDEX;         //!APT CHECK
extern const std::string DST_INDEX;         //!APT CHECK
extern const std::string SRC_DST_INDEX;     //!APT CHECK

struct opt_args
{
    bool create_new = true;
    bool read_optimize = true;
    bool is_directed = true;
    bool is_weighted = false;
    std::string db_name;
    bool optimize_create; // directs when the index should be created //!APT CHECK
};

typedef struct node
{
    uint32_t id; // node ID
    uint32_t in_degree = 0;
    uint32_t out_degree = 0;
} node;

typedef struct edge
{
    int src_id;
    int dst_id;
    int edge_weight;
} edge;

typedef struct adjlist
{
    int node_id;
    int degree;
    vector<int> edgelist;
} adjlist;

// typedef struct adjlist_in_edges
// {
//     int node_id;
//     int degree;
//     vector<int> in_edges;
// } adj_outlist;

typedef struct edge_index
{
    int src_id;
    int dst_id;
} edge_index;

#endif