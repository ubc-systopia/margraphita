configure_file(${CMAKE_CURRENT_SOURCE_DIR}/preprocess.py ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/insert_kron.py ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/graph_datasets.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/paths.py ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/paths.py ${CMAKE_CURRENT_BINARY_DIR}/../benchmark COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/nodes.awk ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/count.awk ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_sort.awk ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/kron_gen.py ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${UTILS} ${PATH_SRC} ${Boost_INCLUDE_DIRS})

#add_executable(bulk_insert bulk_insert.cpp reader.cpp)
#target_include_directories(bulk_insert PRIVATE ${PMAP})
#target_link_libraries(bulk_insert PUBLIC ${Boost_LIBRARIES} ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(bulk_insert_low_mem bulk_insert_low_mem.cpp reader.h
        bulk_insert_low_mem.h)
# target_include_directories(bulk_insert_low_mem PRIVATE)
target_link_libraries(bulk_insert_low_mem PUBLIC ${NAME_LIB} graph_utils TBB::tbb)
#
## ###################################################################################
add_executable(dense_vertexranges dense_vertexranges_set.cpp)
target_include_directories(dense_vertexranges PRIVATE ${PMAP})
target_link_libraries(dense_vertexranges PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
# add_executable(single_threaded_graphapi_insert single_threaded_graphapi_insert.cpp)
# target_include_directories(single_threaded_graphapi_insert PRIVATE ${PATH_SRC})
# target_link_libraries(single_threaded_graphapi_insert PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
#add_executable(mt_graphapi_insert mt_graphapi_insert.cpp)
#target_include_directories(mt_graphapi_insert PRIVATE ${PATH_SRC})
#target_link_libraries(mt_graphapi_insert PUBLIC ${Boost_LIBRARIES} ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(init_db init_db.cpp)
target_include_directories(init_db PRIVATE ${PATH_SRC})
target_link_libraries(init_db PUBLIC ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(mk_adjlists mk_adjlists.cpp reader.h mk_adjlists.h)
target_include_directories(mk_adjlists PRIVATE ${PATH_SRC})
target_link_libraries(mk_adjlists PUBLIC ${Boost_LIBRARIES} ${NAME_LIB} graph_utils)

# ###################################################################################
add_executable(dump_graph dump_graph.cpp reader.h)
target_include_directories(dump_graph PRIVATE ${PATH_SRC})
target_link_libraries(dump_graph PUBLIC ${Boost_LIBRARIES} ${NAME_LIB} graph_utils)

#add_executable(test_tbb test_tbbb.cpp)
#target_link_libraries(test_tbb PUBLIC TBB::tbb)