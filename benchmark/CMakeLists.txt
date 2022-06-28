message(CURR_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}")
#file(COPY ${PATH_BUILD}/run_benchmarks.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
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

# ###################################################################################
# #TEST
add_executable(pr_iter_getout pr_iter_getout.cpp)
target_link_libraries(pr_iter_getout PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################