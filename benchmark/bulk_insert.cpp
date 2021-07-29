#include "common.h"
//#include "mio.hpp"
#include "reader.h"
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

#define NUM_THREADS 10

WT_CONNECTION *conn;
double num_edges;
double num_nodes;
std::string dataset;
int edge_per_part, node_per_part;
int read_optimized = 0;
int is_directed = 1;
std::string type;

typedef struct thread_params
{
    std::string filename;
    int tid;
} thread_params;

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

    WT_CURSOR *cursor;
    WT_SESSION *session;

    conn->open_session(conn, NULL, NULL, &session);
    session->open_cursor(session, "table:edge", NULL, NULL, &cursor);

    int start_idx = (edge_per_part * tid) + 1;

    for (edge e : edjlist)
    {

        if (type == "ekey")
        {
            cursor->set_key(cursor, e.src_id, e.dst_id);
            cursor->set_value(cursor, "");
        }
        else if (type == "std")
        {
            cursor->set_key(cursor, start_idx);
            cursor->set_value(cursor, e.src_id, e.dst_id, 0);
        }
        else if (type == "adj")
        {

            cursor->set_key(cursor, e.src_id, e.dst_id);
            cursor->set_value(cursor, e.src_id, e.dst_id, 0);
        }

        cursor->insert(cursor);
        start_idx++;
    }
    std::cout << tid << "\t" << filename << "\t" << (edge_per_part * tid) + 1 << "\t" << start_idx << "\t" << size << std::endl;
    delete (int *)arg;

    return (void *)(size);
}

int insert_node()
{
    std::string filename = dataset;
    filename += "_nodes";

    std::unordered_map<int, node> nodelist = reader::parse_node_entries(filename);
    int size = nodelist.size();

    WT_CURSOR *cursor;
    WT_SESSION *session;

    conn->open_session(conn, NULL, NULL, &session);
    if (type == "ekey")
    {
        session->open_cursor(session, "table:edge", NULL, NULL, &cursor);
    }
    else
    {
        session->open_cursor(session, "table:node", NULL, NULL, &cursor);
    }
    int cnt = 0;
    for (auto &entry : nodelist)
    {
        node n = entry.second;
        if (type == "ekey")
        {
            cursor->set_key(cursor, n.id, -1);
            cursor->set_value(cursor, "");
        }
        else
        {
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
            cnt++;
        }
    }
    std::cout << "inserted into nodes :" << cnt << std::endl;
    return size;
}

void *insert_inadjlist(void *arg)
{
    WT_CURSOR *cursor;
    WT_SESSION *session;
    int tid = *(int *)arg;
    int start_idx = (node_per_part * tid) + 1;
    int end_idx = (node_per_part * (tid + 1));

    conn->open_session(conn, NULL, NULL, &session);
    session->open_cursor(session, "table:adjlistin", NULL, NULL, &cursor);
    int cnt = 0;
    for (auto &entry : reader::in_adjlist)
    {
        int node_id = entry.first;
        int indeg = entry.second.size();
        size_t size;
        std::string packed_adjlist = CommonUtil::pack_int_vector_std(entry.second, &size);
        cursor->set_key(cursor, node_id);
        cursor->set_value(cursor, indeg, packed_adjlist);
    }
    cursor->close(cursor);
    session->close(session, NULL);
}

void *insert_outadjlist(void *arg)
{
    WT_CURSOR *cursor;
    WT_SESSION *session;

    conn->open_session(conn, NULL, NULL, &session);
    session->open_cursor(session, "table:adjlistout", NULL, NULL, &cursor);
    int cnt = 0;
    for (auto &entry : reader::in_adjlist)
    {
        int node_id = entry.first;
        int outdeg = entry.second.size();
        size_t size;
        std::string packed_adjlist = CommonUtil::pack_int_vector_std(entry.second, &size);
        cursor->set_key(cursor, node_id);
        cursor->set_value(cursor, outdeg, packed_adjlist);
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

    std::cout << "edges per part : " << edge_per_part << std::endl;

    for (std::string prefix : {"ekey"}) //"std", "adj",
    {
        type = prefix;
        std::string middle;
        if (read_optimized)
        {
            middle += "r";
        }
        if (is_directed)
        {
            middle += "d";
        }

        std::string _db_name = "db/" + prefix + "_" + middle + "_" + db_name;
        if (wiredtiger_open(_db_name.c_str(), NULL, "create", &conn) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
        int i;
        pthread_t threads[NUM_THREADS];

        //Insert Edges First;
        std::cout << "id \t filename \t starting index\t ending idx\t size" << std::endl;
        auto start = std::chrono::steady_clock::now();
        for (i = 0; i < NUM_THREADS; i++)
        {
            pthread_create(&threads[i], NULL, insert_edge_thread, new int(i));
        }
        int edge_total = 0;
        int node_total = 0;
        // for (i = 0; i < NUM_THREADS; i++)
        // {
        //     void *found;
        //     pthread_join(threads[i], &found);
        //     edge_total = edge_total + (intptr_t)found;
        // }
        auto end = std::chrono::steady_clock::now();
        // std::cout << "Edges inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

        //Now insert nodes;
        start = std::chrono::steady_clock::now();
        node_total = insert_node();
        end = std::chrono::steady_clock::now();
        std::cout << "Nodes inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

        //Now create the adjlist, if needed
        //node_per_part = ceil(num_nodes / NUM_THREADS);

        std::cout << reader::in_adjlist.bucket_count() << "\t" << reader::out_adjlist.bucket_count();

        // if (prefix == "adj")
        // {
        //     //insert in_Adjlist first
        //     start = std::chrono::steady_clock::now();
        //     int in_adj_total = 0, out_adj_total = 0;
        //     for (i = 0; i < NUM_THREADS; i++)
        //     {
        //         pthread_create(&threads[i], NULL, insert_inadjlist, new int(i));
        //     }

        //     for (i = 0; i < NUM_THREADS; i++)
        //     {
        //         void *found;
        //         pthread_join(threads[i], &found);
        //         in_adj_total += in_adj_total + (intptr_t)found;
        //     }

        //     end = std::chrono::steady_clock::now();
        //     std::cout << "InAdjList inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

        //     //Insert out_Adjlist now
        //     start = std::chrono::steady_clock::now();
        //     int in_adj_total = 0, out_adj_total = 0;
        //     for (i = 0; i < NUM_THREADS; i++)
        //     {
        //         pthread_create(&threads[i], NULL, insert_outadjlist, new int(i));
        //     }

        //     for (i = 0; i < NUM_THREADS; i++)
        //     {
        //         void *found;
        //         pthread_join(threads[i], &found);
        //         out_adj_total += out_adj_total + (intptr_t)found;
        //     }

        //     end = std::chrono::steady_clock::now();
        //     std::cout << "OutAdjList inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
        // }

        std::cout << "total number of edges inserted = " << edge_total << std::endl;
        std::cout << "total number of nodes inserted = " << node_total << std::endl;
        conn->close(conn, NULL);
    }

    return (EXIT_SUCCESS);
}
