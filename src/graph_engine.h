#ifndef GRAPH_ENGINE
#define GRAPH_ENGINE

#include <array>
#include <string>

#include "adj_list.h"
#include "common_util.h"
#include "edgekey.h"
#include "edgekey_split.h"
#include "graph.h"
#include "graph_exception.h"
// #include "standard_graph.h"

enum class PartitionStrategy {
  NODE_COUNT,         // Uses compute_nodes_and_partition() - simple, uniform partitioning
  EDGE_AWARE,         // Uses new_parts() - accounts for non-uniform degree distribution
  NODE_COUNT_FINE,    // Uses make_min_parts() - node-count with finer granularity
};

class GraphEngine
{
 public:
  GraphEngine(int _num_threads, const graph_opts &engine_opts);
  GraphEngine();
  ~GraphEngine();
  GraphBase *create_graph_handle();
  GraphBase *create_ro_graph_handle(std::string &checkpoint_name);
  void create_indices();
  void calculate_thread_offsets(bool make_edge = false);
  key_range get_key_range(int thread_id);
  edge_range get_edge_range(int thread_id);
  std::vector<key_range> get_work_chunks(int chunk_size = 500000);
  void close_graph();
  WT_CONNECTION *get_connection();
  std::string make_checkpoint();
  std::string get_last_checkpoint() { return last_checkpoint; }
  void set_partition_scale(int parts_per_thread)
  {
    this->partition_scale = parts_per_thread;
  }
  int get_total_partitions() const
  {
    // Return actual number of partitions created
    // node_ranges has (num_partitions + 1) boundaries
    if (node_ranges.size() <= 1)
      return 0;
    return node_ranges.size() - 1;
  }
  int get_min_partitions() const { return num_threads * partition_scale; }
  void set_partition_strategy(PartitionStrategy strategy)
  {
    this->partition_strategy = strategy;
  }
  PartitionStrategy get_partition_strategy() const
  {
    return partition_strategy;
  }
  std::string get_strategy_name() const
  {
    switch(partition_strategy) {
      case PartitionStrategy::NODE_COUNT: return "NODE_COUNT";
      case PartitionStrategy::EDGE_AWARE: return "EDGE_AWARE";
      case PartitionStrategy::NODE_COUNT_FINE: return "NODE_COUNT_FINE";
    }
    return "UNKNOWN";
  }

 protected:
  WT_CONNECTION *conn = nullptr;
  std::vector<node_id_t> node_ranges;
  std::vector<node_id_t> all_node_ids;  // All node IDs for chunk creation
  std::vector<edge> edge_ranges;
  int num_threads{};
  graph_opts opts;
  int partition_scale{1};
  PartitionStrategy partition_strategy{PartitionStrategy::EDGE_AWARE};

 protected:
  node_id_t last_node_id{};

  void check_opts_valid() const;
  void create_new_graph();
  void open_connection();
  void close_connection();

 private:
  std::string last_checkpoint;
  node_id_t checkpoint_node_count{};
  void force_metadata_sync();
  node_id_t _calculate_exact_node_count(GraphBase *graph_stats);
  void _calculate_thread_offsets_fast(int thread_max, GraphBase *graph_stats);
  node_id_t compute_nodes_and_partition(int thread_max, GraphBase *graph_stats);
  node_id_t new_parts(int thread_max,
                      GraphBase *graph_stats,
                      int mini_part_scale = 1);
  node_id_t make_min_parts(int thread_max,
                           GraphBase *graph_stats,
                           int mini_part_scale = 1);
  void _calculate_thread_offsets_edge(int thread_max, GraphBase *graph_stats);
};

#endif