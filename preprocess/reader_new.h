#ifndef READER_NEW_H
#define READER_NEW_H
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common_defs.h"
#include "common_util.h"

// ---------------------------------------------------------------------------
// compute_byte_ranges: divide file 'path' into n line-aligned byte chunks.
// Returns n+1 offsets; offsets[0]=0, offsets[n]=file_size.
// Each interior offset is placed at the start of the first line whose
// beginning is >= the naive (i * file_size / n) split point.
// ---------------------------------------------------------------------------
inline std::vector<std::streamoff> compute_byte_ranges(const std::string& path,
                                                        int n)
{
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f.is_open())
    throw GraphException("compute_byte_ranges: cannot open " + path);
  std::streamoff file_size = f.tellg();

  std::vector<std::streamoff> offsets(n + 1);
  offsets[0] = 0;
  offsets[n] = file_size;

  for (int i = 1; i < n; i++)
  {
    std::streamoff candidate =
        static_cast<std::streamoff>((long long)i * file_size / n);
    f.seekg(candidate);
    char ch;
    while (f.get(ch) && ch != '\n')
    {
    }
    std::streamoff aligned = f.tellg();
    offsets[i] = (aligned < 0 || aligned > file_size) ? file_size : aligned;
  }
  return offsets;
}

// ---------------------------------------------------------------------------
// ByteRangeEdgeReader: reads lines from [byte_start, byte_end) of input_path.
//
// Out-adjacency: streamed to body_filepath in 1000-node batches.
//   Only the first and last src-node adjlists are kept in memory for the
//   boundary merge step.  Memory = O(buffer_size) not O(E/N).
//
// In-adjacency: accumulated in in_map (unordered_map<dst, adjlist>).
//   Caller merges across threads after the parallel phase.
// ---------------------------------------------------------------------------
class ByteRangeEdgeReader
{
 public:
  static constexpr int OUT_BUFFER_SIZE = 1000;

  // Out-adjacency boundary adjlists (kept in memory for merge step).
  adjlist first_out_conflict;
  adjlist last_out_conflict;
  bool    has_data         = false;  // any src nodes processed
  bool    last_is_separate = false;  // last != first (2+ distinct src nodes)

  // In-adjacency: full map for this thread's src range.
  std::unordered_map<node_id_t, adjlist> in_map;

  ByteRangeEdgeReader(const std::string& input_path,
                      std::streamoff byte_start,
                      std::streamoff byte_end,
                      const std::string& body_filepath,
                      bool is_weighted = false)
      : range_end_(byte_end), is_weighted_(is_weighted)
  {
    file_.open(input_path, std::ios::in);
    if (!file_.is_open())
      throw GraphException("ByteRangeEdgeReader: cannot open " + input_path);
    file_.seekg(byte_start);

    body_file_.open(body_filepath, std::ios::out);
    if (!body_file_.is_open())
      throw GraphException("ByteRangeEdgeReader: cannot open body file " +
                           body_filepath);
    out_buffer_.reserve(OUT_BUFFER_SIZE + 1);
  }

  void build_adjlists()
  {
    std::string line;
    adjlist cur_out;
    bool has_cur = false;

    while (true)
    {
      std::streamoff pos = file_.tellg();
      if (pos < 0 || pos >= range_end_) break;
      if (!std::getline(file_, line)) break;
      if (line.empty()) continue;

      const char* ptr = line.c_str();
      char* end_ptr;
      node_id_t src = static_cast<node_id_t>(strtoull(ptr, &end_ptr, 10));
      node_id_t dst = static_cast<node_id_t>(strtoull(end_ptr, &end_ptr, 10));
      if (is_weighted_) strtod(end_ptr, nullptr);  // weight unused for now

      // --- Out-adjacency ---
      if (!has_cur)
      {
        cur_out.node_id = src;
        has_cur = true;
      }
      if (src == cur_out.node_id)
      {
        cur_out.edgelist.push_back(dst);
      }
      else
      {
        sort_and_enqueue(cur_out);
        cur_out.clear();
        cur_out.node_id = src;
        cur_out.edgelist.push_back(dst);
      }

      // --- In-adjacency ---
      auto& in_adj = in_map[dst];
      in_adj.node_id = dst;
      in_adj.edgelist.push_back(src);
    }

    if (has_cur && !cur_out.edgelist.empty())
      sort_and_enqueue(cur_out);

    // Drain the buffer: everything except the last entry goes to body.
    // The last entry becomes last_out_conflict.
    if (!out_buffer_.empty())
    {
      for (size_t k = 0; k + 1 < out_buffer_.size(); k++)
        write_entry(body_file_, out_buffer_[k]);
      last_out_conflict  = out_buffer_.back();
      last_is_separate   = (last_out_conflict.node_id !=
                            first_out_conflict.node_id);
    }

    file_.close();
    body_file_.close();
  }

