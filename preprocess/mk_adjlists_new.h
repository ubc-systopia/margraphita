//
// Created by puneet on 30/01/24.
// Rewritten: byte-range single-pass preprocessing (no external sort/split).
//
// In-adjacency is now built with a K-pass partitioned CSR scatter:
//   Phase 1  (here)  count_and_build_out  — count in-degrees, stream out-adj
//   Phase 2  (here)  compute_prefix_sum   — prefix sum → slot offsets
//   Phase 3  (here)  compute_band_bounds  — split ID space into memory-fitting bands
//   Phase 4  (here)  scatter_band_range   — scatter in-neighbors for one band
//            (here)  write_band_adjlists  — sort & write one band to in_* files
//

#ifndef GRAPHAPI_MK_ADJLISTS_NEW_H
#define GRAPHAPI_MK_ADJLISTS_NEW_H

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "reader_new.h"
#include "time_structs.h"

// ---------------------------------------------------------------------------
// MkAdjlistsOpts: extends InsertOpts with:
//   -i <input_file>    original (unsplit) edge-list file
//   -N <max_node_id>   highest node ID in the graph (sizes in_degree/offset)
//   -M <mem_gb>        RAM budget in GB; controls band size (default: 100)
// ---------------------------------------------------------------------------
class MkAdjlistsOpts : public InsertOpts
{
 private:
  std::string input_path_;
  node_id_t   max_node_id_    = 0;   // 0 means "derive from num_nodes - 1"
  uint64_t    mem_gb_         = 100; // default: 100 GB
  uint64_t    budget_edges_   = 0;   // 0 means "derive from mem_gb_"

 public:
  MkAdjlistsOpts(int argc, char** argv) : InsertOpts(argc, argv)
  {
    argstr_ += "i:N:M:B:";
    add_help_message('i', "input",        "Path to original (unsplit) edge-list file");
    add_help_message('N', "max_node_id",  "Highest node ID (sizes in_degree/offset arrays)");
    add_help_message('M', "mem_gb",       "RAM budget in GB for scatter bands (default 100)");
    add_help_message('B', "budget_edges", "Direct budget in edges (overrides -M; for testing)");
  }

  bool handle_args(signed char opt, char* opt_arg) override
  {
    if (opt == 'i') { input_path_    = opt_arg; return true; }
    if (opt == 'N') { max_node_id_   = static_cast<node_id_t>(strtoull(opt_arg, nullptr, 10)); return true; }
    if (opt == 'M') { mem_gb_        = strtoull(opt_arg, nullptr, 10); return true; }
    if (opt == 'B') { budget_edges_  = strtoull(opt_arg, nullptr, 10); return true; }
    return InsertOpts::handle_args(opt, opt_arg);
  }

  const std::string& input_path()  const { return input_path_; }
  node_id_t          max_node_id() const { return max_node_id_; }

  // budget_edges: maximum number of in-adj entries kept in RAM for one band.
  // -B sets it directly (useful for testing small bands without huge graphs).
  // Otherwise we take 70 % of -M GB so the always-resident in_degree and
  // offset arrays (12 bytes × (max_id+1)) fit in the remaining 30 %.
  uint64_t budget_edges() const
  {
    if (budget_edges_ > 0) return budget_edges_;
    uint64_t bytes = mem_gb_ * (1ULL << 30);
    return static_cast<uint64_t>(bytes * 0.70) / sizeof(node_id_t);
  }
};

// ---------------------------------------------------------------------------
// thread_suffix: convert thread index to two-character suffix (aa, ab, …)
// ---------------------------------------------------------------------------
static inline std::string thread_suffix(int tid)
{
  std::string s;
  s.push_back(static_cast<char>(97 + tid / 26));
  s.push_back(static_cast<char>(97 + tid % 26));
  return s;
}

// ---------------------------------------------------------------------------
// write_adjlist_entry: write one adjlist struct to an open stream.
// Format: "node_id degree nbr1,nbr2,...\n"
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
// write_adjlist_file: write a vector<adjlist> to a named file (used for
// small cases; the band writer below handles the large-graph path).
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

// ===========================================================================
// Out-adjacency helpers (unchanged from original design)
// ===========================================================================

