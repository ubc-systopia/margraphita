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

void *insert_edge_thread(void *arg)
{

    int tid = *(int *)arg;
    std::string filename = dataset + "_edges";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    std::vector<edge>
        edjlist = reader::parse_edge_entries(filename);
    int size = edjlist.size();

    // = (edge_per_part * tid) + 1;
    WT_CURSOR *cursor;
    WT_SESSION *session;

    for (std::string type : {"std", "adj", "ekey"})
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
                cursor->set_value(cursor, e.src_id, e.dst_id, 0);
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

    //std::cout << tid << "\t" << filename << "\t" << (edge_per_part * tid) + 1 << "\t" << start_idx << "\t" << size << std::endl;
    delete (int *)arg;

    return (void *)(size);
}

void *insert_node(void *arg)
{

    int tid = *(int *)arg;
    std::string filename = dataset + "_nodes";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    std::vector<node>
        nodes = reader::parse_node_entries(filename);
    std::string type;
    WT_CURSOR *std_cur, *ekey_cur, *adj_cur, *adj_incur, *adj_outcur;
    WT_SESSION *std_sess, *ekey_sess, *adj_sess;

    conn_std->open_session(conn_std, NULL, NULL, &std_sess);
    std_sess->open_cursor(std_sess, "table:node", NULL, NULL, &std_cur);

    conn_ekey->open_session(conn_ekey, NULL, NULL, &ekey_sess);
    ekey_sess->open_cursor(ekey_sess, "table:edge", NULL, NULL, &ekey_cur);

    conn_adj->open_session(conn_adj, NULL, NULL, &adj_sess);
    adj_sess->open_cursor(adj_sess, "table:node", NULL, NULL, &adj_cur);
    adj_sess->open_cursor(adj_sess, "table:adjlistin", NULL, NULL, &adj_incur);
    adj_sess->open_cursor(adj_sess, "table:adjlistout", NULL, NULL, &adj_outcur);

    for (node to_insert : nodes)
    {
        for (std::string type : {"std", "adj", "ekey"})
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
    }

    std_cur->close(std_cur);
    std_sess->close(std_sess, NULL);

    ekey_cur->close(ekey_cur);
    ekey_sess->close(ekey_sess, NULL);

    adj_cur->close(adj_cur);
    adj_sess->close(adj_sess, NULL);

    delete (int *)arg;

    return (void *)(nodes.size());
}

int main(int argc, char *argv[])
{
    std::string db_name;
    std::string db_path;
    static struct option long_opts[] = {
        {"db", required_argument, 0, 'd'},
        {"path", required_argument, 0, 'p'},
        {"edges", required_argument, 0, 'e'},
        {"nodes", required_argument, 0, 'n'},
        {"file", required_argument, 0, 'f'},
        {"undirected", required_argument, &is_directed, 'u'},
        {"ropt", required_argument, &read_optimized, 'r'}};
    int option_idx = 0;
    int c;

    while ((c = getopt_long(argc, argv, "d:p:e:n:f:ru", long_opts, &option_idx)) != -1)
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

    //open std connection
    std::string _db_name = db_path + "/std_" + middle + "_" + db_name;
    if (wiredtiger_open(_db_name.c_str(), NULL, "create", &conn_std) != 0)
    {
        std::cout << "Could not open the DB: " << _db_name;
        exit(1);
    }

    //open adjlist connection
    _db_name = db_path + "/adj_" + middle + "_" + db_name;
    if (wiredtiger_open(_db_name.c_str(), NULL, "create", &conn_adj) != 0)
    {
        std::cout << "Could not open the DB: " << _db_name;
        exit(1);
    }

    //open ekey connection
    _db_name = db_path + "/ekey_" + middle + "_" + db_name;
    if (wiredtiger_open(_db_name.c_str(), NULL, "create", &conn_ekey) != 0)
    {
        std::cout << "Could not open the DB: " << _db_name;
        exit(1);
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
    int edge_total = 0;
    int node_total = 0;
    for (i = 0; i < NUM_THREADS; i++)
    {
        void *found;
        pthread_join(threads[i], &found);
        edge_total = edge_total + (intptr_t)found;
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << edge_total << " edges inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    std::vector<std::string> prefix = {"std", "adj", "ekey"};

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
        node_total = node_total + (intptr_t)found;
    }
    end = std::chrono::steady_clock::now();
    std::cout << node_total << " nodes inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    // //insert in_Adjlist first
    // start = std::chrono::steady_clock::now();
    // int in_adj_total = 0, out_adj_total = 0;
    // insert_inadjlist();
    // end = std::chrono::steady_clock::now();
    // std::cout << "InAdjList inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    // //Insert out_Adjlist now
    // start = std::chrono::steady_clock::now();
    // insert_outadjlist();
    // end = std::chrono::steady_clock::now();
    // std::cout << "OutAdjList inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    std::cout << "total number of edges inserted = " << edge_total << std::endl;
    std::cout << "total number of nodes inserted = " << node_total << std::endl;
    conn_std->close(conn_std, NULL);
    conn_adj->close(conn_adj, NULL);
    conn_ekey->close(conn_ekey, NULL);
    std::cout << "------------------------------------------------------------------------------------" << std::endl;

    return (EXIT_SUCCESS);
}
