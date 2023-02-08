#ifndef GRAPH_ENGINE
#define GRAPH_ENGINE

#include "adj_list.h"
#include "common.h"
#include "edgekey.h"
#include "graph.h"
#include "graph_exception.h"
#include "lock.h"
#include "standard_graph.h"

using namespace std;

class GraphEngine
{
   public:
    struct graph_engine_opts
    {
        int num_threads;
        graph_opts opts;
    };
    GraphEngine(graph_engine_opts engine_opts);
    ~GraphEngine();
    GraphBase* create_graph_handle();
    void create_indices();
    void calculate_thread_offsets();
    void calculate_thread_offsets_edge_partition();
    void close_graph();
    edge_range get_edge_range(int thread_id);
    key_range get_key_range(int thread_id);
    WT_CONNECTION* get_connection();

   protected:
    WT_CONNECTION* conn = nullptr;
    std::vector<key_range> node_ranges;
    std::vector<edge_range> edge_ranges;
    LockSet* locks;
    int num_threads;
    graph_opts opts;

    void check_opts_valid();
    void create_new_graph();
    void open_connection();
    void close_connection();
    void calculate_thread_offsets(int thread_max,
                                  node_id_t num_nodes,
                                  node_id_t num_edges,
                                  std::vector<key_range>& node_ranges,
                                  std::vector<edge_range>& edge_offsets);
    void calculate_thread_offsets_edge_partition(
        int thread_max,
        node_id_t num_nodes,
        node_id_t num_edges,
        std::vector<edge_range>& edge_offsets);
};

#endif