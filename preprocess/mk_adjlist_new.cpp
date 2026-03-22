//
// Created by puneet on 29/01/24.
// Rewritten: single-pass byte-range preprocessing (no external sort/split).
//
#include "mk_adjlists_new.h"

#include <sys/stat.h>

#include <iostream>

#include "common_util.h"

int main(int argc, char** argv)
{
  MkAdjlistsOpts params(argc, argv);
  if (!params.parse_args())
  {
    params.print_help();
    return -1;
  }
  graph_opts opts = params.make_graph_opts();

  // -f is the output prefix (e.g., /scratch/preprocess/knows).
  // The output directory is its parent directory.
  const std::string& output_prefix = opts.dataset;
  std::string output_dir;
  {
    size_t slash = output_prefix.find_last_of("/\\");
    output_dir = (slash != std::string::npos) ? output_prefix.substr(0, slash)
                                              : ".";
  }

  // -i is the original (unsplit) input edge-list file.
  // Fall back to -f if -i is not given (backward compatibility).
  const std::string& input_path = params.input_path().empty()
                                      ? output_prefix
                                      : params.input_path();

  int num_threads = opts.num_threads;
  bool is_weighted = opts.is_weighted;

  std::cout << "Input file:  " << input_path << "\n"
            << "Output dir:  " << output_dir << "\n"
            << "Num threads: " << num_threads << "\n"
            << "Weighted:    " << is_weighted << "\n";

  mkdir(output_dir.c_str(), 0755);

  auto byte_offsets = compute_byte_ranges(input_path, num_threads);

  // Per-thread out-adjacency boundary entries (O(num_threads) total memory).
  // Use int instead of bool: vector<bool> packs bits and is not thread-safe
  // for concurrent element-wise writes to different indices.
  std::vector<adjlist> first_out(num_threads);
  std::vector<adjlist> last_out(num_threads);
  std::vector<int>     has_data(num_threads, 0);
  std::vector<int>     last_is_separate(num_threads, 0);
  std::vector<int>     omit_last(num_threads, 0);

  // Per-thread in-adjacency maps (O(E) total — unavoidable for correctness).
  std::vector<std::unordered_map<node_id_t, adjlist>> per_thread_in_maps(
      num_threads);

  // --- Parallel phase ---
#pragma omp parallel for num_threads(num_threads)
  for (int i = 0; i < num_threads; i++)
  {
    std::string body_path =
        output_dir + "/out_body_" + thread_suffix(i);

    ByteRangeEdgeReader rdr(input_path,
                            byte_offsets[i],
                            byte_offsets[i + 1],
                            body_path,
                            is_weighted);
    rdr.build_adjlists();

    first_out[i]       = std::move(rdr.first_out_conflict);
    last_out[i]        = std::move(rdr.last_out_conflict);
    has_data[i]        = rdr.has_data;
    last_is_separate[i] = rdr.last_is_separate;
    per_thread_in_maps[i] = std::move(rdr.in_map);
  }

  // --- Sequential: merge out-adjacency boundaries (pure in-memory) ---
  merge_out_boundaries(
      first_out, last_out, has_data, last_is_separate, omit_last, num_threads);

  // --- Parallel: assemble final out files (first + body stream + last) ---
  assemble_out_files(
      output_dir, first_out, last_out, has_data, last_is_separate, omit_last,
      num_threads);

  // --- Sequential: merge all in-adjacency maps ---
  auto merged_in   = merge_all_in_maps(per_thread_in_maps);
  auto per_thread_in = split_into_chunks(merged_in, num_threads);

  std::cout << "Unique in-nodes: " << merged_in.size() << "\n";

  // --- Parallel: write in files ---
#pragma omp parallel for num_threads(num_threads)
  for (int i = 0; i < num_threads; i++)
  {
    write_adjlist_file(per_thread_in[i],
                       output_dir + "/in_" + thread_suffix(i));
  }

  std::cout << "mk_adjlists_new done.\n";
  return 0;
}
