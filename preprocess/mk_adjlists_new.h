//
// Created by puneet on 30/01/24.
// Rewritten: byte-range single-pass preprocessing (no external sort/split).
//

#ifndef GRAPHAPI_MK_ADJLISTS_NEW_H
#define GRAPHAPI_MK_ADJLISTS_NEW_H

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "reader_new.h"
#include "time_structs.h"

// ---------------------------------------------------------------------------
// MkAdjlistsOpts: extends InsertOpts with -i <input_file>
// ---------------------------------------------------------------------------
class MkAdjlistsOpts : public InsertOpts
{
 private:
  std::string input_path_;

 public:
  MkAdjlistsOpts(int argc, char** argv) : InsertOpts(argc, argv)
  {
    argstr_ += "i:";
    add_help_message('i', "input", "Path to original (unsplit) edge-list file");
  }

  bool handle_args(signed char opt, char* opt_arg) override
  {
    if (opt == 'i')
    {
      input_path_ = opt_arg;
      return true;
    }
    return InsertOpts::handle_args(opt, opt_arg);
  }

  const std::string& input_path() const { return input_path_; }
};

// ---------------------------------------------------------------------------
// thread_suffix: convert thread id to two-character suffix (aa, ab, ...)
// ---------------------------------------------------------------------------
static inline std::string thread_suffix(int tid)
{
  std::string s;
  s.push_back(static_cast<char>(97 + tid / 26));
  s.push_back(static_cast<char>(97 + tid % 26));
  return s;
}

// ---------------------------------------------------------------------------
// write_adjlist_entry: write one adjlist to an open stream.
// ---------------------------------------------------------------------------
static void write_adjlist_entry(std::ostream& f, const adjlist& adj)
{
  f << adj.node_id << " " << adj.edgelist.size() << " ";
  for (size_t i = 0; i < adj.edgelist.size(); i++)
  {
    f << adj.edgelist[i];
    if (i + 1 < adj.edgelist.size()) f << ",";
  }
  f << "\n";
}

// ---------------------------------------------------------------------------
// write_adjlist_file: write a vector<adjlist> to a file.
// Used for in-adjacency output (still in-memory after merge).
// ---------------------------------------------------------------------------
static void write_adjlist_file(const std::vector<adjlist>& adjlists,
                                const std::string& filepath)
{
  std::ofstream f(filepath);
  if (!f.is_open())
    throw GraphException("write_adjlist_file: cannot open " + filepath);
  for (const auto& adj : adjlists)
    write_adjlist_entry(f, adj);
}

// ---------------------------------------------------------------------------
// merge_out_boundaries: purely in-memory.
// For each adjacent thread pair (i-1, i): if last_out[i-1] and first_out[i]
// share the same node_id, merge the edgelists into first_out[i] and set
// omit_last[i-1] = true so the assembly step skips writing last_out[i-1].
// ---------------------------------------------------------------------------
static void merge_out_boundaries(std::vector<adjlist>& first_out,
                                  std::vector<adjlist>& last_out,
                                  std::vector<int>& has_data,
                                  std::vector<int>& last_is_separate,
                                  std::vector<int>& omit_last,
                                  int num_threads)
{
  for (int i = 1; i < num_threads; i++)
  {
    if (!has_data[i - 1] || !has_data[i]) continue;

    const adjlist& prev_last = last_out[i - 1];
    adjlist&       curr_first = first_out[i];

    if (prev_last.node_id == curr_first.node_id)
    {
      curr_first.edgelist.insert(curr_first.edgelist.end(),
                                 prev_last.edgelist.begin(),
                                 prev_last.edgelist.end());
      std::sort(curr_first.edgelist.begin(), curr_first.edgelist.end());
      curr_first.degree = static_cast<degree_t>(curr_first.edgelist.size());
      omit_last[i - 1] = true;
    }
  }
}

