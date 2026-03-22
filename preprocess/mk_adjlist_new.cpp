//
// Created by puneet on 29/01/24.
// Rewritten: K-pass partitioned CSR scatter for in-adjacency.
//
// Overview of phases:
//
//   Phase 1  — Parallel count + out-adj streaming
//              T threads each read a byte-range of the input file.
//              Out-adj entries are streamed to per-thread body files on disk
//              (unchanged from before).  In-adj counts are accumulated in a
//              single shared in_degree[dst] array via atomic increments.
//
//   Phase 2  — Sequential prefix sum
//              offset[dst] = sum of in_degree[0..dst-1].
//              After this, offset[dst] is the global start of dst's slice
//              in the hypothetical flat in_adj[total_edges] array.
//
//   Phase 3  — Sequential band boundary computation
//              Divide [0, max_node_id] into K bands so that each band's
//              total in-edge count fits within the memory budget.
//              K = ceil(total_edges / budget_edges).
//
//   Phase 4  — K scatter passes (one per band), each parallel
//     4a  Allocate in_adj_band[band_edges] and write_pos[band_ids].
//     4b  T threads re-read the file, scattering src into in_adj_band
//         for any edge whose dst falls in this band.
//     4c  Parallel sort each dst's slice (dynamic schedule for load balance).
//     4d  Parallel write/append to per-thread in_* output files.
//     4e  Free in_adj_band and write_pos (vectors go out of scope).
//
// Memory at peak (during a scatter pass):
//   in_degree   ~4 GB  (1B nodes × 4 B, always resident)
//   offset      ~8 GB  (1B nodes × 8 B, always resident)
//   write_pos   ≤8 GB  (one band's slice of offsets, 8 B each)
//   in_adj_band ≤budget (released after each band)
//   read buffer ~1 GB
//   ────────────────────────
//   peak        fits in 128 GB for typical large graphs with budget=100 GB
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

  int       num_threads = opts.num_threads;
  bool      is_weighted = opts.is_weighted;

  // max_node_id: the highest node ID present in the graph.
  // Sizes the in_degree and offset arrays.
  // If -N is not passed, fall back to num_nodes - 1 (correct when IDs are
  // dense and start at 0, e.g. Kron graphs).
  node_id_t max_node_id = (params.max_node_id() != 0)
                              ? params.max_node_id()
                              : static_cast<node_id_t>(opts.num_nodes - 1);

  // budget_edges: maximum number of in-adj entries held in RAM at once
  // (for one band's in_adj_band allocation).  Derived from -M mem_gb.
  uint64_t budget_edges = params.budget_edges();

  std::cout << "Input:        " << input_path  << "\n"
            << "Output dir:   " << output_dir  << "\n"
            << "Threads:      " << num_threads  << "\n"
            << "Weighted:     " << is_weighted  << "\n"
            << "Max node ID:  " << max_node_id  << "\n"
            << "Budget:       " << budget_edges << " edges (~"
            << (budget_edges * sizeof(node_id_t) >> 30) << " GB)\n";

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

  // =========================================================================
  // Phase 1: Parallel count pass + out-adj streaming
  //
  // Each thread reads its byte range, streams out-adj body files to disk
  // (unchanged), and atomically increments in_degree[dst] for every edge.
  //
  // in_degree is shared; #pragma omp atomic inside count_and_build_out
  // ensures correctness without a mutex or per-thread copies.
  // =========================================================================
  std::cout << "\nPhase 1: count pass + out-adj streaming...\n";

  // in_degree[i] = number of edges with dst == i.
  // Sized max_node_id + 1 so that IDs 0..max_node_id are all valid indices.
  std::vector<uint32_t> in_degree(static_cast<size_t>(max_node_id) + 1, 0);

