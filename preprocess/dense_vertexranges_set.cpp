#include <getopt.h>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

#define NUM_THREADS 16
double num_edges;
double num_nodes;
std::string dataset;
bool map_exit = false;
std::set<int> nodes;
std::unordered_map<int, int> remap;
std::mutex lock;

int64_t construct_vertex_mapping(int beg, int end)
{
    std::set<int> part_nodes;
    std::string filename = dataset;
    std::cout << beg << "\t" << end << std::endl;
    std::ifstream nodefile(filename.c_str());
    int64_t line = 0;

    if (nodefile.is_open())
    {
        std::string tp;
        while (getline(nodefile, tp) && line < end)
        {
            if (line < beg)
            {
                line++;
                continue;
            }
            else
            {
                std::stringstream sstr(tp);
                int a, b;
                sstr >> a;
                sstr >> b;
                part_nodes.insert(a);
                part_nodes.insert(b);
                line++;
            }
        }
        lock.lock();
        nodes.insert(part_nodes.begin(), part_nodes.end());
        lock.unlock();
        nodefile.close();
    }
    else
    {
        std::cout << "failed;";
    }
    return line;
}

int64_t convert_edge_list(int beg_offset, int end_offset, int t_id)
{
    std::ifstream edgefile(dataset.c_str());

    char c = (char)(97 + t_id);
    std::string out_filename = dataset + "_edges";

    out_filename.push_back('a');
    out_filename.push_back(c);
    std::ofstream outfile(out_filename.c_str());
    int line = 0;
    if (edgefile.is_open())
    {
        std::string tp;
        while (getline(edgefile, tp) && line < end_offset)
        {
            if (line < beg_offset)
            {
                line++;
                continue;
            }
            else
            {
                int src, dst;
                std::stringstream s_str(tp);
                s_str >> src;
                s_str >> dst;
                outfile << remap[src] << "\t" << remap[dst] << "\n";

                line++;
            }
        }
        edgefile.close();
        outfile.close();
    }
    return line;
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

int main(int argc, char* argv[])
{
    std::string logfile;
    static struct option long_opts[] = {{"edges", required_argument, 0, 'e'},
                                        {"nodes", required_argument, 0, 'n'},
                                        {"file", required_argument, 0, 'f'},
                                        {"make-map", optional_argument, 0, 'm'},
                                        {0, 0, 0, 0}};
    int option_idx = 0;
    int c;
    if (argc < 2)
    {
        help();
        exit(-1);
    }

    while ((c = getopt_long(argc, argv, "e:n:f:md", long_opts, &option_idx)) !=
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
            case 'm':
                map_exit = true;
                break;
            case ':':
            /* missing option argument */
            case '?':
            default:
                help();
                exit(-1);
        }
    }

    int64_t offsets[NUM_THREADS];
    int64_t end_offsets[NUM_THREADS];

#pragma omp parallel for num_threads(NUM_THREADS) shared(num_nodes)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int beg = (int)((i * num_edges) / NUM_THREADS);
        int end = (int)(((i + 1) * num_edges) / NUM_THREADS);
        offsets[i] = beg;
        end_offsets[i] = construct_vertex_mapping(beg, end);
    }
    assert(nodes.size() == num_nodes);

    // Construct a map of old to new ID. I don't know how to make this MT :((
    int i = 0;
    for (int x : nodes)
    {
        remap[x] = i;
        i++;
    }

    // Write out the old ID to new ID map.
    std::filesystem::path path(dataset);
    std::string out_filename = path.parent_path().string() + "/dense_map.txt";
    std::cout << "\ndense_map file is :" << out_filename << std::endl;
    std::ofstream out(out_filename.c_str());

    for (auto x : remap)
    {
        out << x.first << "\t" << x.second << "\n";
    }

    out.close();

    // Write the nodes file.
    out_filename = dataset + "_nodes";
    out.open(out_filename.c_str());
    for (i = 0; i < num_nodes; i++)
    {
        out << i << "\n";
    }
    out.close();

    if (map_exit)
    {
        exit(EXIT_SUCCESS);
    }

    // Write out the new graph file with new ID mapping.
    int64_t e_offsets[NUM_THREADS];
    int64_t e_end_offsets[NUM_THREADS];
#pragma omp parallel for num_threads(NUM_THREADS) shared(num_edges)
    for (i = 0; i < NUM_THREADS; i++)
    {
        int beg_offset = (int)(i * num_edges) / NUM_THREADS;
        int end_offset = (int)(((i + 1) * num_edges) / NUM_THREADS);
        e_offsets[i] = beg_offset;
        e_end_offsets[i] = convert_edge_list(beg_offset, end_offset, i);
    }

    return (EXIT_SUCCESS);
}