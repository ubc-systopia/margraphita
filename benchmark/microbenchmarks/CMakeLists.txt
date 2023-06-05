file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run_ubenchmark.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${UTILS} ${PATH_SRC})

# ###################################################################################
add_executable(csr_seek_scan csr_seek_scan.cpp)
target_link_libraries(csr_seek_scan PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(adjlist_seek_scan adjlist_seek_scan.cpp)
target_link_libraries(adjlist_seek_scan PUBLIC ${NAME_LIB} graph_utils)

add_executable(adjlist_iter_seek_scan adjlist_iter_seek_scan.cpp adjlist_iter_seek_scan.cpp)
target_link_libraries(adjlist_iter_seek_scan PUBLIC ${NAME_LIB} graph_utils)
