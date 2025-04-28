//
// Created by puneet on 05/02/24.
//

#ifndef GRAPHAPI_BULK_INSERT_LOW_MEM_H
#define GRAPHAPI_BULK_INSERT_LOW_MEM_H
#include <sys/stat.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <random>
#include <utility>
#include <vector>

#include "common_util.h"
#include "reader.h"
#include "time_structs.h"
#include "times.h"

// #define NUM_THREADS 16
#define PRINT_ADJ_ERROR(node_id, ret, msg)                                \
  {                                                                       \
    std::cerr << "Error inserting adjlist for node " << (node_id) << ": " \
              << (msg) << " (" << (ret) << ")" << std::endl;              \
  }

#define PRINT_EDGE_ERROR(src, dst, ret, msg)                      \
  std::cerr << "Error inserting edge (" << (src) << ", " << (dst) \
            << "): " << (msg) << " (" << (ret) << ")" << std::endl;

size_t space = 0;
graph_opts opts;
int num_per_chunk;

WT_CONNECTION *conn_std, *conn_adj, *conn_split_ekey;

typedef struct fgraph_conn_object
{
  GraphType type;
  WT_CONNECTION *conn = nullptr;
  WT_SESSION *session = nullptr;
  WT_CURSOR *e_cur = nullptr;
  WT_CURSOR *n_cur = nullptr;
  WT_CURSOR *cur = nullptr;  // adjlist cursor or split edgekey table cursors
  WT_CURSOR *metadata = nullptr;
  std::string adjlistable_name;

  fgraph_conn_object(WT_CONNECTION *_conn,
                     std::string _wt_adjtable_str,
                     GraphType _type,
                     bool is_edge = true)
      : type(_type), conn(_conn), adjlistable_name(std::move(_wt_adjtable_str))
  {
    int ret = conn->open_session(conn, nullptr, nullptr, &session);
    if (ret != 0)
    {
      std::cerr << "Error opening session: " << wiredtiger_strerror(ret)
                << std::endl;
      exit(1);
    }
    // Open Cursors. If the table type is META, open the metadata cursor
    // and return.
    if (type == GraphType::META)
    {
      // open the metadata cursor
      ret = session->open_cursor(
          session, "table:metadata", nullptr, nullptr, &metadata);
      if (ret != 0)
      {
        std::cerr << "Error opening cursor: " << wiredtiger_strerror(ret)
                  << std::endl;
        exit(1);
      }
      return;
    }
    if (is_edge)
    {
      if (type == GraphType::SplitEKey)
      {
        ret = session->open_cursor(
            session, adjlistable_name.c_str(), nullptr, nullptr, &e_cur);
        if (ret != 0)
        {
          std::cerr << "Error opening cursor to " << adjlistable_name << ": "
                    << wiredtiger_strerror(ret) << std::endl;
          exit(1);
        }
        return;
      }
      ret =
          session->open_cursor(session, "table:edge", nullptr, nullptr, &e_cur);
      if (ret != 0)
      {
        std::cerr << "Error opening cursor: " << wiredtiger_strerror(ret)
                  << std::endl;
        exit(1);
      }
      if (type == GraphType::Adj)
      {
        ret = session->open_cursor(
            session, adjlistable_name.c_str(), nullptr, nullptr, &cur);
        if (ret != 0)
        {
          std::cerr << "Error opening cursor: " << wiredtiger_strerror(ret)
                    << std::endl;
          exit(1);
        }
      }
    }
    else  // We are inserting a node.
    {
      if (type == GraphType::EKey || type == GraphType::SplitEKey)
      {
        std::string table_name = "table:edge";
        if (type == GraphType::SplitEKey) table_name += "_out";
        ret = session->open_cursor(
            session, table_name.c_str(), nullptr, nullptr, &e_cur);
        if (ret != 0)
        {
          std::cerr << "Error opening cursor: " << wiredtiger_strerror(ret)
                    << std::endl;
          exit(1);
        }
      }
      else
      {
        ret = session->open_cursor(
            session, "table:node", nullptr, nullptr, &n_cur);
        if (ret != 0)
        {
          std::cerr << "Error opening cursor: " << wiredtiger_strerror(ret)
                    << std::endl;
          exit(1);
        }
      }
    }
  }

  ~fgraph_conn_object()
  {
    if (cur != nullptr)
    {
      cur->close(cur);
    }
    if (e_cur != nullptr)
    {
      e_cur->close(e_cur);
    }
    if (n_cur != nullptr)
    {
      n_cur->close(n_cur);
    }
    if (metadata != nullptr)
    {
      metadata->close(metadata);
    }
    if (session != nullptr)
    {
      session->close(session, nullptr);
    }
  }
} worker_sessions;

typedef struct degrees_
{
  degree_t in_degree = 0;
  degree_t out_degree = 0;
} degrees;

