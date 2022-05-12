#include "common.h"
//#include "mio.hpp"
#include <getopt.h>
#include <math.h>
#include <omp.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "bulk_insert.h"
#include "reader.h"

WT_CONNECTION *conn_std, *conn_adj, *conn_ekey;
double num_edges;
double num_nodes;
std::string dataset;
int read_optimized = 0;
int is_directed = 1;

std::vector<std::string> types;
std::unordered_map<int, node> nodelist;
std::unordered_map<int, std::vector<int>> in_adjlist;
std::unordered_map<int, std::vector<int>> out_adjlist;
std::mutex lock;

int nodelist_size;

std::string pack_int_to_str(int a, int b)
{
    std::stringstream sstream;
    sstream << a << " " << b;
    return sstream.str();
}

typedef struct time_info
{
    int64_t insert_time;
    int64_t read_time;
    int num_inserted;
    time_info(int _val)
        : insert_time(_val), read_time(_val), num_inserted(_val){};
} time_info;

void print_time_csvline(std::string db_name,
                        std::string db_path,
                        time_info *edget,
                        time_info *nodet,
                        bool is_readopt,
                        std::string logfile_name,
                        std::string type)
{
    std::ofstream log_file;
    log_file.open(logfile_name, std::fstream::app);

    struct stat st;
    stat(logfile_name.c_str(), &st);
    if (st.st_size == 0)
    {
        log_file << "db_name, db_path, type, is_readopt, num_nodes, num_edges, "
                    "t_e_read, t_e_insert, t_n_read, t_n_insert\n";
    }

    log_file << db_name << "," << db_path << "/" << db_name << "," << type
             << "," << is_readopt << "," << nodet->num_inserted << ","
             << edget->num_inserted << "," << edget->read_time << ","
             << edget->insert_time << "," << nodet->read_time << ","
             << nodet->insert_time << "\n";
    log_file.close();
}

time_info *insert_edge_thread(int _tid)
{
    int tid = _tid;  //*(int *)arg;
    std::string filename = dataset + "_edges";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);
    time_info *info = new time_info(0);

    auto start = std::chrono::steady_clock::now();
    std::vector<edge> edjlist = reader::parse_edge_entries(filename);
    auto end = std::chrono::steady_clock::now();
    info->read_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count();
    info->num_inserted = edjlist.size();

    WT_CURSOR *cursor;
    WT_SESSION *session;
    start = std::chrono::steady_clock::now();
    for (std::string type : types)
    {
        if (type == "std")
        {
            conn_std->open_session(conn_std, NULL, NULL, &session);
            session->open_cursor(session, "table:edge", NULL, NULL, &cursor);

            for (edge e : edjlist)
            {
                cursor->set_key(cursor, e.src_id, e.dst_id);
                cursor->set_value(cursor, e.edge_weight);
                cursor->insert(cursor);
            }

            cursor->close(cursor);
            session->close(session, NULL);
        }
        else if (type == "adj")
        {
            conn_adj->open_session(conn_adj, NULL, NULL, &session);
            session->open_cursor(session, "table:edge", NULL, NULL, &cursor);

            for (edge e : edjlist)
            {
                cursor->set_key(cursor, e.src_id, e.dst_id);
                cursor->set_value(cursor, 0);
                cursor->insert(cursor);
            }
            cursor->close(cursor);
            session->close(session, NULL);
        }
        else if (type == "ekey")
        {
            conn_ekey->open_session(conn_ekey, NULL, NULL, &session);
            session->open_cursor(session, "table:edge", NULL, NULL, &cursor);

            for (edge e : edjlist)
            {
                cursor->set_key(cursor, e.src_id, e.dst_id);
                cursor->set_value(cursor, "");
                cursor->insert(cursor);
            }
            cursor->close(cursor);
            session->close(session, NULL);
        }
    }
    end = std::chrono::steady_clock::now();
    info->insert_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count();
    return info;
}

