#ifndef READER_H
#define READER_H
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
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
    std::pair<int, std::vector<node_id_t>> node_adj_list;
    std::vector<node_id_t> adjlist;  // for adjlist.
    node_id_t last_node_id = 0;
    bool is_first = false;
    bool is_last = false;
    int max_pos = 0;
    bool is_right = false;

   public:
    EdgeReader(std::string _filename,
               int _beg,
               int _num,
               std::string adj_type,
               bool _is_right)
    {
        is_right = _is_right;
        filename = _filename;
        beg_offset = _beg;
        num_per_chunk = _num;
        adjlist.reserve(10000);
        std::ios::sync_with_stdio(false);
        edge_file = std::ifstream(_filename, std::ifstream::in);
        adj_file =
            std::ofstream((_filename + "_" + adj_type), std::ios::binary);
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
            edge_file.seekg(0, edge_file.end);
            max_pos = edge_file.tellg();
            edge_file.seekg(0, edge_file.beg);
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
        std::string line;
        if (edge_file.tellg() < max_pos)
        {
            getline(edge_file, line);
            std::stringstream s_stream(line);
            s_stream >> e.src_id;
            s_stream >> e.dst_id;

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
                node_adj_list.first = last_node_id;
                node_adj_list.second = adjlist;
                write_adjlist_to_file(e, node_adj_list);
            }
            return 0;
        }
        else
        {
            if (is_right)
            {
                std::cout << "End of file reached" << std::endl;
            }

            node_adj_list.first = last_node_id;
            node_adj_list.second = adjlist;
            write_adjlist_to_file(e, node_adj_list);
            edge_file.close();
            adj_file.close();
            return -1;
        }
    }

    void write_adjlist_to_file(
        const edge& e,
        const std::pair<int, std::vector<node_id_t>>& node_adj_list)
    {
        // std::cout << "node id: " << node_adj_list.first << std::endl;
        // std::cout << e.src_id << " " << e.dst_id << std::endl;
        boost::archive::binary_oarchive oa(adj_file);
        oa << node_adj_list;
        adjlist.clear();
        adjlist.push_back(e.dst_id);
        last_node_id = e.src_id;
    }
};

// read from boost archive file
class AdjReader
{
   private:
    std::string filename;
    std::ifstream adj_file;
    int length = 0;
    std::pair<int, std::vector<node_id_t>> node_adj_list;

   public:
    AdjReader(std::string _filename)
    {
        filename = _filename;
        adj_file = std::ifstream(_filename, std::ifstream::in);
        if (!adj_file.is_open())
        {
            throw GraphException("Failed to open the adjacency file for " +
                                 filename);
        }
        adj_file.seekg(0, adj_file.end);
        length = adj_file.tellg();
        adj_file.seekg(0, adj_file.beg);
    }

    int get_next_adjlist(std::pair<int, std::vector<node_id_t>>& node_adj_list)
    {
        if ((adj_file.tellg() < length))
        {
            boost::archive::binary_iarchive ia(adj_file);
            ia >> node_adj_list;
            return 0;
        }
        else
        {
            adj_file.close();
            return -1;
        }
    }
};

}  // namespace reader

#endif