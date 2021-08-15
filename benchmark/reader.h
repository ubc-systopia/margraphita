#ifndef READER_H
#define READER_H
#include <vector>
#include <string>
#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include "bulk_insert.h"
namespace reader
{

    std::vector<edge> parse_edge_entries(std::string filename);
    std ::vector<node> parse_node_entries(std::string filename);
    //std::unordered_map<int, node> parse_node_entries(std::string filename);
}

#endif