// Using a TBB concurrent map to store the degrees of the nodes.
using degree_map = tbb::concurrent_hash_map<node_id_t, degrees>;
degree_map node_degrees;

std::tuple<node_id_t, node_id_t> get_min_max_key()
{
  node_id_t min, max;
  min = 0;
  max = OutOfBand_ID_MAX;
  // Reduction clause for finding the maximum
  auto it = node_degrees.cbegin();
  node_id_t key_min = it->first;
  node_id_t key_max = key_min;
  while (++it != node_degrees.end())
  {
    if (it->first > key_min)
    {
      key_max = it->first;
    }
  }
  return {min, max};
}

int check_cursors(worker_sessions &info)
{
  std::cerr << "Checking cursors\n";
  assert(info.cur != nullptr);
  assert(info.session != nullptr);
  assert(info.conn != nullptr);
  return 0;
}

// Overwrites existing weights with random from [1,255]
int *InsertWeights(size_t n)
{
  // Set up random number generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> distribution(0, 255);

  // Allocate memory for the array
  int *randomArray = new int[n];

  // Fill the array with random values
  for (int i = 0; i < n; ++i)
  {
    randomArray[i] = distribution(gen);
  }

  return randomArray;
}

int add_to_adjlist(WT_CURSOR *adjcur, adjlist &adj)
{
  CommonUtil::set_key(adjcur, adj.node_id);
  int ret;
  if (adjcur->search(adjcur) == 0)
  {
    adjlist old_adj;
    CommonUtil::record_to_adjlist(adjcur, &old_adj);
    // merge the edgelists and remove duplicates
    std::vector<node_id_t> new_edgelist;
    std::set_union(adj.edgelist.begin(),
                   adj.edgelist.end(),
                   old_adj.edgelist.begin(),
                   old_adj.edgelist.end(),
                   std::back_inserter(new_edgelist));
    WT_ITEM item;
    item.data = new_edgelist.data();
    item.size = new_edgelist.size() * sizeof(node_id_t);
    adjcur->set_value(adjcur, new_edgelist.size(), &item);
    ret = adjcur->update(adjcur);
  }
  else
  {
    WT_ITEM item;
    item.data = adj.edgelist.data();
    item.size = adj.edgelist.size() * sizeof(node_id_t);
    // space += adj.edgelist.size() * sizeof(node_id_t);
    adjcur->set_value(adjcur, adj.edgelist.size(), &item);
    ret = adjcur->insert(adjcur);
  }
  if (ret != 0)
  {
    PRINT_ADJ_ERROR(adj.node_id, ret, wiredtiger_strerror(ret))
    return ret;
  }
  return ret;
}

int add_to_edge_table(WT_CURSOR *cur,
                      const node_id_t node_id,
                      const std::vector<node_id_t> &edgelist,
                      int *edge_count,
                      bool is_weighted = false)
{
  int *weight = InsertWeights(edgelist.size());

  int i = 0;
  for (node_id_t dst : edgelist)
  {
    CommonUtil::set_key(cur, node_id, dst);
    if (is_weighted)
      cur->set_value(cur, weight[i], OutOfBand_ID_MAX);
    else
      cur->set_value(cur, 0, OutOfBand_ID_MAX);
    int ret = cur->insert(cur);
    if (ret != 0)
    {
      PRINT_EDGE_ERROR(node_id, dst, ret, wiredtiger_strerror(ret))
      delete[] weight;
      return ret;
    }
    i++;
  }
  *edge_count += edgelist.size();
  delete[] weight;
  return 0;
}

int add_to_edgekey(WT_CURSOR *ekey_cur,
                   const node_id_t node_id,
                   const std::vector<node_id_t> &edgelist,
                   bool is_weighted = false)
{
  node_id_t src = node_id;
  int *weight = InsertWeights(edgelist.size());
  for (int i = 0; i < edgelist.size(); i++)
  {
    CommonUtil::ekey_set_key(ekey_cur, src, edgelist[i]);
    if (is_weighted)
      ekey_cur->set_value(ekey_cur, weight[i], OutOfBand_ID_MAX);
    else
      ekey_cur->set_value(ekey_cur, 0, OutOfBand_ID_MAX);

    int ret = ekey_cur->insert(ekey_cur);
    if (ret != 0)
    {
      PRINT_EDGE_ERROR(node_id, edgelist[i], ret, wiredtiger_strerror(ret))
      delete[] weight;
      return ret;
    }
  }
  delete[] weight;
  return 0;
}

inline int add_to_node_table(WT_CURSOR *cur,
                             const node_id_t id,
                             const degree_t in_degree,
                             const degree_t out_degree)
{
  CommonUtil::set_key(cur, id);
  if (opts.read_optimize)
  {
    if (id == 0)
    {
      std::cout << "Adding node 0 with in_degree: " << in_degree
                << " and out_degree: " << out_degree << std::endl;
      std::cout << "directed? " << opts.is_directed << std::endl;
    }
    opts.is_directed ? cur->set_value(cur, in_degree, out_degree)
                     : cur->set_value(cur, in_degree + out_degree);
  }
  else
  {
    cur->set_value(cur, "");
  }
  int ret = cur->insert(cur);
  if (ret != 0)
  {
    std::cerr << "Error inserting node " << id << ": "
              << wiredtiger_strerror(ret) << std::endl;
  }
  return ret;
}

