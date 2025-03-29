#ifndef GRAPH_ENGINE
#define GRAPH_ENGINE

#include <array>

#include "adj_list.h"
#include "common_util.h"
#include "edgekey.h"
#include "edgekey_split.h"
#include "graph.h"
#include "graph_exception.h"
// #include "standard_graph.h"

class GraphEngine
{
 public:
  GraphEngine(int _num_threads, graph_opts &engine_opts);
  GraphEngine();
  ~GraphEngine();
  GraphBase *create_graph_handle();
  GraphBase *create_ro_graph_handle(const string &checkpoint_name = "");
  void create_indices();
  void calculate_thread_offsets(bool make_edge = false);
  key_range get_key_range(int thread_id);
  edge_range get_edge_range(int thread_id);
  void close_graph();
  WT_CONNECTION *get_connection();
  std::string make_checkpoint();
  std::string get_last_checkpoint() { return last_checkpoint; }

 protected:
  WT_CONNECTION *conn = nullptr;
  std::vector<node_id_t> node_ranges;
  std::vector<edge> edge_ranges;
  int num_threads{};
  graph_opts opts;
  node_id_t last_node_id{};

  void check_opts_valid();
  void create_new_graph();
  void open_connection();
  void close_connection();

 private:
  std::string last_checkpoint;
  void force_metadata_sync();
  void _calculate_thread_offsets(int thread_max, GraphBase *graph_stats);
  void _calculate_thread_offsets_fast(int thread_max, GraphBase *graph_stats);
  void _calculate_thread_offsets_edge(int thread_max, GraphBase *graph_stats);
};


#endif