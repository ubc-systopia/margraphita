#include "common.h"
//#include "mio.hpp"
#include <thread>
#include <atomic>
//#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <chrono>
#include <unistd.h>
#include "bulk_insert.h"
#include "reader.h"

WT_CONNECTION *conn_std, *conn_adj, *conn_ekey;
double num_edges;
double num_nodes;
std::string dataset;
int edge_per_part, node_per_part;
int read_optimized = 0;
int is_directed = 1;

std::vector<std::string> types;
std::unordered_map<int, node> nodelist;
std::unordered_map<int, std::vector<int>> in_adjlist;
std::unordered_map<int, std::vector<int>> out_adjlist;
std::mutex lock;

int nodelist_size;

std::string
pack_int_to_str(int a, int b)
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
    time_info(int _val) : insert_time(_val), read_time(_val), num_inserted(_val){};
} time_info;

void print_time_csvline(std::string db_name, std::string db_path, time_info *edget, time_info *nodet)
{
    std::ofstream log_file;
    log_file.open("new_kron_insert.csv", std::fstream::app);
    log_file << "db_name,db_path, num_nodes, num_edges, t_e_read, t_e_insert, t_n_read, t_n_insert\n";
    log_file
        << db_name << ","
        << db_path << ","
        << db_name << ","
        << nodet->num_inserted << ","
        << edget->num_inserted << ","
        << edget->read_time << ","
        << edget->insert_time << ","
        << nodet->read_time << ","
        << nodet->insert_time << "\n";
    log_file.close();
}

void *insert_edge_thread(void *arg)
{

    int tid = *(int *)arg;
    std::string filename = dataset + "_edges";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);
    time_info *info = new time_info(0);

    auto start = std::chrono::steady_clock::now();
    std::vector<edge>
        edjlist = reader::parse_edge_entries(filename);
    auto end = std::chrono::steady_clock::now();
    info->read_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    info->num_inserted = edjlist.size();

    WT_CURSOR *cursor;
    WT_SESSION *session;
    start = std::chrono::steady_clock::now();
    for (std::string type : types)
    {
        int start_idx = (edge_per_part * tid) + 1;
        if (type == "std")
        {
            conn_std->open_session(conn_std, NULL, NULL, &session);
            session->open_cursor(session, "table:edge", NULL, NULL, &cursor);

            for (edge e : edjlist)
            {
                cursor->set_key(cursor, start_idx);
                cursor->set_value(cursor, e.src_id, e.dst_id, 0);
                cursor->insert(cursor);
                start_idx++;
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
                start_idx++;
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
                start_idx++;
            }
            cursor->close(cursor);
            session->close(session, NULL);
        }
    }
    end = std::chrono::steady_clock::now();
    info->insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    delete (int *)arg;

    return (void *)(info);
}

void *insert_node(void *arg)
{

    int tid = *(int *)arg;
    std::string filename = dataset + "_nodes";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    time_info *info = new time_info(0);
    auto start = std::chrono::steady_clock::now();
    std::vector<node>
        nodes = reader::parse_node_entries(filename);
    auto end = std::chrono::steady_clock::now();
    info->read_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
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
            ekey_sess->open_cursor(ekey_sess, "table:edge", NULL, NULL, &ekey_cur);
        }

        if (type.compare("all") == 0 || type.compare("adj") == 0)
        {
            conn_adj->open_session(conn_adj, NULL, NULL, &adj_sess);
            adj_sess->open_cursor(adj_sess, "table:node", NULL, NULL, &adj_cur);
            adj_sess->open_cursor(adj_sess, "table:adjlistin", NULL, NULL, &adj_incur);
            adj_sess->open_cursor(adj_sess, "table:adjlistout", NULL, NULL, &adj_outcur);
        }
        auto start = std::chrono::steady_clock::now();
        for (node to_insert : nodes)
        {
            if (type == "std")
            {
                std_cur->set_key(std_cur, to_insert.id);

                if (read_optimized)
                {
                    std_cur->set_value(std_cur, to_insert.in_degree, to_insert.out_degree);
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
                    std::string packed = pack_int_to_str(to_insert.in_degree, to_insert.out_degree);
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
                    adj_cur->set_value(adj_cur, to_insert.in_degree, to_insert.out_degree);
                }
                else
                {
                    adj_cur->set_value(adj_cur, "");
                }

                adj_cur->insert(adj_cur);

                //Now insert into in and out tables.
                adj_incur->set_key(adj_incur, to_insert.id);
                adj_outcur->set_key(adj_outcur, to_insert.id);
                size_t size;
                try
                {
                    std::string packed_inlist = CommonUtil::pack_int_vector_std(in_adjlist.at(to_insert.id), &size);
                    adj_incur->set_value(adj_incur, to_insert.in_degree, packed_inlist.c_str());
                }
                catch (const std::out_of_range &oor)
                {
                    adj_incur->set_value(adj_incur, 0, "");
                }

                try
                {
                    std::string packed_outlist = CommonUtil::pack_int_vector_std(out_adjlist.at(to_insert.id), &size);
                    adj_outcur->set_value(adj_outcur, to_insert.out_degree, packed_outlist.c_str());
                }
                catch (const std::out_of_range &oor)
                {
                    adj_outcur->set_value(adj_outcur, 0, "");
                }

                adj_incur->insert(adj_incur);
                adj_outcur->insert(adj_outcur);
            }
        }
        auto end = std::chrono::steady_clock::now();
        info->insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
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

    delete (int *)arg;

    return (void *)(info);
}