// ---------------------------------------------------------------------------
// merge_out_boundaries: purely in-memory.
// For each adjacent thread pair (i-1, i): if last_out[i-1] and first_out[i]
// share the same node_id, merge their edgelists into first_out[i] and mark
// omit_last[i-1] = true so assembly skips writing last_out[i-1].
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

    const adjlist& prev_last  = last_out[i - 1];
    adjlist&       curr_first = first_out[i];

    if (prev_last.node_id == curr_first.node_id)
    {
      curr_first.edgelist.insert(curr_first.edgelist.end(),
                                 prev_last.edgelist.begin(),
                                 prev_last.edgelist.end());
      std::sort(curr_first.edgelist.begin(), curr_first.edgelist.end());
      curr_first.degree = static_cast<degree_t>(curr_first.edgelist.size());
      omit_last[i - 1]  = true;
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
    std::string suf        = thread_suffix(i);
    std::string body_path  = output_dir + "/out_body_" + suf;
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

// ===========================================================================
// In-adjacency helpers — CSR K-pass scatter
// ===========================================================================

// ---------------------------------------------------------------------------
// compute_prefix_sum: given in_degree[0..N-1], fill offset[0..N-1] where
//   offset[i] = sum of in_degree[0..i-1]  (i.e., offset[0] = 0).
//
// Returns total_edges = offset[N-1] + in_degree[N-1].
//
// After this call, for any node dst:
//   its in-neighbor slice in the global flat array is
//   [offset[dst], offset[dst] + in_degree[dst]).
// ---------------------------------------------------------------------------
static uint64_t compute_prefix_sum(const std::vector<uint32_t>& in_degree,
                                    std::vector<uint64_t>& offset)
{
  size_t N = in_degree.size();
  offset.resize(N, 0);
  if (N == 0) return 0;

  offset[0] = 0;
  for (size_t i = 1; i < N; i++)
    offset[i] = offset[i - 1] + in_degree[i - 1];

  return offset[N - 1] + in_degree[N - 1];
}

// ---------------------------------------------------------------------------
// compute_band_bounds: divide [0, max_id] into K bands where each band
// contains at most budget_edges in-edges.
//
// Bands cover equal edge counts (not equal ID counts), so every scatter pass
// allocates the same sized in_adj_band buffer and does the same I/O work.
//
// Returns a vector of (lo, hi) pairs that partition [0, max_id] exactly.
// ---------------------------------------------------------------------------
static std::vector<std::pair<node_id_t, node_id_t>>
compute_band_bounds(const std::vector<uint32_t>& in_degree,
                    uint64_t budget_edges)
{
  std::vector<std::pair<node_id_t, node_id_t>> bands;
  node_id_t N = static_cast<node_id_t>(in_degree.size());
  if (N == 0) return bands;

  node_id_t band_lo = 0;
  uint64_t  running = 0;

  for (node_id_t i = 0; i < N; i++)
  {
    running += in_degree[i];
    // Close the current band when it would exceed budget, but never close
    // the last node prematurely (the final band must reach N-1).
    if (running >= budget_edges && i < N - 1)
    {
      bands.push_back({band_lo, i});
      band_lo = i + 1;
      running = 0;
    }
  }
  bands.push_back({band_lo, N - 1});  // final (possibly only) band
  return bands;
}

// ---------------------------------------------------------------------------
// truncate_in_files: create (or truncate to zero) all T in-adj output files
// before the band loop starts.  Each band then appends to these files.
// ---------------------------------------------------------------------------
static void truncate_in_files(const std::string& output_dir, int num_threads)
{
  for (int t = 0; t < num_threads; t++)
  {
    std::ofstream f(output_dir + "/in_" + thread_suffix(t),
                    std::ios::out | std::ios::trunc);
    // File is created empty; write_band_adjlists will append to it.
  }
}

// ---------------------------------------------------------------------------
// write_band_adjlists: write the sorted in-adjacency data for all nodes with
// dst in [lo, hi] to the T per-thread in_* files (appending).
//
// The in-neighbor list for dst lives at:
//   in_adj_band[ offset[dst] - band_base  ..  +in_degree[dst] - 1 ]
// and is already sorted by the caller.
//
// Nodes with in_degree[dst] == 0 are skipped (no in-adj entry written).
//
// The [lo, hi] range is divided into T equal-ID-count chunks so that
// threads write disjoint sections of their files concurrently.
// ---------------------------------------------------------------------------
static void write_band_adjlists(const node_id_t*           in_adj_band,
                                 const std::vector<uint64_t>& offset,
                                 const std::vector<uint32_t>& in_degree,
                                 node_id_t                  lo,
                                 node_id_t                  hi,
                                 uint64_t                   band_base,
                                 const std::string&         output_dir,
                                 int                        num_threads)
{
  node_id_t total_ids = hi - lo + 1;
  // chunk size: ceiling division so all IDs are covered
  node_id_t chunk = (total_ids + static_cast<node_id_t>(num_threads) - 1)
                    / static_cast<node_id_t>(num_threads);

#pragma omp parallel for num_threads(num_threads)
  for (int t = 0; t < num_threads; t++)
  {
    node_id_t t_lo = lo + static_cast<node_id_t>(t) * chunk;
    if (t_lo > hi) continue;
    node_id_t t_hi = std::min(t_lo + chunk - 1, hi);

    std::ofstream f(output_dir + "/in_" + thread_suffix(t), std::ios::app);
    if (!f.is_open())
      throw GraphException("write_band_adjlists: cannot open file for thread "
                           + std::to_string(t));

    for (node_id_t dst = t_lo; dst <= t_hi; dst++)
    {
      if (in_degree[dst] == 0) continue;  // no in-neighbors, skip

      degree_t          deg   = static_cast<degree_t>(in_degree[dst]);
      const node_id_t*  nbrs  = in_adj_band + (offset[dst] - band_base);

      f << dst << " " << deg << " ";
      for (degree_t k = 0; k < deg; k++)
      {
        f << nbrs[k];
        if (k + 1 < deg) f << ",";
      }
      f << "\n";
    }
  }
}

#endif  // GRAPHAPI_MK_ADJLISTS_NEW_H
