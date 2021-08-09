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

    //int start_idx = (edge_per_part * tid) + 1;
    return (void *)(size);
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

    char *impl = (char *)arg;
    std::string type(impl);
    WT_CURSOR *cursor;
    WT_SESSION *session;

    if (type == "std")
    {
        conn_std->open_session(conn_std, NULL, NULL, &session);
        session->open_cursor(session, "table:node", NULL, NULL, &cursor);
        for (auto &entry : nodelist)
        {
            node n = entry.second;
            cursor->set_key(cursor, n.id);

            if (read_optimized)
            {
                cursor->set_value(cursor, n.in_degree, n.out_degree);
            }
            else
            {
                cursor->set_value(cursor, 0, 0);
            }

            cursor->insert(cursor);
        }
        cursor->close(cursor);
        session->close(session, NULL);
    }
    else if (type == "ekey")
    {
        conn_ekey->open_session(conn_ekey, NULL, NULL, &session);
        session->open_cursor(session, "table:edge", NULL, NULL, &cursor);
        for (auto &entry : nodelist)
        {
            node n = entry.second;
            cursor->set_key(cursor, n.id, -1);
            cursor->set_value(cursor, "");
            cursor->insert(cursor);
        }
        cursor->close(cursor);
        session->close(session, NULL);
    }
    else if (type == "adj")
    {
        conn_adj->open_session(conn_adj, NULL, NULL, &session);
        session->open_cursor(session, "table:node", NULL, NULL, &cursor);
        for (auto &entry : nodelist)
        {
            node n = entry.second;
            cursor->set_key(cursor, n.id, -1);
            cursor->set_value(cursor, "");
            cursor->insert(cursor);
        }
        cursor->close(cursor);
        session->close(session, NULL);
    }
}

void insert_inadjlist()
{
    WT_CURSOR *cursor;
    WT_SESSION *session;

    conn_adj->open_session(conn_adj, NULL, NULL, &session);
    session->open_cursor(session, "table:adjlistin", NULL, NULL, &cursor);
    int cnt = 0;
    for (auto &entry : in_adjlist)
    {
        int node_id = entry.first;
        int indeg = entry.second.size();
        size_t size;
        std::string packed_adjlist = CommonUtil::pack_int_vector_std(entry.second, &size);
        cursor->set_key(cursor, node_id);
        cursor->set_value(cursor, indeg, packed_adjlist);
        cursor->insert(cursor);
    }
    cursor->close(cursor);
    session->close(session, NULL);
}

void insert_outadjlist()
{
    WT_CURSOR *cursor;
    WT_SESSION *session;

    conn_adj->open_session(conn_adj, NULL, NULL, &session);
    session->open_cursor(session, "table:adjlistout", NULL, NULL, &cursor);
    int cnt = 0;
    for (auto &entry : out_adjlist)
    {
        int node_id = entry.first;
        int outdeg = entry.second.size();
        size_t size;
        std::string packed_adjlist = CommonUtil::pack_int_vector_std(entry.second, &size);
        cursor->set_key(cursor, node_id);
        cursor->set_value(cursor, outdeg, packed_adjlist);
        cursor->insert(cursor);
    }
    cursor->close(cursor);
    session->close(session, NULL);
}

int main(int argc, char *argv[])
{
    std::string db_name;
    static struct option long_opts[] = {
        {"db", required_argument, 0, 'd'},
        {"edges", required_argument, 0, 'e'},
        {"nodes", required_argument, 0, 'n'},
        {"file", required_argument, 0, 'f'},
        {"undirected", required_argument, &is_directed, 'u'},
        {"ropt", required_argument, &read_optimized, 'r'}};
    int option_idx = 0;
    int c;

    while ((c = getopt_long(argc, argv, "d:e:n:f:ru", long_opts, &option_idx)) != -1)
    {
        switch (c)
        {
        case 'd':
            db_name = optarg;
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
    std::string _db_name = "db/std_" + middle + "_" + db_name;
    if (wiredtiger_open(_db_name.c_str(), NULL, "create", &conn_std) != 0)
    {
        std::cout << "Could not open the DB: " << _db_name;
        exit(1);
    }

    //open adjlist connection
    _db_name = "db/adj_" + middle + "_" + db_name;
    if (wiredtiger_open(_db_name.c_str(), NULL, "create", &conn_adj) != 0)
    {
        std::cout << "Could not open the DB: " << _db_name;
        exit(1);
    }

    //open ekey connection
    _db_name = "db/ekey_" + middle + "_" + db_name;
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
    std::cout << "Edges inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    std::vector<std::string> prefix = {"adj"}; //{"std", "adj", "ekey"};

    //Now insert nodes;

    //read the file for nodes and adjlist
    std::string filename = dataset;
    filename += "_nodes";
    nodelist = reader::parse_node_entries(filename);
    nodelist_size = nodelist.size();

    start = std::chrono::steady_clock::now();
    for (i = 0; i < 1; i++)
    {
        pthread_create(&threads[i], NULL, insert_node, (void *)(prefix.at(i).c_str()));
    }
    for (i = 0; i < 1; i++)
    {
        void *found;
        pthread_join(threads[i], &found);
    }
    end = std::chrono::steady_clock::now();
    std::cout << "Nodes inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    //insert in_Adjlist first
    start = std::chrono::steady_clock::now();
    int in_adj_total = 0, out_adj_total = 0;
    insert_inadjlist();
    end = std::chrono::steady_clock::now();
    std::cout << "InAdjList inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    //Insert out_Adjlist now
    start = std::chrono::steady_clock::now();
    insert_outadjlist();
    end = std::chrono::steady_clock::now();
    std::cout << "OutAdjList inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    std::cout << "total number of edges inserted = " << edge_total << std::endl;
    std::cout << "total number of nodes inserted = " << node_total << std::endl;
    conn_std->close(conn_std, NULL);
    conn_adj->close(conn_adj, NULL);
    conn_ekey->close(conn_ekey, NULL);
    std::cout << "------------------------------------------------------------------------------------" << std::endl;

    return (EXIT_SUCCESS);
}
