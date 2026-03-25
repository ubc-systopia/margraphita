#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#define MAKE_EKEY(x) ((x) + 1)
#define OG_KEY(x) ((x) - 1)

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
const std::string ATTR = "attr";  // Used in EdgeKey as the first attribute.
const std::string ATTR_SECOND =
    "attr_scnd";  // Used in EdgeKey as the second attribute.
const std::string NODE_TABLE = "node";
const std::string EDGE_TABLE = "edge";
[[maybe_unused]] const std::string DST_SRC_INDEX = "IX_edge_" + DST + SRC;
// specific to AdjList implementation
const std::string OUT_ADJLIST = "adjlistout";
const std::string IN_ADJLIST = "adjlistin";
// specific to EdgeKeySplit implementation
const std::string OUT_EDGES = "edge_out";
const std::string IN_EDGES = "edge_in";

#ifdef B64
typedef uint64_t node_id_t;
typedef uint64_t edge_id_t;
#else
typedef uint32_t node_id_t;
typedef uint32_t edge_id_t;
#endif
typedef double edgeweight_t;
typedef uint32_t degree_t;

// ---- Typed vertex ID (bit-reservation) ----
// Top 8 bits of node_id_t encode vertex type; bottom 56 bits are the counter.
// Compatible with bswap64: after Flexograph's key encoding, the type byte lands
// first in WiredTiger's lexicographic comparison, giving free per-type sort order.
// MAKE_EKEY/OG_KEY (+/-1 on full uint64) are safe — 2^56 vertices per type before
// counter bits overflow into type bits.
#define VTYPE_BITS    8
#define VTYPE_SHIFT   56
#define VTYPE_MASK    (((node_id_t)0xFF) << VTYPE_SHIFT)
#define MAKE_TYPED_ID(type, counter) \
    (((node_id_t)(type) << VTYPE_SHIFT) | (node_id_t)(counter))
#define VTYPE_OF(id)    ((id) >> VTYPE_SHIFT)
#define VCOUNTER_OF(id) ((id) & ~VTYPE_MASK)

enum VertexType : uint8_t { VT_PERSON = 0, VT_POST = 1 };

/// @brief EdgeKey specific definitions
const node_id_t OutOfBand_ID_MIN =
    0;  // Used to be -1. Changed to 0 to avoid issues with unsigned types.
#ifdef B64
const node_id_t OutOfBand_ID_MAX = UINT64_MAX;
#else
const node_id_t OutOfBand_ID_MAX = UINT32_MAX;
#endif

typedef enum GraphType
{
  Adj,
  EKey,
  SplitEKey,
  META
} GraphType;

enum PropStorageMode { EMBEDDED, SPLIT, COLUMNAR };

// ---- COLUMNAR mode: per-type property table names ----
const std::string PERSON_PROPS_TABLE   = "person_props";
const std::string POST_PROPS_TABLE     = "post_props";
const std::string KNOWS_PROPS_TABLE    = "knows_props";
const std::string LIKES_PROPS_TABLE    = "likes_props";
const std::string PERSON_EMAIL_TABLE   = "person_email";
const std::string PERSON_SPEAKS_TABLE  = "person_speaks";

// Colgroup name suffixes
const std::string CG_TEMPORAL  = "temporal";
const std::string CG_IDENTITY  = "identity";
const std::string CG_CONTENT   = "content";

struct graph_opts
{
  bool read_only = false;
  bool create_new = false;
  bool read_optimize = false;
  bool is_directed = false;
  bool is_weighted = false;
  bool optimize_create = true;  // directs when the index should be created
  std::string db_name{};
  std::string db_dir{};
  std::string conn_config{};
  std::string stat_log{};
  std::string dataset{};
  std::string checkpoint_name{};
  GraphType type;
  node_id_t num_nodes{};
  uint64_t num_edges{};  // we can have > 4B edges
  int num_threads = 1;
  bool sort_edges = false;  // sort edges in the edge table
  bool has_node_props = false;
  bool has_edge_props = false;
  PropStorageMode prop_mode = EMBEDDED;
  // make a default constructor
  graph_opts()
      : read_only(false),
        create_new(false),
        read_optimize(false),
        is_directed(false),
        is_weighted(false),
        optimize_create(true),
        db_name(),
        db_dir(),
        conn_config(),
        stat_log(),
        dataset(),
        checkpoint_name(),
        type(GraphType::Adj),
        num_nodes(0),
        num_edges(0),
        num_threads(1),
        sort_edges(false),
        has_node_props(false),
        has_edge_props(false),
        prop_mode(EMBEDDED)
  {
  }
  ~graph_opts() = default;

