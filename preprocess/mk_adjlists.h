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
std::unordered_map<int, std::pair<std::vector<edgeweight_t>, std::vector<edgeweight_t>>>
    weights_conflicts;

void insert_edge_thread(int _tid, const std::string& adjtype, bool is_weighted = false)
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

  reader::EdgeReader graph_reader(filename, 0, num_per_chunk, adjtype, is_weighted);
  graph_reader.mk_adjlist();
  std::pair<adjlist, adjlist> conflict;
  conflict = graph_reader.get_conflict();
  conflicts[tid] = conflict;
  if (is_weighted)
  {
    weights_conflicts[tid] = graph_reader.get_conflict_weights();
  }
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

  //do the same for the weights file
  std::string weights_filename =
      dataset.substr(0, dataset.find_last_of('/')) + "/" + adjtype + "_weights_";
  weights_filename.push_back(c1);
  weights_filename.push_back(c2);

  // Delete last line from adjlist file
  std::string command = "sed -i '$d' " + filename;
  system(command.c_str());

  // Delete last line from weights file if it exists
  if (weights_conflicts.find(tid) != weights_conflicts.end())
  {
    std::string weights_command = "sed -i '$d' " + weights_filename;
    system(weights_command.c_str());
  }
}

void replace_first_line(int _tid,
                        const std::string& adjtype,
                        const adjlist& merged,
                        const std::vector<edgeweight_t>& merged_weights = {})
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

  // Replace first line in adjlist file
  std::string command = "sed -i '1s/.*/" + std::to_string(merged.node_id) +
                        " " + std::to_string(merged.edgelist.size());
  for (size_t i = 0; i < merged.edgelist.size(); i++)
  {
    command += " " + std::to_string(merged.edgelist[i]);
    if (i != merged.edgelist.size() - 1)
    {
      command += ",";
    }
  }
  command += "/' " + filename;
  system(command.c_str());

  // Replace first line in weights file if weights are provided
  if (!merged_weights.empty())
  {
    std::string weights_filename =
        dataset.substr(0, dataset.find_last_of('/')) + "/" + adjtype + "_weights_";
    weights_filename.push_back(c1);
    weights_filename.push_back(c2);

    std::string weights_command = "sed -i '1s/.*/" + std::to_string(merged.node_id) + " ";
    for (size_t i = 0; i < merged_weights.size(); i++)
    {
      weights_command += std::to_string(merged_weights[i]);
      if (i != merged_weights.size() - 1)
      {
        weights_command += ",";
      }
    }
    weights_command += "/' " + weights_filename;
    system(weights_command.c_str());
  }
}

void merge_conflicts(const std::string& adjtype, int NUM_THREADS, bool is_weighted = false)
{
  assert(conflicts.size() == NUM_THREADS);
  adjlist first_conflict, last_conflict;

  for (int i = 1; i < NUM_THREADS; i++)
  {
    adjlist top = conflicts[i].first;
    adjlist bottom = conflicts[i - 1].second;

    if (top.node_id == bottom.node_id)
    {
      if (is_weighted && weights_conflicts.find(i) != weights_conflicts.end() &&
          weights_conflicts.find(i - 1) != weights_conflicts.end())
      {
        // Merge edges and weights together
        std::vector<edgeweight_t> top_weights = weights_conflicts[i].first;
        std::vector<edgeweight_t> bottom_weights = weights_conflicts[i - 1].second;

        // Create pairs of (edge, weight)
        std::vector<std::pair<node_id_t, edgeweight_t>> edge_weight_pairs;
        for (size_t j = 0; j < top.edgelist.size(); j++)
        {
          edge_weight_pairs.push_back({top.edgelist[j], top_weights[j]});
        }
        for (size_t j = 0; j < bottom.edgelist.size(); j++)
        {
          edge_weight_pairs.push_back({bottom.edgelist[j], bottom_weights[j]});
        }

        // Sort by edge id
        std::sort(edge_weight_pairs.begin(), edge_weight_pairs.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        // Extract sorted edges and weights
        top.edgelist.clear();
        top_weights.clear();
        for (const auto& pair : edge_weight_pairs)
        {
          top.edgelist.push_back(pair.first);
          top_weights.push_back(pair.second);
        }

        // Update weights_conflicts
        weights_conflicts[i].first = top_weights;
      }
      else
      {
        // merge the bottom list into the top
        top.edgelist.insert(
            top.edgelist.end(), bottom.edgelist.begin(), bottom.edgelist.end());

        // Sort the merged edgelist
        std::sort(top.edgelist.begin(), top.edgelist.end());
      }

      // Update degree
      top.degree = top.edgelist.size();

      // Now delete the last line of the i-1st file
      delete_last_line(i - 1, adjtype);
      // Now replace the first line of the i-th file
      if (is_weighted && weights_conflicts.find(i) != weights_conflicts.end())
      {
        replace_first_line(i, adjtype, top, weights_conflicts[i].first);
      }
      else
      {
        replace_first_line(i, adjtype, top);
      }
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