int main(int argc, char *argv[])
{
    std::string db_name;
    std::string db_path;
    std::string type_opt;
    static struct option long_opts[] = {
        {"db", required_argument, 0, 'd'},
        {"path", required_argument, 0, 'p'},
        {"edges", required_argument, 0, 'e'},
        {"nodes", required_argument, 0, 'n'},
        {"file", required_argument, 0, 'f'},
        {"type", required_argument, 0, 't'},
        {"undirected", required_argument, &is_directed, 'u'},
        {"ropt", required_argument, &read_optimized, 'r'}};
    int option_idx = 0;
    int c;

    while ((c = getopt_long(argc, argv, "d:p:e:n:f:t:ru", long_opts, &option_idx)) != -1)
    {
        switch (c)
        {
        case 'd':
            db_name = optarg;
            break;
        case 'p':
            db_path = optarg;
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
            std::cout << "enter dbname base name(--db), edgecount (--edges), nodecount (--nodes), and dataset file (--file), undirected (--undirected), read_opt (--ropt)";
            exit(0);
        }
    }
    edge_per_part = ceil(num_edges / NUM_THREADS);
    node_per_part = ceil(num_nodes / NUM_THREADS);

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
    //open std connection
    if (type_opt.compare("all") == 0 || type_opt.compare("std") == 0)
    {
        _db_name = db_path + "/std_" + middle + "_" + db_name;
        if (wiredtiger_open(_db_name.c_str(), NULL, "create", &conn_std) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    //open adjlist connection
    if (type_opt.compare("all") == 0 || type_opt.compare("adj") == 0)
    {
        _db_name = db_path + "/adj_" + middle + "_" + db_name;
        if (wiredtiger_open(_db_name.c_str(), NULL, "create", &conn_adj) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    //open ekey connection
    if (type_opt.compare("all") == 0 || type_opt.compare("ekey") == 0)
    {
        _db_name = db_path + "/ekey_" + middle + "_" + db_name;
        if (wiredtiger_open(_db_name.c_str(), NULL, "create", &conn_ekey) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    int i;
    pthread_t threads[NUM_THREADS];

    //Insert Edges First;
    //std::cout << "id \t filename \t starting index\t ending idx\t size" << std::endl;
    auto start = std::chrono::steady_clock::now();
    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, insert_edge_thread, new int(i));
    }
    time_info *edge_times = new time_info(0);
    time_info *node_times = new time_info(0);
    for (i = 0; i < NUM_THREADS; i++)
    {
        void *found;
        pthread_join(threads[i], &found);
        time_info *this_thread_time = (time_info *)found;
        edge_times->insert_time += this_thread_time->insert_time;
        edge_times->num_inserted += this_thread_time->num_inserted;
        edge_times->read_time += this_thread_time->read_time;
    }
    std::cout << " total inserted edges from threads = " << edge_times->num_inserted << " num_edges = " << num_edges << std::endl;
    std::cout << "average time in insert_edges " << edge_times->insert_time / NUM_THREADS << std::endl;
    std::cout << "average read time in insert_edges " << edge_times->read_time / NUM_THREADS << std::endl;
    auto end = std::chrono::steady_clock::now();

    std::cout << " Total time to insert edges from outside was " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    //Now insert nodes;
    start = std::chrono::steady_clock::now();
    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, insert_node, new int(i)); // (void *)(prefix.at(i).c_str()));
    }
    for (i = 0; i < NUM_THREADS; i++)
    {
        void *found;
        pthread_join(threads[i], &found);
        time_info *this_thread_time = (time_info *)found;
        node_times->insert_time += this_thread_time->insert_time;
        node_times->num_inserted += this_thread_time->num_inserted;
        node_times->read_time += this_thread_time->read_time;
    }
    end = std::chrono::steady_clock::now();
    std::cout << " total inserted nodes from threads = " << node_times->num_inserted << " num_nodes = " << num_nodes << std::endl;
    std::cout << "average time in insert_nodes " << node_times->insert_time / NUM_THREADS << std::endl;
    std::cout << "average read time in insert_nodes " << node_times->read_time / NUM_THREADS << std::endl;
    std::cout << " nodes inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    print_time_csvline(db_name, db_path, edge_times, node_times);

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
