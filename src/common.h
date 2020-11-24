#ifndef BASE_COMMON
#define BASE_COMMON

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <wiredtiger.h>

extern const std::string METADATA;
extern const std::string DB_NAME;
extern const std::string NODE_COLUMNS;
extern const std::string EDGE_COLUMNS;
extern const std::string NODE_ATTR_FORMAT;
extern const std::string EDGE_ATTR_FORMAT;
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

// Read Optimize columns
extern const std::string IN_DEGREE;
extern const std::string OUT_DEGREE;

// Shared column names
extern const std::string SRC;
extern const std::string DST;
extern const std::string ID;
extern const std::string NODE_TABLE;
extern const std::string EDGE_TABLE;
extern const std::string SRC_INDEX;
extern const std::string DST_INDEX;
extern const std::string SRC_DST_INDEX;

struct opt_args {
  std::vector<std::string> node_value_columns;
  std::string node_value_format;
  std::string edge_value_format;
  std::vector<std::string> edge_value_columns;

  bool create_new;
  bool optimize_create;
};

typedef struct node {
  int id;                // node ID
  std::vector<std::string> attr; // Should this be something else?
  std::vector<std::string> data; //! I think this is just begging to be a generic. type. What would Galois do?
  int64_t in_degree = 0;
  int64_t out_degree = 0;
} node;

typedef struct edge {
  int src_id;
  int dst_id;
  std::vector<int> attr; // Should this be something else?
} edge;

class CommonUtil {
public:
  static void set_table(WT_SESSION *session, std::string prefix,
                        std::vector<std::string> columns, std::string key_fmt,
                        std::string val_fmt);

  static std::vector<int>
  CommonUtil::get_default_nonstring_attrs(std::string fmt);

  static std::vector<std::string>
  CommonUtil::get_default_string_attrs(std::string fmt);

  static std::string get_db_name(std::string prefix, std::string name);

  static void check_graph_params(opt_args params);

  static std::string create_string_format(std::vector<std::string> to_pack,
                                          size_t *size);
  static std::string create_intvec_format(std::vector<int> to_pack,
                                          size_t *total_size);
  static char *pack_string_vector(std::vector<std::string>, WT_SESSION *session,
                                  size_t *total_size, std::string *fmt);

  static std::vector<std::string> unpack_string_vector(const char *to_unpack,
                                                       WT_SESSION *session);

  static char *pack_int_vector(std::vector<int>, WT_SESSION *session,
                               size_t *total_size, std::string *fmt);
  static std::vector<int> unpack_int_vector(const char *to_unpack,
                                            WT_SESSION *session);

  static char *pack_string(std::string, WT_SESSION *session, std::string *fmt);
  static std::string unpack_string(const char *to_unpack, WT_SESSION *session);

  static char *pack_int(int to_pack, WT_SESSION *session);
  static int unpack_int(const char *to_unpack, WT_SESSION *session);

  static char *pack_bool(bool, WT_SESSION *session, std::string *fmt);
  static bool unpack_bool(const char *to_unpack, WT_SESSION *session);
  // WT Session and Cursor wrangling operations
  static int open_cursor(WT_SESSION *session, WT_CURSOR **cursor,
                         std::string uri, WT_CURSOR *to_dup,
                         std::string config);
  static int close_cursor(WT_CURSOR *cursor);
  static int close_session(WT_SESSION *session);
  static int close_connection(WT_CONNECTION *conn);
  static int open_connection(char *db_name, WT_CONNECTION **conn);
  static int open_session(WT_CONNECTION *conn, WT_SESSION **session);
};

#endif