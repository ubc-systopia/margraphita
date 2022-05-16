#include <getopt.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
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
std::string out_dir;
std::unordered_map<int64_t, int64_t> remap;

typedef std::shared_mutex Lock;
typedef std::unique_lock<Lock> WriteLock;
typedef std::shared_lock<Lock> ReadLock;

Lock lock;

void construct_vertex_mapping(int64_t line_num)
{
    std::string filename = dataset + "_nodes";
    std::ifstream nodefile(filename.c_str());
    if (nodefile.is_open())
    {
        std::string tp;
        int64_t line = 0;
        while (getline(nodefile, tp))
        {
            if (line < line_num)
            {
                line++;
                continue;
            }
            else
            {
                int64_t val;
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
void convert_edge_list(int64_t beg_offset, int64_t end_offset, int64_t t_id)
{
    std::ifstream edgefile(dataset.c_str());

    char c = (char)(97 + t_id);
    std::string out_filename = dataset + "_edges";
    out_filename.push_back('a');
    out_filename.push_back(c);
    std::ofstream outfile(out_filename.c_str());
    if (edgefile.is_open())
    {
        std::string tp;
        int64_t line = 0;
        while (getline(edgefile, tp))
        {
            if (line < beg_offset)
            {
                line++;
                continue;
            }
            else if (line >= beg_offset && line < end_offset)
            {
                int64_t src, dst;
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

void help()
{
    std::cout << "Usage: ./dense_vertexranges --edges <num_edges> --nodes "
                 "<num_nodes> --file <dataset>"
              << std::endl;
    std::cout << "This program will generate a dense vertexrange file for "
                 "the graph and produce a new graph with this new mapping. "
                 "This assumes that the nodes file has been created "
                 "<preprocess.sh does that> "
              << std::endl;
}

int main(int argc, char *argv[])
{
    std::string logfile;
    static struct option long_opts[] = {{"edges", required_argument, 0, 'e'},
                                        {"nodes", required_argument, 0, 'n'},
                                        {"file", required_argument, 0, 'f'},
                                        {0, 0, 0, 0}};
    int option_idx = 0;
    int c;
    if (argc < 2)
    {
        help();
        exit(-1);
    }

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
                help();
                exit(-1);
        }
    }

#pragma omp parallel for num_threads(NUM_THREADS) shared(num_nodes)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int64_t offset = ((i * num_nodes) / NUM_THREADS);

        construct_vertex_mapping(offset);
    }

    std::filesystem::path path(dataset);
    std::string out_filename = path.parent_path().string() + "/dense_map.txt";
    std::ofstream out(out_filename.c_str());
    for (auto iter = remap.begin(); iter != remap.end(); ++iter)
    {
        out << iter->first << " : " << iter->second << std::endl;
    }

#pragma omp parallel for num_threads(NUM_THREADS) shared(num_edges)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int64_t beg_offset = ((i * num_edges) / NUM_THREADS);
        int64_t end_offset = ((i + 1) * num_edges) / NUM_THREADS;
        convert_edge_list(beg_offset, end_offset, i);
    }
    return (EXIT_SUCCESS);
}
