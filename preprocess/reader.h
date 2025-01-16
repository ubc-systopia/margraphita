#ifndef READER_H
#define READER_H
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
    edge_file = std::ifstream(filename, std::ifstream::in);

    std::string dirname = filename.substr(0, filename.find_last_of('/'));
    // strip the filename of anything after_
    std::string::size_type pos = filename.find_last_of('_');
    if (pos != std::string::npos)
    {
      filename =
          dirname + "/" + adj_type + filename.substr(pos, filename.size());
    }

    adj_file = std::ofstream(filename, std::ofstream::out);
    std::cout << "Filename: " << filename << std::endl;
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
    adj_file << node_adj_list.node_id << " " << node_adj_list.edgelist.size()
             << " ";
    for (int i = 0; i < node_adj_list.edgelist.size(); i++)
    {
      adj_file << node_adj_list.edgelist[i];
      if (i != node_adj_list.edgelist.size() - 1)
      {
        adj_file << ",";
      }
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
  ~EdgeReader()
  {
    edge_file.close();
    adj_file.close();
  }
};

// read from boost archive file
class AdjReader
{
 private:
  std::ifstream adj_file;
  int length = 0;

 public:
  explicit AdjReader(const std::string& filename)
  {
    adj_file = std::ifstream(filename, std::ifstream::in);
    if (!adj_file.is_open())
    {
      throw GraphException("Failed to open the adjacency file for " + filename);
    }
  }

  // each line has a node id, it's degree, and the list of neighbors, all
  // separated by spaces getline from file and parse the line
  int get_next_adjlist(adjlist& adj)
  {
    std::string line;
    if (getline(adj_file, line))
    {
      std::istringstream iss(line);
      if (iss >> adj.node_id)
      {
        iss >> adj.degree;
        node_id_t n;
        while (iss >> n)
        {
          adj.edgelist.push_back(n);
          if (iss.peek() == ',')
          {
            iss.ignore();
          }
        }
      }
      else
      {
        adj_file.close();
        std::cerr << "Error reading from file" << std::endl;
        return -1;
      }
      // adj.degree = adj.edgelist.size();n
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
      throw GraphException("Failed to open the node file for " + filename);
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