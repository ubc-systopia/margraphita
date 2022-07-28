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
    std::ifstream filestream;

   public:
    EdgeReader(std::string _filename, int _beg, int _chunk_size)
    {
        filename = _filename;
        beg_offset = _beg;
        num_per_chunk = _chunk_size;
        filestream = std::ifstream(_filename, std::ifstream::in);
        if (filestream.is_open())
        {
            int i = 0;
            while (i < beg_offset)
            {
                std::string line;
                getline(filestream, line);
                i++;
            }
        }
        else
        {
            std::cout << "** could not open " << filename << std::endl;
        }
    }

    int get_next_edge(edge &e)
    {
        if (num_per_chunk > 0)
        {
            std::string line;
            if (getline(filestream, line))
            {
                std::stringstream s_stream(line);
                s_stream >> e.src_id;
                s_stream >> e.dst_id;
                num_per_chunk--;
                return 0;
            }
            else
            {
                filestream.close();
                return -1;
            }
        }
        return -1;
    }
};
}  // namespace reader

#endif