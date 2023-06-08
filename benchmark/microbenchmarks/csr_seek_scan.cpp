#include <math.h>
#include <stdio.h>
#include <times.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <experimental/random>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include "adj_list.h"
#include "benchmark_definitions.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "standard_graph.h"
#include "times.h"

class CSR
{
   private:
    std::vector<int> offsets;
    std::vector<int> edges;

   public:
    int num_vertices;
    int num_edges;
    CSR(int num_vertices, int num_edges)
    {
        this->num_vertices = num_vertices + 1;
        this->num_edges = num_edges;
        this->offsets.reserve(num_vertices);
        this->edges.reserve(num_edges);
    }

    int get_vertex_degree(int vertex)
    {
        return this->offsets[vertex + 1] - this->offsets[vertex];
    }

    void adjacency_to_csr(std::vector<std::vector<int>> &adjacency_list)
    {
        int offset = 0;
        for (std::size_t i = 0; i < adjacency_list.size(); i++)
        {
            this->offsets.push_back(offset);
            for (std::size_t j = 0; j < adjacency_list[i].size(); j++)
            {
                this->edges.push_back(adjacency_list[i][j]);
                offset++;
            }
        }
        this->offsets.push_back(offset);
    }

#pragma GCC push_options
#pragma GCC optimize("O0")
    std::vector<long double> seek_and_scan(int vertex)
    {
        std::vector<long double> results;
        Times timer;
        timer.start();
        [[maybe_unused]] int degree =
            offsets[vertex + 1] - offsets[vertex];  // seek time
        timer.stop();
        results.push_back(degree);
        results.push_back(timer.t_nanos());

        timer.start();
        [[maybe_unused]] int element;
        for (int i = this->offsets[vertex]; i < this->offsets[vertex + 1]; i++)
        {
            element = this->edges[i];  // do nothing
        }
        timer.stop();
        results.push_back(timer.t_nanos());
        return results;
    }
#pragma GCC pop_options

    std::vector<int> get_edges(int vertex)
    {
        std::vector<int> result;
        for (int i = this->offsets[vertex]; i < this->offsets[vertex + 1]; i++)
        {
            result.push_back(this->edges[i]);
        }
        return result;
    }

    void print_csr()
    {
        std::cout << "Offsets: ";
        for (std::size_t i = 0; i < this->offsets.size(); i++)
        {
            std::cout << this->offsets[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "Edges: ";
        for (std::size_t i = 0; i < this->edges.size(); i++)
        {
            std::cout << this->edges[i] << " ";
        }
        std::cout << std::endl;

        for (std::size_t i = 0; i < this->offsets.size() - 1; i++)
        {
            std::cout << "Vertex " << i << " has edges: ";
            for (int j = this->offsets[i]; j < this->offsets[i + 1]; j++)
            {
                std::cout << this->edges[j] << " ";
            }
            std::cout << std::endl;
        }
    }

    void insert_graph(std::filesystem::path graphfile)
    {
        std::vector<std::vector<int>> adjacency_list;
        adjacency_list.reserve(num_vertices);

        std::ifstream infile(graphfile);
        std::string line;
        int current_src = -1;
        std::vector<int> temp;
        int line_num = 0;
        std::vector<int> empty_vec;
        while (std::getline(infile, line))
        {
            std::istringstream iss(line);
            int a, b;
            if (!(iss >> a >> b))
            {
                break;
            }
            if (line_num == 0)
            {
                current_src = a;
            }
            if (a != current_src)
            {
                adjacency_list.push_back(temp);
                temp.clear();
                if (a != current_src + 1)
                {
                    for (int i = current_src + 1; i < a; i++)
                    {
                        adjacency_list.push_back(empty_vec);
                    }
                }
                current_src = a;
                temp.push_back(b);
                line_num++;
            }
            else
            {
                temp.push_back(b);
                line_num++;
            }
        }
        adjacency_list.push_back(temp);

        adjacency_to_csr(adjacency_list);
        // csr.print_csr();
    }
};

void profile_csr(int num_vertices,
                 int num_edges,
                 std::filesystem::path graphfile,
                 int num_random_samples,
                 CSR &csr)
{
    std::set<int> random_vertices;

    // select random nodes
    std::random_device rd;   // a seed source for the random number engine
    std::mt19937 gen(rd());  // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(0, num_vertices - 1);

    // Use distrib to transform the random unsigned int
    // generated by gen into an int in [1, 6]
    for (int n = 0; n < num_random_samples; ++n)
    {
        int rando = distrib(gen);

        if (csr.get_vertex_degree(rando) > 0)
        {
            random_vertices.insert(rando);
        }
        else
        {
            n--;
        }
    }
    // create file for csr seek and scan times
    char outfile_name[256];

    sprintf(outfile_name, "%s_csr_ubench.txt", graphfile.stem().c_str());
    std::ofstream csr_seek_scan_outfile(outfile_name);

    csr_seek_scan_outfile
        << "vertex_id,degree,seek_time_ns,scan_time_per_edge_ns" << std::endl;
    for (int sample : random_vertices)
    {
        std::vector<long double> time = csr.seek_and_scan(sample);
        csr_seek_scan_outfile << sample << "," << time[0] << "," << time[1]
                              << "," << time[2] / time[0] << std::endl;
    }
    csr_seek_scan_outfile.close();
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cout << "Usage: ./csr_seek_scan <num_vertices> <num_edges> "
                     "<graphfile>"
                  << std::endl;
        return 0;
    }
    int num_vertices = atoi(argv[1]);
    int num_edges = atoi(argv[2]);
    std::filesystem::path graphfile(argv[3]);
    int num_random_samples = 1000;

    // check if graph exists
    if (!std::filesystem::exists(graphfile))
    {
        std::cout << "File does not exist" << std::endl;
        return 0;
    }

    CSR csr(num_vertices, num_edges);
    csr.insert_graph(graphfile);

    // Profile the CSR
    profile_csr(num_vertices, num_edges, graphfile, num_random_samples, csr);
}