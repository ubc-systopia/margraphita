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

#include "common_util.h"
#include "parallel_hashmap/phmap.h"
#define MAPNAME phmap::parallel_flat_hash_map
#define NMSP phmap
#define NUM_THREADS 16
#define MTX std::mutex
#define EXTRAARGS                                                     \
  , NMSP::priv::hash_default_hash<K>, NMSP::priv::hash_default_eq<K>, \
      std::allocator<std::pair<const K, V>>, 4, MTX

template <class K, class V>
using HashT = MAPNAME<K, V EXTRAARGS>;
using hash_t = HashT<node_id_t, node_id_t>;

double num_edges;
double num_nodes;
std::string dataset;
[[maybe_unused]] std::string out_dir;
bool map_exit = false;

hash_t remap;

int64_t construct_vertex_mapping(node_id_t beg, node_id_t end)
{
  std::string filename = dataset + "_nodes";
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
        node_id_t val;
        std::stringstream s_str(tp);
        s_str >> val;
        remap[val] = line;
        line++;
      }
    }
    nodefile.close();
  }
  else
  {
    std::cout << "failed;";
  }
  return line;
}

int64_t convert_edge_list(node_id_t beg_offset,
                          node_id_t end_offset,
                          node_id_t t_id)
{
  std::ifstream edgefile(dataset.c_str());

  char c = (char)(97 + t_id);
  std::string out_filename = dataset + "_edges";

  out_filename.push_back('a');
  out_filename.push_back(c);
  std::ofstream outfile(out_filename.c_str());
  node_id_t line = 0;
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
        node_id_t src, dst;
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

int main(int argc, char *argv[])
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
    node_id_t beg = ((i * num_nodes) / NUM_THREADS);
    node_id_t end = (((i + 1) * num_nodes) / NUM_THREADS);
    offsets[i] = beg;
    end_offsets[i] = construct_vertex_mapping(beg, end);
  }
  assert(remap.size() == num_nodes);

  auto __nodes_per_edge = num_nodes / NUM_THREADS;
  int i = 0;
  for (auto x : offsets)
  {
    std::cout << (i * __nodes_per_edge) << "\t(" << x << "," << end_offsets[i]
              << ")" << std::endl;
    i++;
  }
  std::filesystem::path path(dataset);
  std::string out_filename = path.parent_path().string() + "/dense_map_og.txt";
  std::cout << "\ndense_map file is :" << out_filename << std::endl;
  std::ofstream out(out_filename.c_str());
  for (auto iter = remap.begin(); iter != remap.end(); ++iter)
  {
    out << iter->first << ":" << iter->second << std::endl;
  }
  out.close();
  if (map_exit)
  {
    exit(EXIT_SUCCESS);
  }

  int64_t e_offsets[NUM_THREADS];
  int64_t e_end_offsets[NUM_THREADS];
#pragma omp parallel for num_threads(NUM_THREADS) shared(num_edges)
  for (int i = 0; i < NUM_THREADS; i++)
  {
    node_id_t beg_offset = ((i * num_edges) / NUM_THREADS);
    node_id_t end_offset = ((i + 1) * num_edges) / NUM_THREADS;
    e_offsets[i] = beg_offset;
    e_end_offsets[i] = convert_edge_list(beg_offset, end_offset, i);
  }
  std::cout << "-----------------\n\n";
  i = 0;
  auto shoulda = num_edges / NUM_THREADS;
  for (auto x : e_offsets)
  {
    std::cout << (i * shoulda) << "\t(" << x << "," << e_end_offsets[i] << ")"
              << std::endl;
    i++;
  }

  // test the combined file with map

  return (EXIT_SUCCESS);
}