time_info *insert_node(int _tid)
{
    int tid = _tid;  //*(int *)arg;
    std::string filename = dataset + "_nodes";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    time_info *info = new time_info(0);
    auto start = std::chrono::steady_clock::now();
    std::vector<node> nodes = reader::parse_node_entries(filename);
    auto end = std::chrono::steady_clock::now();
    info->read_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count();
    info->num_inserted = nodes.size();

    std::string type;
    WT_CURSOR *std_cur, *ekey_cur, *adj_cur, *adj_incur, *adj_outcur;
    WT_SESSION *std_sess, *ekey_sess, *adj_sess;

    for (std::string type : types)
    {
        if (type.compare("all") == 0 || type.compare("std") == 0)
        {
            conn_std->open_session(conn_std, NULL, NULL, &std_sess);
            std_sess->open_cursor(std_sess, "table:node", NULL, NULL, &std_cur);
        }

        if (type.compare("all") == 0 || type.compare("ekey") == 0)
        {
            conn_ekey->open_session(conn_ekey, NULL, NULL, &ekey_sess);
            ekey_sess->open_cursor(
                ekey_sess, "table:edge", NULL, NULL, &ekey_cur);
        }

        if (type.compare("all") == 0 || type.compare("adj") == 0)
        {
            conn_adj->open_session(conn_adj, NULL, NULL, &adj_sess);
            adj_sess->open_cursor(adj_sess, "table:node", NULL, NULL, &adj_cur);
            adj_sess->open_cursor(
                adj_sess, "table:adjlistin", NULL, NULL, &adj_incur);
            adj_sess->open_cursor(
                adj_sess, "table:adjlistout", NULL, NULL, &adj_outcur);
        }
        auto start = std::chrono::steady_clock::now();
        for (node to_insert : nodes)
        {
            if (type == "std")
            {
                std_cur->set_key(std_cur, to_insert.id);

                if (read_optimized)
                {
                    std_cur->set_value(
                        std_cur, to_insert.in_degree, to_insert.out_degree);
                }
                else
                {
                    std_cur->set_value(std_cur, "");
                }

                std_cur->insert(std_cur);
            }
            else if (type == "ekey")
            {
                ekey_cur->set_key(ekey_cur, to_insert.id, -1);
                if (read_optimized)
                {
                    std::string packed = pack_int_to_str(to_insert.in_degree,
                                                         to_insert.out_degree);
                    ekey_cur->set_value(ekey_cur, packed.c_str());
                }
                else
                {
                    ekey_cur->set_value(ekey_cur, "");
                }

                ekey_cur->insert(ekey_cur);
            }
            else if (type == "adj")
            {
                adj_cur->set_key(adj_cur, to_insert.id);
                if (read_optimized)
                {
                    adj_cur->set_value(
                        adj_cur, to_insert.in_degree, to_insert.out_degree);
                }
                else
                {
                    adj_cur->set_value(adj_cur, "");
                }

                adj_cur->insert(adj_cur);

                // Now insert into in and out tables.
                adj_incur->set_key(adj_incur, to_insert.id);
                adj_outcur->set_key(adj_outcur, to_insert.id);
                // size_t size;
                try
                {
                    WT_ITEM item;
                    item.data = CommonUtil::pack_int_vector_wti(
                        adj_sess, in_adjlist.at(to_insert.id), &item.size);
                    adj_incur->set_value(adj_incur, to_insert.in_degree, &item);
                }
                catch (const std::out_of_range &oor)
                {
                    WT_ITEM item = {.data = {}, .size = 0};  // todo: check
                    adj_incur->set_value(adj_incur, 0, &item);
                }

                try
                {
                    // std::string packed_outlist =
                    // CommonUtil::pack_int_vector_std(out_adjlist.at(to_insert.id),
                    // &size);
                    WT_ITEM item;
                    item.data = CommonUtil::pack_int_vector_wti(
                        adj_sess, out_adjlist.at(to_insert.id), &item.size);
                    adj_outcur->set_value(
                        adj_outcur, to_insert.out_degree, &item);
                }
                catch (const std::out_of_range &oor)
                {
                    WT_ITEM item = {.data = {}, .size = 0};
                    adj_outcur->set_value(adj_outcur, 0, &item);
                }

                adj_incur->insert(adj_incur);
                adj_outcur->insert(adj_outcur);
            }
        }
        auto end = std::chrono::steady_clock::now();
        info->insert_time =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                .count();
        if (type.compare("all") == 0 || type.compare("std") == 0)
        {
            std_cur->close(std_cur);
            std_sess->close(std_sess, NULL);
        }
        if (type.compare("all") == 0 || type.compare("ekey") == 0)
        {
            ekey_cur->close(ekey_cur);
            ekey_sess->close(ekey_sess, NULL);
        }
        if (type.compare("all") == 0 || type.compare("adj") == 0)
        {
            adj_cur->close(adj_cur);
            adj_sess->close(adj_sess, NULL);
        }
    }

    // delete (int *)arg;

    return info;
}