// ---------------------------------------------------------------------------
// assemble_out_files: for each thread, concatenate:
//   first_out  (one adjlist, in memory)
//   body file  (streamed from temp file, O(1) memory)
//   last_out   (one adjlist, in memory, unless omit_last or not separate)
// into the final out_<suffix> file, then delete the body temp file.
// ---------------------------------------------------------------------------
static void assemble_out_files(const std::string& output_dir,
                                std::vector<adjlist>& first_out,
                                std::vector<adjlist>& last_out,
                                const std::vector<int>& has_data,
                                const std::vector<int>& last_is_separate,
                                const std::vector<int>& omit_last,
                                int num_threads)
{
#pragma omp parallel for num_threads(num_threads)
  for (int i = 0; i < num_threads; i++)
  {
    std::string suf       = thread_suffix(i);
    std::string body_path = output_dir + "/out_body_" + suf;
    std::string final_path = output_dir + "/out_" + suf;

    std::ofstream out(final_path);
    if (!out.is_open())
      throw GraphException("assemble_out_files: cannot open " + final_path);

    if (has_data[i])
    {
      write_adjlist_entry(out, first_out[i]);

      // Stream body (avoids loading into memory).
      // Note: out << body.rdbuf() sets failbit if no characters are
      // transferred (empty body), so clear error state afterwards.
      {
        std::ifstream body(body_path, std::ios::binary);
        if (body.is_open()) out << body.rdbuf();
        out.clear();
      }

      if (last_is_separate[i] && !omit_last[i])
        write_adjlist_entry(out, last_out[i]);
    }

    std::remove(body_path.c_str());
  }
}

// ---------------------------------------------------------------------------
// merge_all_in_maps: merge per-thread in-adjacency maps into a single sorted
// vector<adjlist>.  Each edge (src,dst) appears in exactly one thread's
// in_map, but the same dst can have contributions from any thread.
// ---------------------------------------------------------------------------
static std::vector<adjlist> merge_all_in_maps(
    std::vector<std::unordered_map<node_id_t, adjlist>>& per_thread_in_maps)
{
  // Single-pass accumulation into one unordered_map
  std::unordered_map<node_id_t, adjlist> merged;
  merged.reserve(per_thread_in_maps.size() *
                 (per_thread_in_maps.empty() ? 0
                                             : per_thread_in_maps[0].size()));

  for (auto& in_map : per_thread_in_maps)
  {
    for (auto& [node_id, adj] : in_map)
    {
      auto& target = merged[node_id];
      target.node_id = node_id;
      target.edgelist.insert(
          target.edgelist.end(), adj.edgelist.begin(), adj.edgelist.end());
    }
  }

  // Convert to sorted vector, sort each edgelist
  std::vector<adjlist> result;
  result.reserve(merged.size());
  for (auto& [id, adj] : merged)
  {
    std::sort(adj.edgelist.begin(), adj.edgelist.end());
    adj.degree = static_cast<degree_t>(adj.edgelist.size());
    result.push_back(std::move(adj));
  }
  std::sort(result.begin(), result.end(),
            [](const adjlist& a, const adjlist& b)
            { return a.node_id < b.node_id; });
  return result;
}

// ---------------------------------------------------------------------------
// split_into_chunks: distribute a sorted adjlist vector evenly across
// num_threads sub-vectors (for parallel file writing).
// ---------------------------------------------------------------------------
static std::vector<std::vector<adjlist>> split_into_chunks(
    std::vector<adjlist>& all_adj, int num_threads)
{
  std::vector<std::vector<adjlist>> result(num_threads);
  if (all_adj.empty()) return result;

  size_t total = all_adj.size();
  size_t chunk = (total + num_threads - 1) / num_threads;

  for (int i = 0; i < num_threads; i++)
  {
    size_t beg = static_cast<size_t>(i) * chunk;
    size_t end = std::min(beg + chunk, total);
    if (beg >= total) break;
    result[i].assign(std::make_move_iterator(all_adj.begin() + beg),
                     std::make_move_iterator(all_adj.begin() + end));
  }
  return result;
}

#endif  // GRAPHAPI_MK_ADJLISTS_NEW_H
