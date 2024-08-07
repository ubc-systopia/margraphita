#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <string>
#include <vector>
#define MAKE_EKEY(x) ((x) + 1)
#define OG_KEY(x) ((x)-1)

// These are the constants
typedef enum MetadataKey
{
    db_name,
    db_dir,
    is_weighted,
    read_optimize,
    is_directed,
    num_nodes,
    num_edges,
    max_node_id,
    min_node_id
} MetadataKey;

const std::string MetadataKeyNames[9] = {"db_name",
                                         "db_dir",
                                         "is_weighted",
                                         "read_optimize",
                                         "is_directed",
                                         "num_nodes",
                                         "num_edges",
                                         "max_node_id",
                                         "min_node_id"};

const std::string METADATA = "metadata";
// Read Optimize columns
const std::string IN_DEGREE = "in_degree";
const std::string OUT_DEGREE = "out_degree";

// Shared column names
const std::string SRC = "src";
const std::string DST = "dst";
const std::string ID = "id";
const std::string ATTR_FIRST =
    "attr_fst";  // Used in EdgeKey as the first attribute.
const std::string ATTR_SECOND =
    "attr_scnd";  // Used in EdgeKey as the second attribute.
const std::string WEIGHT = "weight";
const std::string NODE_TABLE = "node";
const std::string EDGE_TABLE = "edge";
const std::string SRC_INDEX = "IX_edge_" + SRC;
const std::string DST_INDEX = "IX_edge_" + DST;
const std::string SRC_DST_INDEX = "IX_edge_" + SRC + DST;
const std::string DST_SRC_INDEX = "IX_edge_" + DST + SRC;
// specific to AdjList implementation
const std::string OUT_ADJLIST = "adjlistout";
const std::string IN_ADJLIST = "adjlistin";
// specific to EdgeKeySplit implementation
const std::string OUT_EDGES = "edge_out";
const std::string IN_EDGES = "edge_in";
const std::string node_count = "nNodes";
const std::string edge_count = "nEdges";

#ifdef B64
typedef uint64_t node_id_t;
typedef uint64_t edge_id_t;
#else
typedef uint32_t node_id_t;
typedef uint32_t edge_id_t;
#endif
typedef int32_t edgeweight_t;
typedef uint32_t degree_t;

/// @brief EdgeKey specific definitions
const node_id_t OutOfBand_ID_MIN =
    0;  // Used to be -1. Changed to 0 to avoid issues with unsigned types.
#ifdef B64
const node_id_t OutOfBand_ID_MAX = UINT64_MAX;
#else
const degree_t OutOfBand_ID_MAX = UINT32_MAX;
#endif

typedef enum GraphType
{
    Std,
    Adj,
    EKey,
    SplitEKey,
    META
} GraphType;

struct graph_opts
{
    bool create_new = false;
    bool read_optimize = false;
    bool is_directed = false;
    bool is_weighted = false;
    std::string db_name;
    std::string db_dir;
    bool optimize_create = false;  // directs when the index should be created
    std::string conn_config;
    std::string stat_log;
    GraphType type;
    node_id_t num_nodes;
    uint64_t num_edges;  // we can have > 4B edges
    std::string dataset;
    int num_threads = 1;
    ~graph_opts() = default;
};

typedef struct node
{
    node_id_t id = 0;  // node ID
    degree_t in_degree = 0;
    degree_t out_degree = 0;

} node;

typedef struct edge
{
    edge_id_t id = 0;
    node_id_t src_id = 0;
    node_id_t dst_id = 0;
    edgeweight_t edge_weight = 0;
} edge;

typedef struct edge_index
{
    node_id_t src_id = 0;
    node_id_t dst_id = 0;
    edge_index() : src_id(0), dst_id(0) {}
    edge_index(node_id_t a, node_id_t b) : src_id(a), dst_id(b) {}

} edge_index;

typedef struct edge_index key_pair;

typedef struct key_range
{
    node_id_t start;
    node_id_t end;
    key_range() : start(), end() {}
    key_range(node_id_t a, node_id_t b) : start(a), end(b) {}
} key_range;

typedef key_range node_range;

typedef struct edge_range
{
    key_pair start{};
    key_pair end{};
    edge_range() : start(), end() {}
    edge_range(key_pair a, key_pair b) : start(a), end(b) {}
} edge_range;

typedef struct adjlist
{
    node_id_t node_id{};
    degree_t degree{};
    std::vector<node_id_t> edgelist;
    // This could be dynamic, but this is a good starting point.
    adjlist() { edgelist.reserve(1000); }
    adjlist(node_id_t id, degree_t deg) : node_id(id), degree(deg)
    {
        edgelist.reserve(deg);
    }
    void clear()
    {
        edgelist.clear();
        node_id = 0;
        degree = 0;
    }
} adjlist;
#endif