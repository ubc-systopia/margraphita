message(CURR_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}")

# file(COPY ${PATH_BUILD}/run_benchmarks.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(CREATE_LINK ${PATH_BUILD}/run_benchmarks.sh ${CMAKE_CURRENT_BINARY_DIR}/run_benchmarks.sh
    COPY_ON_ERROR SYMBOLIC)

INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${UTILS} ${PATH_SRC})

# ###################################################################################
add_executable(pagerank pagerank.cpp)
target_link_libraries(pagerank PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(pr_iter_map pagerank_iter_map.cpp)
target_link_libraries(pr_iter_map PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(bfs bfs.cpp)
target_link_libraries(bfs PUBLIC ${NAME_LIB} graph_utils)

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

# add_executable(bc bc.cpp)
# target_link_libraries(bc PUBLIC ${NAME_LIB} graph_utils)

# add_executable(bc_parallel bc_parallel.cpp)
# target_link_libraries(bc_parallel PUBLIC ${NAME_LIB} graph_utils)

# add_executable(tc_iter_parallel tc_iter_parallel.cpp)
# target_link_libraries(tc_iter_parallel PUBLIC ${NAME_LIB} graph_utils)

# add_executable(bfs_parallel bfs_parallel.cpp)
# target_link_libraries(bfs_parallel PUBLIC ${NAME_LIB} graph_utils)