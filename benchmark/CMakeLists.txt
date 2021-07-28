#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/preprocess.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/preprocess.sh ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

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

add_executable(bulk_insert bulk_insert.cpp)
INCLUDE_DIRECTORIES("${PATH_INCLUDE}" "${UTILS}")

target_include_directories(bulk_insert PRIVATE ${PATH_SRC})
target_link_libraries(bulk_insert PUBLIC ${NAME_LIB} Threads::Threads graph_utils)
