#ifndef ADJ_LIST
#define ADJ_LIST

#include "common.h"
#include "graph_exception.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;

class AdjList
{
public:
    AdjList(graph_opts opt_params);
    AdjList();

    void create_new_graph();
    void init_cursors();

    void add_node(node to_insert);
    void add_node(int to_insert, std::vector<int>, std::vector<int>);
    bool has_node(int node_id);
    node get_node(int node_id);
    void delete_node(int node_id);
    node get_random_node();
    int get_in_degree(int node_id);
    int get_out_degree(int node_id);
    std::vector<node> get_nodes();

    int get_num_nodes();
    int get_num_edges();

    void add_edge(edge to_insert, bool is_bulk);
    bool has_edge(int src_id, int dst_id);
    int get_edge_weight(int src_id, int dst_id);
    void delete_edge(int src_id, int dst_id);
    void update_edge_weight(int src_id, int dst_id, int edge_weight);

    void add_adjlist(WT_CURSOR *cursor, int node_id);
    void add_adjlist(WT_CURSOR *cursor, int node_id, std::vector<int> list);
    void delete_adjlist(WT_CURSOR *cursor, int node_id);
    void delete_node_from_adjlists(int node_id);
    std::vector<int> get_adjlist(WT_CURSOR *cursor, int node_id);
    void add_to_adjlists(WT_CURSOR *cursor, int node_id, int to_insert);
    void delete_from_adjlists(WT_CURSOR *cursor, int node_id, int to_delete);

    void delete_related_edges_and_adjlists(int node_id);

    void update_node_degree(WT_CURSOR *cursor, int node_id, int in_degree,
                            int out_degree);
    edge get_edge(int src_id, int dst_id);
    // void update_edge(int src_id, int dst_id, char *new_attrs); <-not needed.
    // skip.
    std::vector<edge> get_edges();
    std::vector<edge> get_out_edges(int node_id);
    std::vector<node> get_out_nodes(int node_id);
    std::vector<edge> get_in_edges(int node_id);
    std::vector<node> get_in_nodes(int node_id);
    //! void set_node_data(int node_id, int idx, string data);
    //! Do we need get all weights of in/out degree from a node
    std::string get_node_data(int node_id, int idx);
    WT_CURSOR *get_node_iter();
    node get_next_node(WT_CURSOR *n_cur);
    WT_CURSOR *get_edge_iter();
    edge get_next_edge(WT_CURSOR *e_cur);
    // Metadata operations:
    void insert_metadata(string key, char *value);
    string get_metadata(string key); //
    void close();

    WT_CURSOR *get_node_cursor();
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_in_adjlist_cursor();
    WT_CURSOR *get_out_adjlist_cursor();

    std::string get_db_name() const { return this->db_name; };
    // WT privates
private:
    WT_CONNECTION *conn;
    WT_SESSION *session;
    //create params
    bool create_new = true;
    bool read_optimize = true;
    bool is_directed = true;
    bool is_weighted = false; //needed to understand when to interpret the weight field in struct edge
    std::string db_name;
    std::string db_dir;

    //structure of the graph
    int edge_id;
    int node_attr_size = 0; // set on checking the list len

    vector<string> node_columns = {ID}; //Always there :)
    vector<string> edge_columns = {SRC, DST};
    vector<string> in_adjlist_columns = {ID, IN_DEGREE, IN_ADJLIST};
    vector<string> out_adjlist_columns = {ID, OUT_DEGREE, OUT_ADJLIST};

    string node_value_format;
    string node_key_format = "I";
    string edge_key_format = "II"; // SRC DST in the edge table
    string edge_value_format = ""; // Make I if weighted , x otherwise
    string adjlist_key_format = "I";
    string adjlist_value_format = "IS";

    WT_CURSOR *node_cursor = NULL;
    WT_CURSOR *random_node_cursor = NULL;
    WT_CURSOR *edge_cursor = NULL;
    WT_CURSOR *in_adjlist_cursor = NULL;
    WT_CURSOR *out_adjlist_cursor = NULL;
    WT_CURSOR *metadata_cursor = NULL;

    void __node_to_record(WT_CURSOR *cursor, node to_insert); //! APT Check
    node __record_to_node(WT_CURSOR *cursor, int key);        //! APT Check
    void __record_to_edge(WT_CURSOR *cursor, edge *found);    //! APT Check
    void __record_to_adjlist(WT_CURSOR *cursor, adjlist *found);
    void __adjlist_to_record(WT_CURSOR *cursor, adjlist to_insert);
    void __read_from_edge_idx(WT_CURSOR *cursor, edge *e_idx); //! APT Check

    // Internal cursor methods
    int _get_table_cursor(string table, WT_CURSOR **cursor, bool is_random);
    void __restore_from_db(string db_name);
};

#endif