#include "common.h"
#include "mio.hpp"
#include <thread>
#include <atomic>
//#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <mutex>

#define NUM_THREADS 10

WT_CONNECTION *conn;
double num_edges;
double num_nodes;
std::string dataset;
int edge_per_part;
int read_optimized = 0;

std::unordered_map<int, std::vector<int>> in_adjlist;
std::unordered_map<int, std::vector<int>> out_adjlist;

std::mutex lock;

typedef struct thread_params
{
    std::string filename;
    int tid;
} thread_params;

namespace reader
{

    std::vector<edge> parse_edge_entries(std::string filename)
    {
        std::vector<edge> edges;
        std::ifstream newfile(filename);

        if (newfile.is_open())
        {
            std::string tp;
            while (getline(newfile, tp))
            {
                if ((tp.compare(0, 1, "#") == 0) or (tp.compare(0, 1, "%") == 0) or (tp.empty()))
                {
                    continue;
                }

                else
                {
                    int a, b;
                    std::stringstream s_stream(tp);
                    s_stream >> a;
                    s_stream >> b;
                    edge to_insert;
                    to_insert.src_id = a;
                    to_insert.dst_id = b;
                    lock.lock();
                    edges.push_back(to_insert);
                    in_adjlist[b].push_back(a);  //insert a in b's in_adjlist
                    out_adjlist[a].push_back(b); //insert b in a's out_adjlist
                    lock.unlock();
                }
            }
            newfile.close();
        }
        else
        {
            std::cout << "**could not open " << filename << std::endl;
        }
        return edges;
    }

    std::unordered_map<int, node> parse_node_entries(std::string filename)
    {
        std::unordered_map<int, node> nodes;
        std::vector<std::string> filenames;

        std::string name = filename + "_indeg_";
        filenames.push_back(name);

        name = filename + "_outdeg_";
        filenames.push_back(name);
        int i = 0;
        for (std::string file : filenames)
        {

            std::cout << file << std::endl;
            std::ifstream newfile(file);
            if (newfile.is_open())
            {
                std::string tp;
                while (getline(newfile, tp))
                {
                    if ((tp.compare(0, 1, "#") == 0) or (tp.compare(0, 1, "%") == 0) or (tp.empty()))
                    {
                        continue;
                    }

                    else
                    {
                        int a, b;
                        std::stringstream s_stream(tp);
                        s_stream >> a;
                        s_stream >> b;

                        auto search = nodes.find(b);
                        if (search == nodes.end())
                        {
                            node to_insert;
                            to_insert.id = b; //Second entry is node id
                            if (i == 0)
                            {
                                to_insert.in_degree = a;
                            }
                            else
                            {
                                to_insert.out_degree = a;
                            }
                            nodes[b] = to_insert;
                        }
                        else
                        {
                            node to_insert = search->second;
                            if (i == 0)
                            {
                                to_insert.in_degree = a;
                            }
                            else
                            {
                                to_insert.out_degree = a;
                            }
                            nodes[b] = to_insert;
                        }
                    }
                }
                newfile.close();
            }
            else
            {
                std::cout << "could not open";
            }
            i++;
        }

        return nodes;
    }
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

    //Now insert nodes;
    node_total = insert_node();

    std::cout << "total number of edges inserted = " << edge_total << std::endl;
    std::cout << "total number of nodes inserted = " << node_total << std::endl;
    conn->close(conn, NULL);
    return (EXIT_SUCCESS);
}
