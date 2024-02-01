#pragma clang diagnostic push
#pragma ide diagnostic ignored "openmp-use-default-none"
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <unordered_map>
#include <vector>

#include "bulk_insert.h"
#include "common_util.h"
#include "mk_adjlists.h"
#include "parallel_hashmap/phmap.h"
#include "reader.h"
#include "time_structs.h"
#include "times.h"

#define MAPNAME phmap::parallel_flat_hash_map
#define NMSP phmap
#define NUM_THREADS 16
#define MTX std::mutex
#define EXTRA_ARGS                                                      \
    , NMSP::priv::hash_default_hash<K>, NMSP::priv::hash_default_eq<K>, \
        std::allocator<std::pair<const K, V>>, 4, MTX

template <class K, class V>
using HashT = MAPNAME<K, V EXTRA_ARGS>;
using hash_t = HashT<node_id_t, uint32_t>;
using vec_hash_t = HashT<node_id_t, std::vector<node_id_t>>;

#define PRINT_ERROR(src, dst, ret, msg)                            \
    std::cout << "Error inserting edge (" << (src) << "," << (dst) \
              << "). ret= " << (ret) << " : " << (msg) << std::endl;
#define PRINT_NODE_ERROR(node, ret, msg)                                  \
    std::cout << "Error inserting node " << (node) << ". ret = " << (ret) \
              << ": " << (msg) << std::endl;
WT_CONNECTION *conn_std, *conn_adj, *conn_ekey;
double num_edges;
double num_nodes;
std::string dataset;
int read_optimized = 0;
int is_directed = 1;
int num_per_chunk;
hash_t in_adj_list;
hash_t out_adj_list;
vec_hash_t merged_adjlists;
std::string type_opt;

void insert_edge_thread(int _tid)
{
    int tid = _tid;
    std::string filename = dataset + "_edges";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    int32_t edge_id = (tid * num_per_chunk);

    reader::EdgeReader graph_reader(
        std::move(filename), 0, num_per_chunk, "out");

    edge e;

    WT_CURSOR *cur_std, *cur_adj, *cur_ekey;
    WT_SESSION *sess_std, *sess_adj, *sess_ekey;
    conn_std->open_session(conn_std, nullptr, nullptr, &sess_std);
    sess_std->open_cursor(sess_std, "table:edge", nullptr, nullptr, &cur_std);

    conn_adj->open_session(conn_adj, nullptr, nullptr, &sess_adj);
    sess_adj->open_cursor(sess_adj, "table:edge", nullptr, nullptr, &cur_adj);

    conn_ekey->open_session(conn_ekey, nullptr, nullptr, &sess_ekey);
    sess_ekey->open_cursor(
        sess_ekey, "table:edge", nullptr, nullptr, &cur_ekey);

    while (graph_reader.get_next_edge(e) == 0)
    {
        int ret;
        e.id = edge_id++;  // Setting this here in case we need to use it later.
        CommonUtil::set_key(cur_std, e.src_id, e.dst_id);
        cur_std->set_value(cur_std, e.edge_weight);
        if ((ret = cur_std->insert(cur_std)) != 0)
        {
            PRINT_ERROR(e.src_id, e.dst_id, ret, wiredtiger_strerror(ret))
        }

        CommonUtil::set_key(cur_adj, e.src_id, e.dst_id);
        cur_adj->set_value(cur_adj, e.edge_weight);
        if ((ret = cur_adj->insert(cur_adj)) != 0)
        {
            PRINT_ERROR(e.src_id, e.dst_id, ret, wiredtiger_strerror(ret))
        }

        CommonUtil::set_key(cur_ekey, e.src_id, e.dst_id);
        cur_ekey->set_value(cur_ekey, 0, OutOfBand_Val);
        if ((ret = cur_ekey->insert(cur_ekey)) != 0)
        {
            PRINT_ERROR(e.src_id, e.dst_id, ret, wiredtiger_strerror(ret))
        }
        out_adj_list[e.src_id] += 1;
    }

    cur_std->close(cur_std);
    cur_adj->close(cur_adj);
    cur_ekey->close(cur_ekey);

    sess_adj->close(sess_adj, nullptr);
    sess_ekey->close(sess_ekey, nullptr);
    sess_std->close(sess_std, nullptr);
}