 private:
  std::ifstream file_;
  std::ofstream body_file_;
  std::streamoff range_end_;
  bool is_weighted_;
  std::vector<adjlist> out_buffer_;

  static void write_entry(std::ofstream& f, const adjlist& adj)
  {
    f << adj.node_id << " " << adj.edgelist.size() << " ";
    for (size_t i = 0; i < adj.edgelist.size(); i++)
    {
      f << adj.edgelist[i];
      if (i + 1 < adj.edgelist.size()) f << ",";
    }
    f << "\n";
  }

  // Sort the completed adjlist, record first_out_conflict on the first call,
  // then push into out_buffer_.  When the buffer reaches OUT_BUFFER_SIZE,
  // flush all but the last entry to body_file_.
  void sort_and_enqueue(adjlist& adj)
  {
    std::sort(adj.edgelist.begin(), adj.edgelist.end());
    adj.degree = static_cast<degree_t>(adj.edgelist.size());

    if (!has_data)
    {
      first_out_conflict = adj;
      has_data = true;
      return;  // first node: keep in memory, not in buffer
    }

    out_buffer_.push_back(adj);

    if (static_cast<int>(out_buffer_.size()) >= OUT_BUFFER_SIZE)
    {
      // Flush all but the last entry (which stays as the potential last_out).
      for (size_t k = 0; k + 1 < out_buffer_.size(); k++)
        write_entry(body_file_, out_buffer_[k]);
      adjlist tmp = std::move(out_buffer_.back());
      out_buffer_.clear();
      out_buffer_.push_back(std::move(tmp));
    }
  }
};

namespace reader
{

std::vector<edge> parse_edge_entries(std::string filename);
std ::vector<node> parse_node_entries(std::string filename);

typedef struct weight_array
{
  node_id_t node_id;
  std::vector<edgeweight_t> arr;
} weight_array;


class EdgeReader
{
 private:
  std::string filename;
  std::string weights_filename;
  int beg_offset;
  int num_per_chunk;
  std::ifstream edge_file;
  std::ofstream adj_file, edge_file_txt, weights_file;
  adjlist node_adj_list, first_conflict, last_conflict;
  
  weight_array weights, w_conflict1, w_conflict2;

  node_id_t last_node_id = 0;
  long max_pos = 0;
  bool first = false;
  int cur_pos = 0;
  bool is_weighted = false;

 public:
  EdgeReader(const std::string& _filename,
             int _beg,
             int _num,
             const std::string& adj_type,
             bool weighted = false)
  {
    filename = _filename;
    is_weighted = weighted;
    beg_offset = _beg;
    cur_pos = beg_offset;
    num_per_chunk = _num;
    std::ios::sync_with_stdio(false);
        edge_file = std::ifstream(filename, std::ifstream::in);
        //check if the file is open
        if (!edge_file.is_open())
        {
            throw GraphException("Failed to open the edge file for " + filename);
        }

        std::string dirname = filename.substr(0, filename.find_last_of('/'));
        // strip the filename of anything after_
        std::string::size_type pos = filename.find_last_of('_');
        if (pos != std::string::npos)
        {
            std::string suffix = filename.substr(pos, filename.size());
            filename = dirname + "/" + adj_type + suffix;
            weights_filename = dirname + "/" + adj_type + "_weights" + suffix;
        }

        std::cout <<"Opening for writing: " << filename << std::endl;
        adj_file = std::ofstream(filename, std::ofstream::out);
        if (!adj_file.is_open())
        {
            throw GraphException("Failed to create the " + adj_type +
                                 " adjacency file for " + filename);
        }
        if (is_weighted)
        {
          std::cout <<"Opening for writing: " << weights_filename << std::endl;
          weights_file = std::ofstream(weights_filename, std::ofstream::out);
          if (!weights_file.is_open())
          {
              throw GraphException("Failed to create the weights file for " +
                                  weights_filename);
          }
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
      if (is_weighted)
      {
        s_stream >> e.edge_weight;
      }
      

      if (cur_pos == beg_offset)
      {
        node_adj_list.node_id = e.src_id;
        node_adj_list.edgelist.push_back(e.dst_id);
        node_adj_list.degree += 1;
        if (is_weighted)
        {
          weights.arr.push_back(e.edge_weight);
        }
        first = true;
        cur_pos++;
        continue;
      }

      if (e.src_id == node_adj_list.node_id)
      {
        node_adj_list.edgelist.push_back(e.dst_id);
        node_adj_list.degree += 1;
        if (is_weighted)
        {
          weights.arr.push_back(e.edge_weight);
        }
      }
      else  // new node
      {
        write_adjlist_to_file();
        if (is_weighted) write_edge_weights_to_file();
        if (first)
        {
          first_conflict.edgelist = std::move(node_adj_list.edgelist);
          first_conflict.node_id = node_adj_list.node_id;
          w_conflict1 = std::move(weights);
          first = false;
        }
        node_adj_list.clear();
        if (is_weighted) weights.arr.clear();
        node_adj_list.node_id = e.src_id;
        node_adj_list.edgelist.push_back(e.dst_id);
        node_adj_list.degree = 1;
        if (is_weighted)
        {
          weights.arr.push_back(e.edge_weight);
        }
      }
      cur_pos++;
    }
    write_adjlist_to_file();
    if (is_weighted) write_edge_weights_to_file();

    last_conflict.node_id = node_adj_list.node_id;
    last_conflict.edgelist = std::move(node_adj_list.edgelist);
    w_conflict2 = std::move(weights);
    node_adj_list.clear();
    if (is_weighted) weights.arr.clear();

    edge_file.close();
    edge_file_txt.close();
    adj_file.close();
    if (is_weighted) weights_file.close();
  }

