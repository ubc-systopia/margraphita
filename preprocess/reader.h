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
// std::unordered_map<int, node> parse_node_entries(std::string filename);
}  // namespace reader

#endif