int main(int argc, char *argv[])
{
    std::string db_name;
    std::string db_path;
    std::string type_opt;
    std::string logfile;
    static struct option long_opts[] = {
        {"db", required_argument, 0, 'd'},
        {"path", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'},
        {"edges", required_argument, 0, 'e'},
        {"nodes", required_argument, 0, 'n'},
        {"file", required_argument, 0, 'f'},
        {"type", required_argument, 0, 't'},
        {"undirected", required_argument, &is_directed, 'u'},
        {"ropt", required_argument, &read_optimized, 'r'}};
    int option_idx = 0;
    int c;

    while ((c = getopt_long(
                argc, argv, "d:p:l:e:n:f:t:ru", long_opts, &option_idx)) != -1)
    {
        switch (c)
        {
            case 'd':
                db_name = optarg;
                break;
            case 'p':
                db_path = optarg;
                break;
            case 'l':
                logfile = optarg;
                break;
            case 'e':
                num_edges = atol(optarg);
                break;
            case 'n':
                num_nodes = atol(optarg);
                break;
            case 'f':
                dataset = optarg;
                break;
            case 't':
            {
                type_opt = optarg;
                if (type_opt.compare("all") == 0)
                {
                    types.push_back("std");
                    types.push_back("adj");
                    types.push_back("ekey");
                }
                else
                {
                    types.push_back(type_opt);
                }
                break;
            }
            case 'r':
                read_optimized = 1;
                break;
            case 'u':
                is_directed = 0;
                break;
            case ':':
            /* missing option argument */
            case '?':
            default:
                std::cout
                    << "enter dbname base name(--db), edgecount (--edges), "
                       "nodecount (--nodes), and dataset file (--file), "
                       "undirected (--undirected), read_opt (--ropt)";
                exit(0);
        }
    }

    std::string middle;
    if (read_optimized)
    {
        middle += "r";
    }
    if (is_directed)
    {
        middle += "d";
    }

    std::string _db_name;
    std::string conn_config = "create,cache_size=10GB";
#ifdef STAT
    std::string stat_config =
        "statistics=(all),statistics_log=(wait=0,on_close=true";
    conn_config += "," + stat_config;
#endif
    // open std connection
    if (type_opt.compare("all") == 0 || type_opt.compare("std") == 0)
    {
        _db_name = db_path + "/std_" + middle + "_" + db_name;
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
    if (type_opt.compare("all") == 0 || type_opt.compare("adj") == 0)
    {
        _db_name = db_path + "/adj_" + middle + "_" + db_name;
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
    if (type_opt.compare("all") == 0 || type_opt.compare("ekey") == 0)
    {
        _db_name = db_path + "/ekey_" + middle + "_" + db_name;
        if (wiredtiger_open(_db_name.c_str(),
                            NULL,
                            const_cast<char *>(conn_config.c_str()),
                            &conn_ekey) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    int i;
    // pthread_t threads[NUM_THREADS];

    auto start = std::chrono::steady_clock::now();
    time_info *edge_times = new time_info(0);
    time_info *node_times = new time_info(0);
#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        time_info *this_thread_time = insert_edge_thread(i);
        edge_times->insert_time += this_thread_time->insert_time;
        edge_times->num_inserted += this_thread_time->num_inserted; //open cursor to metadata table, insert key "numNodes/Edges", conversion
        edge_times->read_time += this_thread_time->read_time;
    }

    auto end = std::chrono::steady_clock::now();
    std::cout << " Total time to insert edges was "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << std::endl;

    // Now insert nodes;
    start = std::chrono::steady_clock::now();
#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        time_info *this_thread_time = insert_node(i);
        node_times->insert_time += this_thread_time->insert_time;
        node_times->num_inserted += this_thread_time->num_inserted;
        node_times->read_time += this_thread_time->read_time;
    }

    end = std::chrono::steady_clock::now();
    std::cout << " Total time to insert nodes was "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << std::endl;

    // Adjust for threads
    node_times->insert_time = node_times->insert_time / NUM_THREADS;
    node_times->read_time = node_times->read_time / NUM_THREADS;
    edge_times->insert_time = edge_times->insert_time / NUM_THREADS;
    edge_times->read_time = edge_times->read_time / NUM_THREADS;

    std::cout << "total inserted edges = " << edge_times->num_inserted
              << " num_edges = " << num_edges << std::endl;
    std::cout << "time to insert edges " << edge_times->insert_time
              << std::endl;
    std::cout << "time to read edges " << edge_times->read_time << std::endl;

    std::cout << "total inserted nodes  = " << node_times->num_inserted
              << " num_nodes = " << num_nodes << std::endl;
    std::cout << "time to insert_nodes " << node_times->insert_time
              << std::endl;
    std::cout << "time to read nodes " << node_times->read_time << std::endl;

    print_time_csvline(db_name,
                       db_path,
                       edge_times,
                       node_times,
                       read_optimized,
                       logfile,
                       type_opt);

    if (type_opt.compare("all") == 0 || type_opt.compare("std") == 0)
    {
        conn_std->close(conn_std, NULL);
    }
    if (type_opt.compare("all") == 0 || type_opt.compare("adj") == 0)
    {
        conn_adj->close(conn_adj, NULL);
    }
    if (type_opt.compare("all") == 0 || type_opt.compare("ekey") == 0)
    {
        conn_ekey->close(conn_ekey, NULL);
    }

    return (EXIT_SUCCESS);
}