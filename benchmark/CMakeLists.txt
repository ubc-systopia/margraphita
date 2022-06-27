# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/preprocess.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/preprocess.sh ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/insert_kron.sh ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_benchmarks.sh ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/kron_gen.sh ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

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
add_executable(bulk_insert bulk_insert.cpp reader.cpp)
target_include_directories(bulk_insert PRIVATE ${PMAP})
target_link_libraries(bulk_insert PUBLIC ${NAME_LIB} Threads::Threads graph_utils)

# ###################################################################################
add_executable(dense_vertexranges dense_vertexranges.cpp)
target_include_directories(dense_vertexranges PRIVATE ${PMAP})
target_link_libraries(dense_vertexranges PUBLIC ${NAME_LIB} Threads::Threads graph_utils)

# ###################################################################################
add_executable(single_threaded_graphapi_insert single_threaded_graphapi_insert.cpp)
target_include_directories(single_threaded_graphapi_insert PRIVATE ${PATH_SRC})
target_link_libraries(single_threaded_graphapi_insert PUBLIC ${NAME_LIB} graph_utils)