void create_revedge_thread(int _tid)
{
    int tid = _tid;
    std::string filename = dataset + "_reverse_edges";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    reader::EdgeReader graph_reader(
        std::move(filename), 0, num_per_chunk, "in");
    edge e;
    while (graph_reader.get_next_edge(e) == 0)
    {
        in_adj_list[e.src_id] += 1;
    }
}
void insert_node(int _tid)
{
    int tid = _tid;  //*(int *)arg;
    std::string filename = dataset + "_nodes";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    WT_CURSOR *std_cur, *ekey_cur, *adj_cur;
    WT_SESSION *std_sess, *ekey_sess, *adj_sess;

    conn_std->open_session(conn_std, nullptr, nullptr, &std_sess);
    std_sess->open_cursor(std_sess, "table:node", nullptr, nullptr, &std_cur);

    conn_adj->open_session(conn_adj, nullptr, nullptr, &adj_sess);
    adj_sess->open_cursor(adj_sess, "table:node", nullptr, nullptr, &adj_cur);

    conn_ekey->open_session(conn_ekey, nullptr, nullptr, &ekey_sess);
    ekey_sess->open_cursor(
        ekey_sess, "table:edge", nullptr, nullptr, &ekey_cur);

    reader::NodeReader graph_reader(std::move(filename));
    node to_insert;
    while (graph_reader.get_next_node(to_insert) == 0)
    {
        int ret;
        to_insert.in_degree = in_adj_list[to_insert.id];
        to_insert.out_degree = out_adj_list[to_insert.id];

        CommonUtil::set_key(std_cur, to_insert.id);
        if (read_optimized)
        {
            std_cur->set_value(
                std_cur, to_insert.in_degree, to_insert.out_degree);
        }
        else
        {
            std_cur->set_value(std_cur, "");
        }

        ret = std_cur->insert(std_cur) != 0;
        if (ret)
        {
            PRINT_NODE_ERROR(to_insert.id, ret, wiredtiger_strerror(ret))
        }

        CommonUtil::set_key(adj_cur, to_insert.id);
        if (read_optimized)
        {
            adj_cur->set_value(
                adj_cur, to_insert.in_degree, to_insert.out_degree);
        }
        else
        {
            adj_cur->set_value(adj_cur, "");
        }
        if ((ret = adj_cur->insert(adj_cur)) != 0)
        {
            PRINT_NODE_ERROR(to_insert.id, ret, wiredtiger_strerror(ret))
        }

        CommonUtil::set_key(ekey_cur, to_insert.id, OutOfBand_ID);
        if (read_optimized)
        {
            ekey_cur->set_value(
                ekey_cur, to_insert.in_degree, to_insert.out_degree);
        }
        else
        {
            ekey_cur->set_value(ekey_cur, 0, 0);
        }
        if ((ret = ekey_cur->insert(ekey_cur)) != 0)
        {
            PRINT_NODE_ERROR(to_insert.id, ret, wiredtiger_strerror(ret))
        }
    }
    std_cur->close(std_cur);
    adj_cur->close(adj_cur);
    ekey_cur->close(ekey_cur);

    std_sess->close(std_sess, nullptr);
    adj_sess->close(adj_sess, nullptr);
    ekey_sess->close(ekey_sess, nullptr);
}

