#ifndef READER_H
#define READER_H
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "bulk_insert.h"
#include "common.h"
namespace reader
{

std::vector<edge> parse_edge_entries(std::string filename);
std ::vector<node> parse_node_entries(std::string filename);

class EdgeReader
{
   private:
    std::string filename;
    int beg_offset;
    int num_per_chunk;
    std::ifstream edge_file;
    std::ofstream adj_file;
    std::vector<node_id_t> adjlist;  // for adjlist.
    node_id_t last_node_id = 0;
    bool is_first = false;
    bool is_last = false;

   public:
    EdgeReader(std::string _filename, int _beg, int _num, std::string adj_type)
    {
        filename = _filename;
        beg_offset = _beg;
        num_per_chunk = _num;
        edge_file = std::ifstream(_filename, std::ifstream::in);
        adj_file =
            std::ofstream((_filename + "_" + adj_type), std::ofstream::out);
        if (edge_file.is_open())

        {
            int i = 0;
            while (i < beg_offset)
            {
                std::string line;
                getline(edge_file, line);
                i++;
            }
            is_first = true;
        }
        else
        {
            throw GraphException("** could not open " + filename);
        }
        if (!adj_file.is_open())
        {
            throw GraphException("Failed to create the " + adj_type +
                                 " adjacency file for " + filename);
        }
    }

    int get_next_edge(edge& e)

    {
        if (num_per_chunk > 0)
        {
            std::string line;

            if (getline(edge_file, line))
            {
                std::stringstream s_stream(line);
                s_stream >> e.src_id;
                s_stream >> e.dst_id;
                num_per_chunk--;

                if (is_first)
                {
                    last_node_id = e.src_id;
                    is_first = false;
                }
                if (e.src_id == last_node_id)
                {
                    adjlist.push_back(e.dst_id);
                }
                else
                {
                    write_adjlist_to_file(e);
                }

                return 0;
            }
            else
            {
                is_last = true;
                write_adjlist_to_file(e);
                edge_file.close();
                adj_file.close();

                return -1;
            }
        }
        return -1;
    }

    void write_adjlist_to_file(const edge& e)
    {
        // write out the adjlist
        adj_file << last_node_id << " ";
        for (auto x : adjlist)
        {
            adj_file << x << " ";
        }
        adj_file << "\n";
        // advance last_node_id to what we just read
        last_node_id = e.src_id;
        adjlist.clear();
        // add e.dst_id to adjlist
        if (!is_last)
        {
            adjlist.push_back(e.dst_id);
        }
    }
};
}  // namespace reader

#endif