int add_node_to_ekey(WT_CURSOR *ekey_cur,
                     const node_id_t id,
                     const degree_t in_degree,
                     const degree_t out_degree)
{
  CommonUtil::ekey_set_key(ekey_cur, id, OutOfBand_ID_MIN);
  ekey_cur->set_value(ekey_cur, in_degree, out_degree);
  int ret = ekey_cur->insert(ekey_cur);
  if (ret != 0)
  {
    std::cerr << "Error inserting node " << id << ": "
              << wiredtiger_strerror(ret) << std::endl;
    return ret;
  }
  return ret;
}

void debug_print_edges(WT_CURSOR *cur,
                       GraphType type,
                       const std::string &filename)
{
  cur->reset(cur);
  // open filename for writing
  std::ofstream file;
  file.open(filename);

  while (cur->next(cur) == 0)
  {
    node_id_t src, dst;
    int val1, val2;
    if (type == GraphType::EKey)
    {
      CommonUtil::ekey_get_key(cur, &src, &dst);
      cur->get_value(cur, &val1, &val2);
      file << "Edge: (" << src << ", " << dst << ") val1: " << val1
           << ", val2: " << val2 << std::endl;
    }
    else
    {
      CommonUtil::get_key(cur, &src, &dst);
      cur->get_value(cur, &val1);
      file << "Edge: (" << src << ", " << dst << ") weight: " << val1
           << std::endl;
    }
  }
  file.close();
}

void make_connections(graph_opts &_opts, const std::string &conn_config)
{
  std::string middle;
  if (_opts.read_optimize)
  {
    middle += "r";
  }
  if (_opts.is_directed)
  {
    middle += "d";
  }
  if (!middle.empty())
  {
    middle += "_";
  }
  // make sure the dataset is a directory, if not, extract the directory name
  // and use it
  if (_opts.dataset.back() != '/')
  {
    size_t found = _opts.dataset.find_last_of("/\\");
    if (found != std::string::npos)
    {
      _opts.dataset = _opts.dataset.substr(0, found);
    }
  }
  std::cout << "Dataset: " << _opts.dataset << std::endl;

  std::string _db_name = _opts.db_dir + "/adj_" + middle + _opts.db_name;
  if (wiredtiger_open(_db_name.c_str(),
                      nullptr,
                      const_cast<char *>(conn_config.c_str()),
                      &conn_adj) != 0)
  {
    std::cout << "Could not open the DB: " << _db_name;
    exit(1);
  }

  // open split ekey connection
  _db_name = _opts.db_dir + "/split_ekey_" + middle + _opts.db_name;
  if (wiredtiger_open(_db_name.c_str(),
                      nullptr,
                      const_cast<char *>(conn_config.c_str()),
                      &conn_split_ekey) != 0)
  {
    std::cout << "Could not open the DB: " << _db_name;
    exit(1);
  }

  assert(conn_adj != nullptr);
  assert(conn_split_ekey != nullptr);
}

/**
 * @brief This private function inserts metadata values into the metadata
 * table. The fields are self explanatory.
 *
 */
void add_metadata(const int key,
                  const char *value,
                  size_t size,
                  WT_CURSOR *cursor)
{
  cursor->set_key(cursor, key);
  WT_ITEM item;
  item.data = value;
  item.size = size;
  cursor->set_value(cursor, &item);
  int ret = cursor->insert(cursor);
  if (ret != 0)
  {
    fprintf(stderr, "Failed to insert metadata for the key %d", key);
    fprintf(stderr, "Error: %s\n", wiredtiger_strerror(ret));
    exit(-1);
  }
}

void dump_config(const graph_opts &_opts, const std::string &conn_config)
{
  // dump the config
  std::cout << "Dumping parameters: " << std::endl;
  std::cout << "Dataset: " << _opts.dataset << std::endl;
  std::cout << "Read optimized: " << _opts.read_optimize << std::endl;
  std::cout << "Directed: " << _opts.is_directed << std::endl;
  std::cout << "Weighted: " << _opts.is_weighted << std::endl;
  std::cout << "Num edges: " << _opts.num_edges << std::endl;
  std::cout << "Num nodes: " << _opts.num_nodes << std::endl;
  std::cout << "Num threads: " << _opts.num_threads << std::endl;
  std::cout << "DB dir: " << _opts.db_dir << std::endl;
  std::cout << "DB name: " << _opts.db_name << std::endl;
  std::cout << "DB config: " << conn_config << std::endl;
}

#endif  // GRAPHAPI_BULK_INSERT_LOW_MEM_H