void create_adj_insert_thread(int _tid, const std::string &adj_type)
{
    int tid = _tid;
    std::string filename = dataset;
    if (adj_type == "in")
    {
        filename += "_reverse_edgesa";
    }
    else
    {
        filename += "_edgesa";
    }
    char c = (char)(97 + tid);
    filename.push_back(c);
    filename.push_back('_');
    filename += adj_type;

    reader::AdjReader adj_reader(filename);
    std::pair<int, std::vector<node_id_t>> node_adj_list;

    WT_CURSOR *adjlist_cur;
    WT_SESSION *adj_sess;
    conn_adj->open_session(conn_adj, nullptr, nullptr, &adj_sess);
    if (adj_type == "in")
    {
        adj_sess->open_cursor(
            adj_sess, "table:adjlistin", nullptr, nullptr, &adjlist_cur);
    }
    else
    {
        adj_sess->open_cursor(
            adj_sess, "table:adjlistout", nullptr, nullptr, &adjlist_cur);
    }

    int ret = 0;
    bool first = true;
    do
    {
        ret = adj_reader.get_next_adjlist(node_adj_list);
        if (first)
        {
            if (merged_adjlists.contains(node_adj_list.first))
            {
                node_adj_list.second.insert(
                    node_adj_list.second.end(),
                    merged_adjlists[node_adj_list.first].begin(),
                    merged_adjlists[node_adj_list.first].end());
                merged_adjlists[node_adj_list.first] = node_adj_list.second;
            }

            first = false;
        }
        if (!ret)
        {
            adjlist_cur->set_key(adjlist_cur, node_adj_list.first);
            try
            {
                WT_ITEM item;
                item.data = CommonUtil::pack_int_vector_wti(
                    adj_sess, node_adj_list.second, &item.size);
                adjlist_cur->set_value(
                    adjlist_cur, node_adj_list.second.size(), &item);
            }
            catch (const std::out_of_range &oor)
            {
                WT_ITEM item = {.data = {}, .size = 0};  // todo: check
                adjlist_cur->set_value(adjlist_cur, 0, &item);
            }
            adjlist_cur->insert(adjlist_cur);
        }

    } while (ret == 0);
    for (const auto &conflict_nodes : merged_adjlists)
    {
        adjlist_cur->set_key(adjlist_cur, conflict_nodes.first);
        try
        {
            WT_ITEM item;
            item.data = CommonUtil::pack_int_vector_wti(
                adj_sess, conflict_nodes.second, &item.size);
            adjlist_cur->set_value(
                adjlist_cur, conflict_nodes.second.size(), &item);
        }
        catch (const std::out_of_range &oor)
        {
            WT_ITEM item = {.data = {}, .size = 0};  // todo: check
            adjlist_cur->set_value(adjlist_cur, 0, &item);
        }
        adjlist_cur->insert(adjlist_cur);
    }
    adjlist_cur->close(adjlist_cur);
    adj_sess->close(adj_sess, nullptr);
}

int main(int argc, char *argv[])
{
    InsertOpts params(argc, argv);
    if (!params.parse_args())
    {
        params.print_help();
        return -1;
    }
    graph_opts opts = params.make_graph_opts();
    std::string middle;
    if (opts.read_optimize)
    {
        middle += "r";
    }
    if (opts.is_directed)
    {
        middle += "d";
    }
    dataset = opts.dataset;
    std::string _db_name;
    std::string conn_config = "create,cache_size=10GB";
#ifdef STAT
    std::string stat_config =
        "statistics=(all),statistics_log=(wait=0,on_close=true";
    conn_config += "," + stat_config;
#endif
    num_per_chunk = (int)(opts.num_edges / NUM_THREADS);
    // We are using edge IDs now so we might need to assign a unique range to
    // each thread

    // open std connection
    type_opt = params.get_type_str();
    //    std::vector<std::string> db_names = {};

    if (type_opt == "all" || type_opt == "std")
    {
        _db_name = opts.db_dir + "/std_" + middle + "_" + opts.db_name;
        if (wiredtiger_open(_db_name.c_str(),
                            nullptr,
                            const_cast<char *>(conn_config.c_str()),
                            &conn_std) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    // open adjlist connection
    if (type_opt == "all" || type_opt == "adj")
    {
        _db_name = opts.db_dir + "/adj_" + middle + "_" + opts.db_name;
        if (wiredtiger_open(_db_name.c_str(),
                            nullptr,
                            const_cast<char *>(conn_config.c_str()),
                            &conn_adj) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    // open ekey connection
    if (type_opt == "all" || type_opt == "ekey")
    {
        _db_name = opts.db_dir + "/ekey_" + middle + "_" + opts.db_name;
        if (wiredtiger_open(_db_name.c_str(),
                            nullptr,
                            const_cast<char *>(conn_config.c_str()),
                            &conn_ekey) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    Times t;
    t.start();
    time_info edge_times, node_times, adj_times;

#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        insert_edge_thread(i);
    }

#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        create_revedge_thread(i);
    }

#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        create_adj_insert_thread(i, "in");
        create_adj_insert_thread(i, "out");
    }

#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        insert_node(i);
    }

    if (type_opt == "all" || type_opt == "std")
    {
        conn_std->close(conn_std, nullptr);
    }
    if (type_opt == "all" || type_opt == "adj")
    {
        conn_adj->close(conn_adj, nullptr);
    }
    if (type_opt == "all" || type_opt == "ekey")
    {
        conn_ekey->close(conn_ekey, nullptr);
    }
    return (EXIT_SUCCESS);
}
