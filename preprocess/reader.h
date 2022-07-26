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
    std::ifstream filestream;

   public:
    EdgeReader(std::string _filename)
    {
        filename = _filename;
        filestream = std::ifstream(_filename, std::ifstream::in);
    }

    int get_next_edge(edge &e)
    {
        if (filestream.is_open())
        {
            std::string line;
            if (getline(filestream, line))
            {
                std::stringstream s_stream(line);
                s_stream >> e.src_id;
                s_stream >> e.dst_id;
                return 0;
            }
            else
            {
                filestream.close();
                return -1;
            }
        }
        else
        {
            std::cout << "** could not open " << filename << std::endl;
            return -1;
        }
    }
};

}  // namespace reader

#endif