#pragma omp parallel for num_threads(num_threads)
  for (int i = 0; i < num_threads; i++)
  {
    std::string body_path = output_dir + "/out_body_" + thread_suffix(i);

    ByteRangeEdgeReader rdr(input_path,
                            byte_offsets[i],
                            byte_offsets[i + 1],
                            body_path,
                            is_weighted);
    rdr.count_and_build_out(in_degree);  // out-adj → disk; in-deg → in_degree

    first_out[i]        = std::move(rdr.first_out_conflict);
    last_out[i]         = std::move(rdr.last_out_conflict);
    has_data[i]         = rdr.has_data;
    last_is_separate[i] = rdr.last_is_separate;
  }

  // Merge and assemble out-adjacency files (logic unchanged from before).
  merge_out_boundaries(first_out, last_out, has_data, last_is_separate,
                       omit_last, num_threads);
  assemble_out_files(output_dir, first_out, last_out, has_data,
                     last_is_separate, omit_last, num_threads);

  // =========================================================================
  // Phase 2: Prefix sum
  //
  // offset[dst] = sum of in_degree[0..dst-1].
  // After this, the in-neighbor list for node dst occupies global positions
  // [offset[dst], offset[dst] + in_degree[dst]) in a hypothetical flat array.
  // =========================================================================
  std::cout << "\nPhase 2: prefix sum...\n";

  std::vector<uint64_t> offset;
  uint64_t total_edges = compute_prefix_sum(in_degree, offset);
  std::cout << "  Total in-edges: " << total_edges << "\n";

  // =========================================================================
  // Phase 3: Band boundary computation
  //
  // Divide [0, max_node_id] into K bands, each holding at most budget_edges
  // in-neighbors.  Bands have equal edge counts so every scatter pass
  // allocates the same memory and performs the same amount of I/O.
  // =========================================================================
  std::cout << "\nPhase 3: computing band boundaries...\n";

  auto bands = compute_band_bounds(in_degree, budget_edges);
  std::cout << "  K = " << bands.size() << " band(s)\n";

  // Pre-create (truncate) all T in-adj output files so that band writes
  // can unconditionally append without worrying about stale content.
  truncate_in_files(output_dir, num_threads);

  // =========================================================================
  // Phase 4: K scatter passes — one per band
  //
  // For each band [lo, hi]:
  //   4a  Allocate in_adj_band (holds all in-neighbors for dst in [lo, hi]).
  //   4b  init write_pos[dst - lo] = offset[dst]  (per-node write cursor).
  //   4c  T threads re-read the file in parallel, each writing src into
  //       in_adj_band at an atomically-reserved slot.
  //   4d  Sort each dst's slice in parallel (dynamic schedule handles hubs).
  //   4e  Write sorted slices to in_* files (parallel, append mode).
  //   4f  in_adj_band and write_pos are freed as they go out of scope.
  // =========================================================================
  std::cout << "\nPhase 4: scatter passes...\n";

  for (size_t k = 0; k < bands.size(); k++)
  {
    node_id_t lo         = bands[k].first;
    node_id_t hi         = bands[k].second;
    uint64_t  band_base  = offset[lo];
    uint64_t  band_edges = offset[hi] + in_degree[hi] - band_base;

    std::cout << "  band " << k + 1 << "/" << bands.size()
              << "  dst=[" << lo << "," << hi << "]"
              << "  edges=" << band_edges << "\n";

    // 4a: allocate the band buffer (freed at end of loop iteration)
    std::vector<node_id_t> in_adj_band(band_edges);

    // 4b: write_pos[dst - lo] is each dst's write cursor, starting at
    //     offset[dst].  Threads advance it atomically during scatter.
    std::vector<uint64_t> write_pos(hi - lo + 1);
    for (node_id_t dst = lo; dst <= hi; dst++)
      write_pos[dst - lo] = offset[dst];

    // 4c: parallel scatter — T threads, same byte ranges as Phase 1
#pragma omp parallel for num_threads(num_threads)
    for (int i = 0; i < num_threads; i++)
    {
      scatter_band_range(input_path,
                         byte_offsets[i],
                         byte_offsets[i + 1],
                         is_weighted,
                         in_adj_band.data(),
                         write_pos.data(),
                         lo, hi, band_base);
    }

    // 4d: parallel sort of each dst's in-neighbor list within the band.
    //     dynamic schedule because hub nodes have O(M) neighbors while
    //     most nodes have O(1) — static would leave threads idle.
#pragma omp parallel for schedule(dynamic) num_threads(num_threads)
    for (int64_t dst_s = static_cast<int64_t>(lo);
         dst_s <= static_cast<int64_t>(hi);
         dst_s++)
    {
      node_id_t dst = static_cast<node_id_t>(dst_s);
      if (in_degree[dst] < 2) continue;  // 0 or 1 element: already sorted
      node_id_t* begin = in_adj_band.data() + (offset[dst] - band_base);
      std::sort(begin, begin + in_degree[dst]);
    }

    // 4e: append sorted in-adj entries for this band to the in_* files
    write_band_adjlists(in_adj_band.data(), offset, in_degree,
                        lo, hi, band_base, output_dir, num_threads);

    // 4f: in_adj_band and write_pos go out of scope here and are freed.
  }

  std::cout << "\nmk_adjlists_new done.\n";
  return 0;
}
