//
// Created by puneet on 30/01/24.
//

#ifndef GRAPHAPI_MK_ADJLISTS_H
#define GRAPHAPI_MK_ADJLISTS_H
#include <string>
#include <utility>

#include "cstdlib"
#include "reader.h"

std::string dataset;
int num_per_chunk;

std::unordered_map<int, std::pair<adjlist, adjlist>> conflicts;

void insert_edge_thread(int _tid, const std::string& adjtype)
{
  int tid = _tid;
  std::string filename = dataset + "_";
  // the thread ID can be greater than number of alphabets, so we need to
  // append multiple characters
  int first_char = tid / 26;
  int second_char = tid % 26;
  char c1 = (char)(97 + first_char);
  char c2 = (char)(97 + second_char);
  filename.push_back(c1);
  filename.push_back(c2);

  reader::EdgeReader graph_reader(filename, 0, num_per_chunk, adjtype);
  graph_reader.mk_adjlist();
  std::pair<adjlist, adjlist> conflict;
  conflict = graph_reader.get_conflict();
  conflicts[tid] = conflict;
}

void delete_last_line(int _tid, const std::string& adjtype)
{
  int tid = _tid;
  // construct the filename from the directory name and the thread id
  std::string filename =
      dataset.substr(0, dataset.find_last_of('/')) + "/" + adjtype + "_";
  int first_char = tid / 26;
  int second_char = tid % 26;
  char c1 = (char)(97 + first_char);
  char c2 = (char)(97 + second_char);
  filename.push_back(c1);
  filename.push_back(c2);

  // run the sed command
  std::string command = "sed -i '$d' " + filename;
  // std::cout << "Command: " << command << std::endl;
  system(command.c_str());
}

void replace_first_line(int _tid,
                        const std::string& adjtype,
                        const adjlist& merged)
{
  int tid = _tid;
  // construct the filename from the directory name and the thread id
  std::string filename =
      dataset.substr(0, dataset.find_last_of('/')) + "/" + adjtype + "_";
  int first_char = tid / 26;
  int second_char = tid % 26;
  char c1 = (char)(97 + first_char);
  char c2 = (char)(97 + second_char);
  filename.push_back(c1);
  filename.push_back(c2);

  // run the sed command
  std::string command = "sed -i '1s/.*/" + std::to_string(merged.node_id) +
                        " " + std::to_string(merged.edgelist.size());
  for (auto i : merged.edgelist)
  {
    command += " " + std::to_string(i);
  }
  command += "/' " + filename;
  // std::cout <<"Command: " << command << std::endl;
  system(command.c_str());
}

void merge_conflicts(const std::string& adjtype, int NUM_THREADS)
{
  assert(conflicts.size() == NUM_THREADS);
  adjlist first_conflict, last_conflict;

  for (int i = 1; i < NUM_THREADS; i++)
  {
    adjlist top = conflicts[i].first;
    adjlist bottom = conflicts[i - 1].second;

    if (top.node_id == bottom.node_id)
    {
      // merge the bottom list into the top
      top.edgelist.insert(
          top.edgelist.end(), bottom.edgelist.begin(), bottom.edgelist.end());
      // Now delete the last line of the i-1st file
      delete_last_line(i - 1, adjtype);
      // Now replace the first line of the i-th file
      replace_first_line(i, adjtype, top);
    }
  }
}

void print_conflict_map()
{
  for (const auto& item : conflicts)
  {
    std::cout << "Thread: " << item.first << std::endl;
    std::cout << "Top: " << std::endl;
    adjlist temp = item.second.first;
    std::cout << "node: " << temp.node_id << ":: ";
    for (auto i : temp.edgelist)
    {
      std::cout << i << " ";
    }
    std::cout << std::endl;
    std::cout << "Bottom: " << std::endl;
    temp = item.second.second;
    std::cout << "node: " << temp.node_id << ":: ";
    for (auto i : temp.edgelist)
    {
      std::cout << i << " ";
    }
    std::cout << "\n----------------------" << std::endl
              << "----------------------" << std::endl;
  }
}
#endif  // GRAPHAPI_MK_ADJLISTS_H
