#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

//#include "dense_vertexrange.h"
#define NUM_THREADS 10
double num_edges;
double num_nodes;
std::string dataset;
std::unordered_map<int, int> remap;

typedef std::shared_mutex Lock;
typedef std::unique_lock<Lock> WriteLock;
typedef std::shared_lock<Lock> ReadLock;

Lock lock;

void construct_vertex_mapping(int line_num)
{
    std::string filename = dataset + "_nodes";
    std::ifstream nodefile(filename.c_str());
    if (nodefile.is_open())
    {
        std::string tp;
        int line = 0;
        while (getline(nodefile, tp))
        {
            if (line < line_num)
            {
                line++;
                continue;
            }
            else
            {
                int val;
                std::stringstream s_str(tp);
                s_str >> val;
                WriteLock writer(lock);
                remap[val] = line;  // key is old id, val is new_id
                writer.unlock();
                line++;
            }
        }
        nodefile.close();
    }
    else
    {
        std::cout << "failed;";
    }
}
void convert_edge_list(int beg_offset, int end_offset, int t_id)
{
    std::cout << "tid " << t_id << "; (" << beg_offset << "," << end_offset
              << ")";
    std::string filename = dataset;
    std::ifstream edgefile(filename.c_str());

    char c = (char)(97 + t_id);
    std::string out_filename;
    out_filename.push_back('a');
    out_filename.push_back(c);
    std::ofstream outfile(out_filename.c_str());
    if (edgefile.is_open())
    {
        std::string tp;
        int line = 0;
        while (getline(edgefile, tp))
        {
            if (line < beg_offset)
            {
                line++;
                continue;
            }
            else if (line >= beg_offset && line < end_offset)
            {
                int src, dst;
                std::stringstream s_str(tp);
                s_str >> src;
                s_str >> dst;

                outfile << remap[src] << " " << remap[dst] << "\n";
                line++;
            }
        }
        edgefile.close();
        outfile.close();
    }
}

int main(int argc, char *argv[])
{
    std::string logfile;
    static struct option long_opts[] = {{"edges", required_argument, 0, 'e'},
                                        {"nodes", required_argument, 0, 'n'},
                                        {"file", required_argument, 0, 'f'}};
    int option_idx = 0;
    int c;

    while ((c = getopt_long(argc, argv, "e:n:f:", long_opts, &option_idx)) !=
           -1)
    {
        switch (c)
        {
            case 'e':
                num_edges = atol(optarg);
                break;
            case 'n':
                num_nodes = atol(optarg);
                break;
            case 'f':
                dataset = optarg;
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

#pragma omp parallel for num_threads shared(num_nodes, NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int offset = ((i * num_nodes) / NUM_THREADS);

        construct_vertex_mapping(offset);
    }
    sleep(5);

    std::ofstream out("mapping.txt");
    for (auto iter = remap.begin(); iter != remap.end(); ++iter)
    {
        out << iter->first << " : " << iter->second << std::endl;
    }

#pragma omp parallel for num_threads shared(num_edges, NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int beg_offset = ((i * num_edges) / NUM_THREADS);
        int end_offset = ((i + 1) * num_edges) / NUM_THREADS;
        convert_edge_list(beg_offset, end_offset, i);
    }
    return (EXIT_SUCCESS);
}