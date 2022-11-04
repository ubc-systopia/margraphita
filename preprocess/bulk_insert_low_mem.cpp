#pragma clang diagnostic push
#pragma ide diagnostic ignored "openmp-use-default-none"
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <unordered_map>
#include <vector>

#include "bulk_insert.h"
#include "common.h"
#include "reader.h"
#include "time_structs.h"
#include "times.h"

#define PRINT_ERROR(src, dst, ret, msg)                        \
    std::cerr << "Error inserting edge (" << src << "," << dst \
              << "). ret= " << ret << " : " << msg << std::endl;
#define PRINT_NODE_ERROR(node, ret, msg)                                      \
    std::cerr << "Error inserting node " << node << ". ret = " << ret << ": " \
              << msg << std::endl;
std::string pack_int_to_str(degree_t a, degree_t b)
{
    return std::string(to_string(a) + to_string(b));
}

const static int NUM_THREADS = 16;
const static int BUFFER_LENGTH = 20;

WT_CONNECTION *conn_std, *conn_adj, *conn_ekey;
double num_edges;
double num_nodes;
std::string dataset;
int read_optimized = 0;
int is_directed = 1;
int num_per_chunk;

std::string type_opt;

time_info *insert_edge_thread(int _tid)
{
    int tid = _tid;
    std::string filename = dataset + "_edges";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);
    auto *info = new time_info();

    if (c == 'a')
    {
        reader::EdgeReader graph_reader(
            std::move(filename), 0, num_per_chunk, "out", true);
        edge e;
        while (graph_reader.get_next_edge(e) == 0)
        {
        }
    }
    else
    {
        reader::EdgeReader graph_reader(
            std::move(filename), 0, num_per_chunk, "out", false);
        edge e;
        while (graph_reader.get_next_edge(e) == 0)
        {
        }
    }

    // edge e;

    // WT_CURSOR *cur_std, *cur_adj, *cur_ekey;
    // WT_SESSION *sess_std, *sess_adj, *sess_ekey;
    // conn_std->open_session(conn_std, NULL, NULL, &sess_std);
    // sess_std->open_cursor(sess_std, "table:edge", NULL, NULL, &cur_std);

    // conn_adj->open_session(conn_adj, NULL, NULL, &sess_adj);
    // sess_adj->open_cursor(sess_adj, "table:edge", NULL, NULL, &cur_adj);

    // conn_ekey->open_session(conn_ekey, NULL, NULL, &sess_ekey);
    // sess_ekey->open_cursor(sess_ekey, "table:edge", NULL, NULL, &cur_ekey);
    // std::cout << c;
    // while (graph_reader.get_next_edge(e) == 0)
    // {
    // int ret;
    // cur_std->set_key(cur_std, e.src_id, e.dst_id);
    // cur_std->set_value(cur_std, e.edge_weight);
    // if ((ret = cur_std->insert(cur_std)) != 0)
    // {
    //     PRINT_ERROR(e.src_id, e.dst_id, ret, wiredtiger_strerror(ret))
    // }

    // cur_adj->set_key(cur_adj, e.src_id, e.dst_id);
    // cur_adj->set_value(cur_adj, 0);
    // if ((ret = cur_adj->insert(cur_adj)) != 0)
    // {
    //     PRINT_ERROR(e.src_id, e.dst_id, ret, wiredtiger_strerror(ret))
    // }

    // cur_ekey->set_key(cur_ekey, e.src_id, e.dst_id);
    // cur_ekey->set_value(cur_ekey, 0, OutOfBand_Val);
    // if ((ret = cur_ekey->insert(cur_ekey)) != 0)
    // {
    //     PRINT_ERROR(e.src_id, e.dst_id, ret, wiredtiger_strerror(ret))
    // }
    // }

    // cur_std->close(cur_std);
    // cur_adj->close(cur_adj);
    // cur_ekey->close(cur_ekey);

    // sess_adj->close(sess_adj, NULL);
    // sess_ekey->close(sess_ekey, NULL);
    // sess_std->close(sess_std, NULL);

    return info;
}

int main(int argc, char *argv[])
{
    InsertOpts params(argc, argv);
    if (!params.parse_args())
    {
        params.print_help();
        return -1;
    }

    std::string middle;
    if (params.is_read_optimize())
    {
        middle += "r";
    }
    if (params.is_directed())
    {
        middle += "d";
    }
    dataset = params.get_dataset();
    read_optimized = params.is_read_optimize();
    std::string _db_name;
    std::string conn_config = "create,cache_size=10GB";
#ifdef STAT
    std::string stat_config =
        "statistics=(all),statistics_log=(wait=0,on_close=true";
    conn_config += "," + stat_config;
#endif
    num_per_chunk = (int)(params.get_num_edges() / NUM_THREADS);
    // open std connection
    type_opt = "all";
    if (type_opt == "all" || type_opt == "std")
    {
        _db_name = params.get_db_path() + "/std_" + middle + "_" +
                   params.get_db_name();
        if (wiredtiger_open(_db_name.c_str(),
                            NULL,
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
        _db_name = params.get_db_path() + "/adj_" + middle + "_" +
                   params.get_db_name();
        if (wiredtiger_open(_db_name.c_str(),
                            NULL,
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
        _db_name = params.get_db_path() + "/ekey_" + middle + "_" +
                   params.get_db_name();
        if (wiredtiger_open(_db_name.c_str(),
                            NULL,
                            const_cast<char *>(conn_config.c_str()),
                            &conn_ekey) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    Times t;
    t.start();
    auto *edge_times = new time_info;
    auto *node_times = new time_info;
#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < 1; i++)
    {
        time_info *this_thread_time = insert_edge_thread(i);
        edge_times->insert_time += this_thread_time->insert_time;
        edge_times->num_inserted += this_thread_time->num_inserted;
        edge_times->read_time += this_thread_time->read_time;
    }

    reader::AdjReader adj_reader(
        "/drives/hdd_main/s10_e8/graph_s10_e8_edgesaa_out");

    std::pair<int, std::vector<node_id_t>> node_adj_list;
    int ret = 0;
    do
    {
        ret = adj_reader.get_next_adjlist(node_adj_list);
        if (!ret)
        {
            std::cout << "node: " << node_adj_list.first << std::endl;
            for (auto &dst : node_adj_list.second)
            {
                std::cout << dst << " ";
            }
            std::cout << std::endl;
        }

    } while (ret == 0);

    if (type_opt == "all" || type_opt == "std")
    {
        conn_std->close(conn_std, NULL);
    }
    if (type_opt == "all" || type_opt == "adj")
    {
        conn_adj->close(conn_adj, NULL);
    }
    if (type_opt == "all" || type_opt == "ekey")
    {
        conn_ekey->close(conn_ekey, NULL);
    }
    return (EXIT_SUCCESS);
}