INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${UTILS} ${PATH_SRC})

# ###################################################################################
add_executable(csr_seek_scan csr_seek_scan.cpp)
target_link_libraries(csr_seek_scan PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(adjlist_seek_scan adjlist_seek_scan.cpp)
target_link_libraries(adjlist_seek_scan PUBLIC ${NAME_LIB} graph_utils)
