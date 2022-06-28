configure_file(${CMAKE_CURRENT_SOURCE_DIR}/preprocess.sh ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/insert_kron.sh ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/kron_gen.sh ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${UTILS} ${PATH_SRC})

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

# ###################################################################################
add_executable(init_db init_db.cpp)
target_include_directories(init_db PRIVATE ${PATH_SRC})
target_link_libraries(init_db PUBLIC ${NAME_LIB} graph_utils)