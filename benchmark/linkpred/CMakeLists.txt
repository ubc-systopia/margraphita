add_executable(fg_linkpred_bench_db bench/fg_linkpred_bench_db.cpp)
# Sources live in subfolders (bench/, common/, direct/, pipeline-generators/,
# reference/, other/); headers are included by name, so every folder is on the path.
set(LINKPRED_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/direct
    ${CMAKE_CURRENT_SOURCE_DIR}/direct/exhaustive
    ${CMAKE_CURRENT_SOURCE_DIR}/direct/fof
    ${CMAKE_CURRENT_SOURCE_DIR}/direct/khop
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline-generators
    ${CMAKE_CURRENT_SOURCE_DIR}/reference)
target_include_directories(fg_linkpred_bench_db PRIVATE ${LINKPRED_INCLUDE_DIRS})
target_link_libraries(fg_linkpred_bench_db PUBLIC ${NAME_LIB} graph_utils)

# Tests live in tests/; they include linkpred headers by name, so add this folder.
add_executable(test_linkpred_common other/tests/test_linkpred_common.cpp)
target_include_directories(test_linkpred_common PRIVATE ${PATH_SRC} ${LINKPRED_INCLUDE_DIRS})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/run_s12_lp.sh"
  "${CMAKE_CURRENT_BINARY_DIR}/run_s12_lp.sh"
  COPYONLY
)
file(CHMOD "${CMAKE_CURRENT_BINARY_DIR}/run_s12_lp.sh"
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                 GROUP_READ GROUP_EXECUTE
                 WORLD_READ WORLD_EXECUTE)

# Cosmetic only — headers aren't compiled, but this makes them visible in IDEs
target_sources(fg_linkpred_bench_db PRIVATE
    common/linkpred_types.h
    common/linkpred_options.h
    common/linkpred_scores.h
    common/linkpred_topk.h
    common/linkpred_eval.h
    common/linkpred_output.h
    reference/linkpred_reference.h
    direct/exhaustive/linkpred_structural_exhaustive.h
    direct/exhaustive/linkpred_embedding_exhaustive.h
    pipeline-generators/linkpred_candidates.h
    direct/fof/linkpred_structural_fof.h
    direct/fof/linkpred_structural_fof_compact.h
    direct/fof/linkpred_embedding_fof.h
    direct/khop/linkpred_structural_khop.h
    direct/khop/linkpred_embedding_khop.h
)

add_executable(test_linkpred_candidates other/tests/test_linkpred_candidates.cpp)
target_include_directories(test_linkpred_candidates PRIVATE ${LINKPRED_INCLUDE_DIRS})
target_link_libraries(test_linkpred_candidates PUBLIC ${NAME_LIB})

# Microbenchmark: NodeCursor::next / get_node_properties vs cosine (ns per call).
# Source lives in ../microbenchmarks; built here because benchmark/CMakeLists.txt
# has add_subdirectory(microbenchmarks) commented out. Uses linkpred_scores.h.
add_executable(next_vs_cosine ../microbenchmarks/next_vs_cosine.cpp)
target_include_directories(next_vs_cosine PRIVATE ${LINKPRED_INCLUDE_DIRS})
target_link_libraries(next_vs_cosine PUBLIC ${NAME_LIB} graph_utils)

# Microbenchmark: random embedding fetch with 1..T threads (does it scale?).
# ML data-access microbenchmark (2026-11-24): per-query p50/p95/p99 for features, sampling, k-hop
add_executable(ml_access_bench ../microbenchmarks/ml_access_bench.cpp)
target_include_directories(ml_access_bench PRIVATE ${LINKPRED_INCLUDE_DIRS})
target_link_libraries(ml_access_bench PUBLIC ${NAME_LIB} graph_utils)

# AdjList neighbour-lookup cost (has_node() guard removal, 2026-11-24)
add_executable(adj_lookup_bench ../microbenchmarks/adj_lookup_bench.cpp)
target_link_libraries(adj_lookup_bench PUBLIC ${NAME_LIB} graph_utils)

add_executable(fetch_random_parallel ../microbenchmarks/fetch_random_parallel.cpp)
target_include_directories(fetch_random_parallel PRIVATE ${LINKPRED_INCLUDE_DIRS})
target_link_libraries(fetch_random_parallel PUBLIC ${NAME_LIB} graph_utils)   # OpenMP flags are global, as for fg_linkpred_bench_db

# Throwaway A/B: block stealing vs contiguous partition (needs a real DB to run).
add_executable(test_partition_compare other/tests/test_partition_compare.cpp)
target_include_directories(test_partition_compare PRIVATE ${LINKPRED_INCLUDE_DIRS})
target_link_libraries(test_partition_compare PUBLIC ${NAME_LIB} graph_utils)

# Simplified runner that only exercises linkpred_reference.h variants.
# Same deps as fg_linkpred_bench_db — opens the DB, preloads embeddings for
# pvector, calls reference functions directly. No threads, no trials, no CSV.
add_executable(fg_linkpred_bench_reference bench/fg_linkpred_bench_reference.cpp)
target_include_directories(fg_linkpred_bench_reference PRIVATE ${LINKPRED_INCLUDE_DIRS})
target_link_libraries(fg_linkpred_bench_reference PUBLIC ${NAME_LIB} graph_utils)

target_sources(fg_linkpred_bench_reference PRIVATE
    common/linkpred_types.h
    common/linkpred_options.h
    common/linkpred_scores.h
    common/linkpred_topk.h
    reference/linkpred_reference.h
    direct/exhaustive/linkpred_embedding_exhaustive.h
)



#NOT PERMANENT and only for perf runs
# Copy run_perf.sh next to the binary so `./run_perf.sh name -- <args>`
# works from the build dir. Lives in runners/ (source) → build/linkpred/ (dest).
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/other/runners/run_perf.sh"
  "${CMAKE_CURRENT_BINARY_DIR}/run_perf.sh"
  COPYONLY
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/other/runners/run_perf_ref.sh"
  "${CMAKE_CURRENT_BINARY_DIR}/run_perf_ref.sh"
  COPYONLY
)
file(CHMOD "${CMAKE_CURRENT_BINARY_DIR}/run_perf_ref.sh"
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                 GROUP_READ GROUP_EXECUTE
                 WORLD_READ WORLD_EXECUTE)

# Microbenchmark runners + output READMEs, copied next to next_vs_cosine and
# fetch_random_parallel so `./run_microbench_sweep.sh` works from the build dir.
# Sources live in ../microbenchmarks.
foreach(f run_next_vs_cosine.sh run_fetch_random_parallel.sh run_microbench_sweep.sh
          next_vs_cosine_output_README.md fetch_random_parallel_output_README.md)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../microbenchmarks/${f}"
                 "${CMAKE_CURRENT_BINARY_DIR}/${f}" COPYONLY)
endforeach()
foreach(f run_next_vs_cosine.sh run_fetch_random_parallel.sh run_microbench_sweep.sh)
  file(CHMOD "${CMAKE_CURRENT_BINARY_DIR}/${f}"
       PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                   GROUP_READ GROUP_EXECUTE
                   WORLD_READ WORLD_EXECUTE)
endforeach()


# FastRP embeddings computed from a Flexograph DB (writes a row-per-node-id .npy
# that test/graphml/add_pytorch_emb_npy loads into node_props).
add_executable(fg_fastrp tools/fastrp.cpp)
target_include_directories(fg_fastrp PRIVATE ${PATH_SRC} ${LINKPRED_INCLUDE_DIRS})
target_link_libraries(fg_fastrp PUBLIC ${NAME_LIB})
