add_executable(pagerank pagerank.cpp)
INCLUDE_DIRECTORIES("${PATH_INCLUDE}" "${UTILS}")

target_include_directories(pagerank PRIVATE ${PATH_SRC})
target_link_libraries(pagerank PUBLIC ${NAME_LIB}
                                        graph_utils)

add_executable(bfs bfs.cpp)
INCLUDE_DIRECTORIES("${PATH_INCLUDE}" "${UTILS}")

target_include_directories(bfs PRIVATE ${PATH_SRC})
target_link_libraries(bfs PUBLIC ${NAME_LIB}
                                        graph_utils)

add_executable(tc tc.cpp)
INCLUDE_DIRECTORIES("${PATH_INCLUDE}" "${UTILS}")

target_include_directories(bfs PRIVATE ${PATH_SRC})
target_link_libraries(tc PUBLIC ${NAME_LIB}
                                        graph_utils)

add_executable(wt_thread wt_thread.cpp)
INCLUDE_DIRECTORIES("${PATH_INCLUDE}" "${UTILS}")

target_include_directories(wt_thread PRIVATE ${PATH_SRC})
target_link_libraries(wt_thread PUBLIC ${NAME_LIB} Threads::Threads graph_utils)
