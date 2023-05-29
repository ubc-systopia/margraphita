#ifndef GRAPH_ENGINE
#define GRAPH_ENGINE

#include "adj_list.h"
#include "common.h"
#include "edgekey.h"
#include "edgelist.h"
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
    std::pair<uint64_t, uint64_t> request_insert_range();
    std::pair<edge, edge> get_edge_range(int thread_id);
    std::pair<node, node> get_key_range(int thread_id);
    WT_CONNECTION* get_connection();
    void close_graph();

   protected:
    WT_CONNECTION* conn = nullptr;
    std::vector<std::pair<node, node>> node_ranges;
    std::vector<std::pair<edge, edge>> edge_ranges;
    uint64_t insert_head;
    LockSet* locks;
    int num_threads;
    graph_opts opts;

    void check_opts_valid();
    void create_new_graph();
    void open_connection();
    void close_connection();
    void calculate_thread_offsets_node(
        int thread_max,
        node_id_t num_nodes,
        std::vector<std::pair<node, node>>& node_ranges);
    void calculate_thread_offsets_edge(
        int thread_max,
        node_id_t num_edges,
        std::vector<std::pair<edge, edge>>& edge_ranges);
};

#endif