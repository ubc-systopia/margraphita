INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${UTILS} ${PATH_SRC})

add_executable(fg_structural_bench fg_structural_bench.cpp)
target_link_libraries(fg_structural_bench PUBLIC ${NAME_LIB} graph_utils)

add_executable(fg_cdlp fg_cdlp.cpp)
target_link_libraries(fg_cdlp PUBLIC ${NAME_LIB} graph_utils)

add_executable(fg_property_bench fg_property_bench.cpp)
target_link_libraries(fg_property_bench PUBLIC ${NAME_LIB} graph_utils)

add_executable(fg_bfs_light fg_bfs_light.cpp)
target_link_libraries(fg_bfs_light PUBLIC ${NAME_LIB} graph_utils)

add_executable(fg_sp_light fg_sp_light.cpp)
target_link_libraries(fg_sp_light PUBLIC ${NAME_LIB} graph_utils)
