#ifndef ADJ_LIST
#define ADJ_LIST

#include "common.h"
#include "graph_exception.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <wiredtiger.h>

using namespace std;
namespace AdjIterator
{

    class InCursor : public table_iterator
    {
    public:
        InCursor(WT_CURSOR *cur, WT_SESSION *sess)
        {
            init(cur, sess);
        }

        void set_key(key_pair key)
        {
            cursor->set_key(cursor, key.dst_id); // In neighbourhood
        }

        void next(adjlist *found)
        {
            if (cursor->next(cursor) == 0)
            {
                CommonUtil::__record_to_adjlist(session, cursor, found);
                cursor->get_key(cursor, &found->node_id);
            }
            else
            {
                found->node_id = -1;
            }
        }

        void next(adjlist *found, key_pair keys) override
        {
            cursor->set_key(cursor, keys.dst_id);
            if (cursor->search(cursor) == 0)
                {

                    CommonUtil::__record_to_adjlist(session, cursor, found);
                found->node_id = keys.dst_id;
                    cursor->reset(cursor);
            }
            else
            {
                found->node_id = -1; // check for out-of-band values in application program.
                cursor->reset(cursor);
            }
        }
    };

    class OutCursor : public table_iterator
    {
    public:
        OutCursor(WT_CURSOR *cur, WT_SESSION *sess)
        {
            init(cur, sess);
        }

        void set_key(key_pair key)
        {
            cursor->set_key(cursor, key.src_id); // Out neighbourhood.
        }

        void next(adjlist *found)
        {
            if (cursor->next(cursor) == 0)
            {
                CommonUtil::__record_to_adjlist(session, cursor, found);
                cursor->get_key(cursor, &found->node_id);
            }
            else
            {
                found->node_id = -1;
            }
        }

        void next(adjlist *found, key_pair keys) override
        {
            if (cursor->next(cursor) == 0)
            {
                cursor->get_key(cursor, &found->node_id);
                if (found->node_id == keys.src_id)
                {
                    CommonUtil::__record_to_adjlist(session, cursor, found);
                }
                else
                {
                    found->node_id = -1;
                    cursor->reset(cursor);
                }
            }
            else
            {
                found->node_id = -1;
            }
        }
    };
};

class AdjList
{
public:
    AdjList(graph_opts &opt_params);
    AdjList();
    void create_new_graph();
    void add_node(node to_insert);
    void add_node(int to_insert, std::vector<int> &inlist, std::vector<int> &outlist);
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
    void delete_edge(int src_id, int dst_id);
    edge get_edge(int src_id, int dst_id);
    std::vector<edge> get_edges();
    std::vector<edge> get_out_edges(int node_id);
    std::vector<node> get_out_nodes(int node_id);
    std::vector<edge> get_in_edges(int node_id);
    std::vector<node> get_in_nodes(int node_id);
    void close();
    std::string get_db_name() const { return opts.db_name; };
    std::vector<int> get_adjlist(WT_CURSOR *cursor, int node_id);
    AdjIterator::OutCursor get_outnbd_cursor();
    AdjIterator::InCursor get_innbd_cursor();

    int get_edge_weight(int src_id, int dst_id);                      // todo <-- is this implemented?
    void update_edge_weight(int src_id, int dst_id, int edge_weight); // todo <-- is this implemented?

    // internal cursor operations:
    //! Check if these should be public:
    void init_cursors();
    WT_CURSOR *get_node_cursor();
    WT_CURSOR *get_edge_cursor();
    WT_CURSOR *get_in_adjlist_cursor();
    WT_CURSOR *get_out_adjlist_cursor();
    WT_CURSOR *get_edge_iter();
    WT_CURSOR *get_node_iter();

private:
    WT_CONNECTION *conn;
    WT_SESSION *session;
    graph_opts opts;

    // structure of the graph
    int edge_id;
    int node_attr_size = 0; // set on checking the list len

    vector<string> node_columns = {ID}; // Always there :)
    vector<string> edge_columns = {SRC, DST};
    vector<string> in_adjlist_columns = {ID, IN_DEGREE, IN_ADJLIST};
    vector<string> out_adjlist_columns = {ID, OUT_DEGREE, OUT_ADJLIST};

    string node_value_format;
    string node_key_format = "I";
    string edge_key_format = "II"; // SRC DST in the edge table
    string edge_value_format = ""; // Make I if weighted , x otherwise
    string adjlist_key_format = "I";
    string adjlist_value_format = "Iu"; // This HAS to be u. S does not work. s needs the number.

    WT_CURSOR *node_cursor = NULL;
    WT_CURSOR *random_node_cursor = NULL;
    WT_CURSOR *edge_cursor = NULL;
    WT_CURSOR *in_adjlist_cursor = NULL;
    WT_CURSOR *out_adjlist_cursor = NULL;
    WT_CURSOR *metadata_cursor = NULL;
    // AdjIterator::InCursor in_cursor;
    // AdjIterator::OutCursor out_cursor;

    // AdjList specific internal methods:
    int _get_table_cursor(const string &table, WT_CURSOR **cursor, bool is_random);
    void update_node_degree(WT_CURSOR *cursor, int node_id, int in_degree,
                            int out_degree);
    node get_next_node(WT_CURSOR *n_cur);
    edge get_next_edge(WT_CURSOR *e_cur);
    void add_adjlist(WT_CURSOR *cursor, int node_id);
    void add_adjlist(WT_CURSOR *cursor, int node_id, std::vector<int> &list);
    void delete_adjlist(WT_CURSOR *cursor, int node_id);
    void delete_node_from_adjlists(int node_id);
    void add_to_adjlists(WT_CURSOR *cursor, int node_id, int to_insert);
    void delete_from_adjlists(WT_CURSOR *cursor, int node_id, int to_delete);
    void delete_related_edges_and_adjlists(int node_id);

    // Metadata operations:
    void insert_metadata(const string &key, const char *value);
    string get_metadata(const string &key);
    void __restore_from_db(string db_name);
    void dump_tables();
};

/**
 * iterator design:
 *
 * In the application, we need access to in and out neighbouhoods.
 * Ideally, we need an iterator class, on object of which can be returned to the app
 * these objects should have set_key(src, dst) and std::vector<int> get_value()
 *
 */
#endif