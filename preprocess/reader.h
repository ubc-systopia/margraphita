#ifndef READER_H
#define READER_H
// #include <boost/archive/binary_iarchive.hpp>
// #include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "bulk_insert.h"
#include "common_defs.h"
#include "common_util.h"
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
    std::ofstream adj_file, edge_file_txt;
    adjlist node_adj_list, first_conflict, last_conflict;
    node_id_t last_node_id = 0;
    long max_pos = 0;
    bool first = false;
    int cur_pos = 0;

   public:
    EdgeReader(const std::string& _filename,
               int _beg,
               int _num,
               const std::string& adj_type)
    {
        filename = _filename;
        beg_offset = _beg;
        cur_pos = beg_offset;
        num_per_chunk = _num;

        std::ios::sync_with_stdio(false);
        edge_file = std::ifstream(_filename, std::ifstream::in);
        adj_file =
            std::ofstream((_filename + "_" + adj_type), std::ios::binary);

        if (!edge_file.is_open())
        {
            throw GraphException("** could not open " + filename);
        }
        if (!adj_file.is_open())
        {
            throw GraphException("Failed to create the " + adj_type +
                                 " adjacency file for " + filename);
        }
    }

    void mk_adjlist()
    {
        edge e;
        std::string line;
        while (getline(edge_file, line))
        {
            std::stringstream s_stream(line);
            s_stream >> e.src_id;
            s_stream >> e.dst_id;

            if (cur_pos == beg_offset)
            {
                node_adj_list.node_id = e.src_id;
                node_adj_list.edgelist.push_back(e.dst_id);
                first = true;
                cur_pos++;
                continue;
            }

            if (e.src_id == node_adj_list.node_id)
            {
                node_adj_list.edgelist.push_back(e.dst_id);
            }
            else  // new node
            {
                write_adjlist_to_file();
                if (first)
                {
                    first_conflict.edgelist = std::move(node_adj_list.edgelist);
                    first_conflict.node_id = node_adj_list.node_id;
                    first = false;
                }
                node_adj_list.clear();
                node_adj_list.node_id = e.src_id;
                node_adj_list.edgelist.push_back(e.dst_id);
            }
            cur_pos++;
        }
        write_adjlist_to_file();

        last_conflict.node_id = node_adj_list.node_id;
        last_conflict.edgelist = std::move(node_adj_list.edgelist);
        node_adj_list.clear();

        edge_file.close();
        edge_file_txt.close();
        adj_file.close();
    }

    void write_adjlist_to_file()
    {
        // boost::archive::binary_oarchive oa(adj_file);
        // boost::archive::text_oarchive oa(adj_file);
        // oa << node_adj_list.node_id << node_adj_list.edgelist.size()
        //   << node_adj_list.edgelist;
        adj_file << node_adj_list.node_id << " "
                 << node_adj_list.edgelist.size() << " ";
        for (auto n : node_adj_list.edgelist)
        {
            adj_file << n << " ";
        }
        adj_file << "\n";
    }

    std::pair<adjlist, adjlist> get_conflict() const
    {
        std::pair<adjlist, adjlist> conflict;
        conflict.first = first_conflict;
        conflict.second = last_conflict;
        return conflict;
    }
};

// read from boost archive file
class AdjReader
{
   private:
    std::string filename;
    std::ifstream adj_file;
    int length = 0;


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
            boost::archive::text_iarchive ia(adj_file);
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

class NodeReader
{
   private:
    std::string filename;
    std::ifstream node_file;
    node n;
    int length = 0;

   public:
    NodeReader(std::string _filename)
    {
        filename = _filename;
        node_file = std::ifstream(_filename, std::ifstream::in);
        if (!node_file.is_open())
        {
            throw GraphException("Failed to open the node file for " +
                                 filename);
        }
        node_file.seekg(0, node_file.end);
        length = node_file.tellg();
        node_file.seekg(0, node_file.beg);
    }

    int get_next_node(node& n)
    {
        std::string line;
        if ((node_file.tellg() < length))
        {
            getline(node_file, line);
            std::stringstream s_str(line);
            s_str >> n.id;
            n.in_degree = 0;
            n.out_degree = 0;
            return 0;
        }
        else
        {
            node_file.close();
            return -1;
        }
    }
};

}  // namespace reader

#endif