  void write_adjlist_to_file()
  {
    // Sort edgelist and weights together before writing
    if (is_weighted && weights.arr.size() == node_adj_list.edgelist.size())
    {
      // Create a vector of pairs (edge, weight) for sorting
      std::vector<std::pair<node_id_t, edgeweight_t>> edge_weight_pairs;
      for (size_t i = 0; i < node_adj_list.edgelist.size(); i++)
      {
        edge_weight_pairs.push_back({node_adj_list.edgelist[i], weights.arr[i]});
      }

      // Sort by edge id
      std::sort(edge_weight_pairs.begin(), edge_weight_pairs.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });

      // Extract sorted edges and weights
      for (size_t i = 0; i < edge_weight_pairs.size(); i++)
      {
        node_adj_list.edgelist[i] = edge_weight_pairs[i].first;
        weights.arr[i] = edge_weight_pairs[i].second;
      }
    }
    else
    {
      // Sort edgelist only
      std::sort(node_adj_list.edgelist.begin(), node_adj_list.edgelist.end());
    }

    adj_file << node_adj_list.node_id << " " << node_adj_list.edgelist.size()
             << " ";
    for (size_t i = 0; i < node_adj_list.edgelist.size(); i++)
    {
      adj_file << node_adj_list.edgelist[i];
      if (i != node_adj_list.edgelist.size() - 1)
      {
        adj_file << ",";
      }
    }
    adj_file << "\n";
  }

  void write_edge_weights_to_file()
  {
    assert(node_adj_list.edgelist.size() == weights.arr.size());
    weights_file << node_adj_list.node_id << " ";
    for (size_t i = 0; i < weights.arr.size(); i++)
      {
        weights_file << weights.arr[i];
        if (i != weights.arr.size() - 1)
        {
          weights_file << ",";
        }
      }
      weights_file << "\n";
  }

  std::pair<adjlist, adjlist> get_conflict() const
  {
    std::pair<adjlist, adjlist> conflict;
    conflict.first = first_conflict;
    conflict.second = last_conflict;
    return conflict;
  }

  std::pair<std::vector<edgeweight_t>, std::vector<edgeweight_t>> get_conflict_weights() const
  {
    return std::make_pair(w_conflict1.arr, w_conflict2.arr);
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

// read from boost archive file
class WeightsReader
{
 private:
  std::ifstream weights_file;
  int length = 0;

 public:
  explicit WeightsReader(const std::string& filename, bool weighted = true)
  {
    if (!weighted) return;
    weights_file = std::ifstream(filename, std::ifstream::in);
    if (!weights_file.is_open())
    {
      throw GraphException("Failed to open the weights file for " + filename);
    }
  }
  
  // each line has a node id, it's degree, and the list of neighbors, all
  // separated by spaces getline from file and parse the line
  int get_next_adjlist(weight_array& adj)
  {
    std::string line;
    if (getline(weights_file, line))
    {
      std::istringstream iss(line);
      if (iss >> adj.node_id)
      {
        edge_id_t n;
        while (iss >> n)
        {
          adj.arr.push_back(n);
          if (iss.peek() == ',')
          {
            iss.ignore();
          }
        }
      }
      else
      {
        weights_file.close();
        std::cerr << "Error reading from file" << std::endl;
        return -1;
      }
      // adj.degree = adj.edgelist.size();n
      return 0;
    }
    else
    {
      weights_file.close();
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
