message(CURR_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run_benchmarks.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${UTILS} ${PATH_SRC})

# Microbenchmark For Insertion
ADD_EXECUTABLE(insertion_mb insertion_microbenchmark.cpp)
TARGET_LINK_LIBRARIES(insertion_mb PUBLIC ${NAME_LIB} ${wt_shared_lib} graph_utils)

# Microbenchmark For Hashing
ADD_EXECUTABLE(hashing_mb hashing_microbenchmark.cpp)
TARGET_LINK_LIBRARIES(hashing_mb PUBLIC ${NAME_LIB} ${wt_shared_lib} graph_utils)

# Insertion/deletion benchmarks
ADD_EXECUTABLE(insert_delete insertion_deletion.cpp)
TARGET_LINK_LIBRARIES(insert_delete PUBLIC ${NAME_LIB} ${wt_shared_lib} graph_utils)


# ###################################################################################
add_executable(pagerank pagerank.cpp)
target_link_libraries(pagerank PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(pr_iter_map pagerank_iter_map.cpp)
target_link_libraries(pr_iter_map PUBLIC ${NAME_LIB} graph_utils)

add_executable(pr_ec pagerank_ec.cpp)
target_link_libraries(pr_ec PUBLIC ${NAME_LIB} graph_utils)

add_executable(pr_vc pagerank_vc.cpp)
target_link_libraries(pr_vc PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(bfs bfs.cpp)
target_link_libraries(bfs PUBLIC ${NAME_LIB} graph_utils)

###################################################################################
add_executable(bfs_parallel_ec bfs_parallel_ec.cpp)
target_link_libraries(bfs_parallel_ec PUBLIC ${NAME_LIB} graph_utils)

add_executable(bfs_parallel bfs_parallel.cpp)
target_link_libraries(bfs_parallel PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(tc tc.cpp)
target_link_libraries(tc PUBLIC ${NAME_LIB} graph_utils)

add_executable(tc_iter tc_iter.cpp)
target_link_libraries(tc_iter PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
# #TEST
add_executable(pr_iter_getout pr_iter_getout.cpp)
target_link_libraries(pr_iter_getout PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################

# add_executable(pr_iter_parallel pr_iter_parallel.cpp)
# target_link_libraries(pr_iter_parallel PUBLIC ${NAME_LIB} graph_utils)
add_executable(cc cc.cpp)
target_link_libraries(cc PUBLIC ${NAME_LIB} graph_utils)

add_executable(cc_parallel cc_parallel.cpp)
target_link_libraries(cc_parallel PUBLIC ${NAME_LIB} graph_utils)

add_executable(cc_parallel_ec cc_parallel_ec.cpp)
target_link_libraries(cc_parallel_ec PUBLIC ${NAME_LIB} graph_utils)

add_executable(sssp sssp.cpp)
target_link_libraries(sssp PUBLIC ${NAME_LIB} graph_utils)

add_executable(iteration_vc iteration_vc.cpp)
target_link_libraries(iteration_vc PUBLIC ${NAME_LIB} graph_utils)

add_executable(iteration_ec iteration_ec.cpp)
target_link_libraries(iteration_ec PUBLIC ${NAME_LIB} graph_utils)

add_executable(adj_ekey_microbenchmark adj_ekey_microbenchmark.cpp)
target_link_libraries(adj_ekey_microbenchmark PUBLIC ${NAME_LIB} graph_utils)

# add_executable(bc bc.cpp)
# target_link_libraries(bc PUBLIC ${NAME_LIB} graph_utils)

# add_executable(bc_parallel bc_parallel.cpp)
# target_link_libraries(bc_parallel PUBLIC ${NAME_LIB} graph_utils)

# add_executable(tc_iter_parallel tc_iter_parallel.cpp)
# target_link_libraries(tc_iter_parallel PUBLIC ${NAME_LIB} graph_utils)

# add_executable(bfs_parallel bfs_parallel.cpp)
# target_link_libraries(bfs_parallel PUBLIC ${NAME_LIB} graph_utils)
