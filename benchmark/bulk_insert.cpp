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
int edge_per_part;
int read_optimized = 0;

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
        cursor->set_key(cursor, start_idx);
        cursor->set_value(cursor, e.src_id, e.dst_id, 0);
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
    session->open_cursor(session, "table:node", NULL, NULL, &cursor);
    int cnt = 0;
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
        cnt++;
    }
    std::cout << "inserted into nodes :" << cnt << std::endl;
    return size;
}

int main(int argc, char *argv[])
{
    std::string db_name;
    static struct option long_opts[] = {
        {"db", required_argument, 0, 'd'},
        {"edges", required_argument, 0, 'e'},
        {"nodes", required_argument, 0, 'n'},
        {"file", required_argument, 0, 'f'},
        {"ropt", required_argument, &read_optimized, 'r'}};
    int option_idx = 0;
    int c;

    while ((c = getopt_long(argc, argv, "d:e:n:f:r", long_opts, &option_idx)) != -1)
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
        case ':':
        /* missing option argument */
        case '?':
        default:
            std::cout << "enter dbname(--db), edgecount (--edges), nodecount (--nodes), and dataset file (--file)";
            exit(0);
        }
    }
    edge_per_part = ceil(num_edges / NUM_THREADS);
    std::cout << "edges per part : " << edge_per_part << std::endl;
    wiredtiger_open(db_name.c_str(), NULL, "create", &conn);

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
    for (i = 0; i < NUM_THREADS; i++)
    {
        void *found;
        pthread_join(threads[i], &found);
        edge_total = edge_total + (intptr_t)found;
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "Edges inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    //Now insert nodes;
    start = std::chrono::steady_clock::now();
    node_total = insert_node();
    end = std::chrono::steady_clock::now();
    std::cout << "Nodes inserted in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    std::cout << "total number of edges inserted = " << edge_total << std::endl;
    std::cout << "total number of nodes inserted = " << node_total << std::endl;
    conn->close(conn, NULL);
    return (EXIT_SUCCESS);
}