  // dump the options
  void print_config(const std::string &filename) const
  {
    std::ostream *out;
    std::ofstream file;

    if (filename.empty())
    {
      out = &std::cout;
    }
    else
    {
      file.open(filename, std::ios::out);
      out = &file;
    }

    *out << "CREATE_NEW: " << create_new << std::endl;
    *out << "READ_OPTIMIZE: " << read_optimize << std::endl;
    *out << "DIRECTED: " << is_directed << std::endl;
    *out << "WEIGHTED: " << is_weighted << std::endl;
    *out << "DB_NAME: " << db_name << std::endl;
    *out << "DB_DIR: " << db_dir << std::endl;
    *out << "OPTIMIZE_CREATE: " << optimize_create << std::endl;
    *out << "CONN_CONFIG: " << conn_config << std::endl;
    *out << "STAT_LOG: " << stat_log << std::endl;
    *out << "GRAPH_TYPE: " << type << std::endl;
    *out << "DATASET: " << dataset << std::endl;
    *out << "NUM_NODES: " << num_nodes << std::endl;
    *out << "NUM_EDGES: " << num_edges << std::endl;
    *out << "SORT_EDGES: " << sort_edges << std::endl;
    *out << "HAS_NODE_PROPS: " << has_node_props << std::endl;
    *out << "HAS_EDGE_PROPS: " << has_edge_props << std::endl;
    *out << "PROP_MODE: " << prop_mode << std::endl;

    if (file.is_open())
    {
      file.close();
    }
  }

  // make an assignment operator
  graph_opts &operator=(const graph_opts &other)
  {
    if (this != &other)
    {
      read_only = other.read_only;
      create_new = other.create_new;
      read_optimize = other.read_optimize;
      is_directed = other.is_directed;
      is_weighted = other.is_weighted;
      db_name = other.db_name;
      db_dir = other.db_dir;
      optimize_create = other.optimize_create;
      conn_config = other.conn_config;
      stat_log = other.stat_log;
      type = other.type;
      num_nodes = other.num_nodes;
      num_edges = other.num_edges;
      dataset = other.dataset;
      num_threads = other.num_threads;
      checkpoint_name = other.checkpoint_name;
      sort_edges = other.sort_edges;  // copy sort_edges option
      has_node_props = other.has_node_props;
      has_edge_props = other.has_edge_props;
      prop_mode = other.prop_mode;
    }
    return *this;
  }
  // make a copy constructor
  graph_opts(const graph_opts &other)
  {
    read_only = other.read_only;
    create_new = other.create_new;
    read_optimize = other.read_optimize;
    is_directed = other.is_directed;
    is_weighted = other.is_weighted;
    db_name = other.db_name;
    db_dir = other.db_dir;
    optimize_create = other.optimize_create;
    conn_config = other.conn_config;
    stat_log = other.stat_log;
    type = other.type;
    num_nodes = other.num_nodes;
    num_edges = other.num_edges;
    checkpoint_name = other.checkpoint_name;
    num_threads = other.num_threads;
    dataset = other.dataset;
    sort_edges = other.sort_edges;  // copy sort_edges option
    has_node_props = other.has_node_props;
    has_edge_props = other.has_edge_props;
    prop_mode = other.prop_mode;
  }
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
  edgeweight_t edge_weight = 0.0;
} edge;

typedef struct edge_index
{
  node_id_t src_id = 0;
  node_id_t dst_id = 0;
  edge_index() : src_id(0), dst_id(0) {}
  [[maybe_unused]] edge_index(node_id_t a, node_id_t b) : src_id(a), dst_id(b)
  {
  }

} edge_index;

typedef struct edge_index key_pair;

typedef struct key_range
{
  node_id_t start;
  node_id_t end;
  key_range() : start(), end() {}
  key_range(node_id_t a, node_id_t b) : start(a), end(b) {}
} key_range;

[[maybe_unused]] typedef key_range node_range;

typedef struct edge_range
{
  key_pair start{};
  key_pair end{};
  edge_range() : start(), end() {}
  [[maybe_unused]] edge_range(key_pair a, key_pair b) : start(a), end(b) {}
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
    if (deg > 1000) edgelist.reserve(deg);
  }
  void clear()
  {
    edgelist.clear();
    node_id = 0;
    degree = 0;
  }
  void insert(node_id_t id)
  {
    edgelist.emplace_back(id);
    degree++;
  }
  void insert_sorted(node_id_t id)
  {
    edgelist.emplace_back(id);
    degree++;
    std::sort(edgelist.begin(), edgelist.end());
  }
} adjlist;
#endif