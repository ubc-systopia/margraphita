INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${UTILS} ${PATH_SRC})

add_executable(fg_aster_bench fg_aster_bench.cpp)
target_link_libraries(fg_aster_bench PUBLIC ${NAME_LIB} graph_utils)

add_executable(fg_cdlp fg_cdlp.cpp)
target_link_libraries(fg_cdlp PUBLIC ${NAME_LIB} graph_utils)
