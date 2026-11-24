include_directories("${PATH_SRC}" "${PATH_INCLUDE}")
message("WT SO IS ${wt_shared_lib}")

# Create Test executables
# Test standard_graph
#ADD_EXECUTABLE(test_standard_graph "${PATH_TEST}/test_standard_graph.cpp")
#TARGET_INCLUDE_DIRECTORIES(test_standard_graph PRIVATE ${PATH_SRC} ${WT_INSTALL_DIR}/include)
#TARGET_LINK_LIBRARIES(test_standard_graph PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test AdjList
ADD_EXECUTABLE(test_adj_list "${PATH_TEST}/test_adj_list.cpp")
TARGET_INCLUDE_DIRECTORIES(test_adj_list PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_adj_list PUBLIC ${NAME_LIB} ${wt_shared_lib})

## Test Edge Key
#ADD_EXECUTABLE(test_edgekey "${PATH_TEST}/test_edgekey.cpp")
#TARGET_INCLUDE_DIRECTORIES(test_edgekey PRIVATE ${PATH_SRC})
#TARGET_LINK_LIBRARIES(test_edgekey PUBLIC ${NAME_LIB} ${wt_shared_lib})

# ML access correctness tests (sources in test/ml/)
ADD_EXECUTABLE(test_ml_access "${PATH_TEST}/ml/test_ml_access.cpp")
TARGET_INCLUDE_DIRECTORIES(test_ml_access PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_ml_access PUBLIC ${NAME_LIB} ${wt_shared_lib})
ADD_EXECUTABLE(test_ml_edge_access "${PATH_TEST}/ml/test_ml_edge_access.cpp")
TARGET_INCLUDE_DIRECTORIES(test_ml_edge_access PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_ml_edge_access PUBLIC ${NAME_LIB} ${wt_shared_lib})
ADD_EXECUTABLE(test_ml_vector_persistence "${PATH_TEST}/ml/test_ml_vector_persistence.cpp")
TARGET_INCLUDE_DIRECTORIES(test_ml_vector_persistence PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_ml_vector_persistence PUBLIC ${NAME_LIB} ${wt_shared_lib})

ADD_EXECUTABLE(test_fastrp "${PATH_TEST}/ml/test_fastrp.cpp")
TARGET_INCLUDE_DIRECTORIES(test_fastrp PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_fastrp PUBLIC ${NAME_LIB} ${wt_shared_lib})

ADD_EXECUTABLE(test_fastrp_db "${PATH_TEST}/ml/test_fastrp_db.cpp")
TARGET_INCLUDE_DIRECTORIES(test_fastrp_db PRIVATE ${PATH_SRC} ${UTILS})
TARGET_LINK_LIBRARIES(test_fastrp_db PUBLIC ${NAME_LIB} ${wt_shared_lib})

ADD_EXECUTABLE(test_ml_sampling "${PATH_TEST}/ml/test_ml_sampling.cpp")
TARGET_INCLUDE_DIRECTORIES(test_ml_sampling PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_ml_sampling PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test Split EdgeKey
ADD_EXECUTABLE(test_split_ekey "${PATH_TEST}/test_split_edgekey.cpp")
TARGET_INCLUDE_DIRECTORIES(test_split_ekey PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_split_ekey PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test Partitioning
ADD_EXECUTABLE(test_partition "${PATH_TEST}/test_partition.cpp")
TARGET_INCLUDE_DIRECTORIES(test_partition PRIVATE ${PATH_SRC} ${UTILS})
TARGET_LINK_LIBRARIES(test_partition PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test Concurrency
ADD_EXECUTABLE(test_concurrency "${PATH_TEST}/test_concurrency.cpp")
TARGET_INCLUDE_DIRECTORIES(test_concurrency PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_concurrency PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test Cursor (this is a generic testing thing for WT examples to fit our usecase)
ADD_EXECUTABLE(cursor_test "${PATH_TEST}/cursor_test.cpp")
TARGET_INCLUDE_DIRECTORIES(cursor_test PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(cursor_test PUBLIC ${NAME_LIB} ${wt_shared_lib})

# test packing
ADD_EXECUTABLE(test_packing "${PATH_TEST}/test_packing.cpp")
TARGET_INCLUDE_DIRECTORIES(test_packing PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_packing PUBLIC ${NAME_LIB} ${wt_shared_lib})

# # test string
# ADD_EXECUTABLE(test_string_fmt "${PATH_TEST}/test_string_fmt.cpp")
# TARGET_INCLUDE_DIRECTORIES(test_string_fmt PRIVATE ${PATH_SRC})
# TARGET_LINK_LIBRARIES(test_string_fmt PUBLIC ${NAME_LIB} ${wt_shared_lib} fmt::fmt)
#
#Test Std Containers
ADD_EXECUTABLE(std_container_test "${PATH_TEST}/array_test.cpp")
set_source_files_properties( "${PATH_TEST}/array_test.cpp" PROPERTIES COMPILE_FLAGS "-std=c++20 -O3" )

#test bulk_load
ADD_EXECUTABLE(test_bulk_load "${PATH_TEST}/test_bulk_load.cpp")
TARGET_INCLUDE_DIRECTORIES(test_bulk_load PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_bulk_load PUBLIC ${NAME_LIB} ${wt_shared_lib})

#add test_pvector
ADD_EXECUTABLE(test_pvector "${PATH_TEST}/pvector_test.cpp")
INCLUDE_DIRECTORIES(test_pvector PRIVATE ${PATH_INCLUDE} ${UTILS} ${ITTAPI_INCLUDE_DIR} ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_pvector PUBLIC ${NAME_LIB})

# COLUMNAR property storage tests
ADD_EXECUTABLE(test_columnar_ekey "${PATH_TEST}/test_columnar_ekey.cpp")
TARGET_INCLUDE_DIRECTORIES(test_columnar_ekey PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_columnar_ekey PUBLIC ${NAME_LIB} ${wt_shared_lib})

ADD_EXECUTABLE(test_columnar_adj "${PATH_TEST}/test_columnar_adj.cpp")
TARGET_INCLUDE_DIRECTORIES(test_columnar_adj PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_columnar_adj PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Full-schema COLUMNAR tests (all 11 vertex types + new edge types)
ADD_EXECUTABLE(test_columnar_full_ekey "${PATH_TEST}/test_columnar_full_ekey.cpp")
TARGET_INCLUDE_DIRECTORIES(test_columnar_full_ekey PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_columnar_full_ekey PUBLIC ${NAME_LIB} ${wt_shared_lib})

ADD_EXECUTABLE(test_columnar_full_adj "${PATH_TEST}/test_columnar_full_adj.cpp")
TARGET_INCLUDE_DIRECTORIES(test_columnar_full_adj PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_columnar_full_adj PUBLIC ${NAME_LIB} ${wt_shared_lib})

# LDBC SNB query harness
ADD_EXECUTABLE(ldbc_snb_queries "${PATH_TEST}/ldbc_snb_queries.cpp")
TARGET_INCLUDE_DIRECTORIES(ldbc_snb_queries PRIVATE ${PATH_SRC} ${PATH_TEST})
TARGET_LINK_LIBRARIES(ldbc_snb_queries PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Validation binary: outputs query results as JSON lines for cross-system comparison
ADD_EXECUTABLE(validate_queries "${PATH_TEST}/validate_queries.cpp")
TARGET_INCLUDE_DIRECTORIES(validate_queries PRIVATE ${PATH_SRC} ${PATH_TEST})
TARGET_LINK_LIBRARIES(validate_queries PUBLIC ${NAME_LIB} ${wt_shared_lib})

ADD_EXECUTABLE(inspect_graph "${PATH_TEST}/inspect_graph.cpp")
TARGET_INCLUDE_DIRECTORIES(inspect_graph PRIVATE ${PATH_SRC} ${PATH_TEST})
TARGET_LINK_LIBRARIES(inspect_graph PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Typed vertex-ID overflow / B64 sanity check
ADD_EXECUTABLE(test_typed_id "${PATH_TEST}/test_typed_id.cpp")
TARGET_INCLUDE_DIRECTORIES(test_typed_id PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_typed_id PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Error-handling and driver-compliance tests (single-threaded)
ADD_EXECUTABLE(test_error_handling_adj "${PATH_TEST}/test_error_handling_adj.cpp")
TARGET_INCLUDE_DIRECTORIES(test_error_handling_adj PRIVATE ${PATH_SRC} ${PATH_TEST})
TARGET_LINK_LIBRARIES(test_error_handling_adj PUBLIC ${NAME_LIB} ${wt_shared_lib})

ADD_EXECUTABLE(test_error_handling_ekey "${PATH_TEST}/test_error_handling_ekey.cpp")
TARGET_INCLUDE_DIRECTORIES(test_error_handling_ekey PRIVATE ${PATH_SRC} ${PATH_TEST})
TARGET_LINK_LIBRARIES(test_error_handling_ekey PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Multi-threaded correctness tests (thread_local set_key, concurrent inserts)
ADD_EXECUTABLE(test_mt_adj "${PATH_TEST}/test_mt_adj.cpp")
TARGET_INCLUDE_DIRECTORIES(test_mt_adj PRIVATE ${PATH_SRC} ${PATH_TEST})
TARGET_LINK_LIBRARIES(test_mt_adj PUBLIC ${NAME_LIB} ${wt_shared_lib})

ADD_EXECUTABLE(test_mt_ekey "${PATH_TEST}/test_mt_ekey.cpp")
TARGET_INCLUDE_DIRECTORIES(test_mt_ekey PRIVATE ${PATH_SRC} ${PATH_TEST})
TARGET_LINK_LIBRARIES(test_mt_ekey PUBLIC ${NAME_LIB} ${wt_shared_lib})

# IIu schema experiment: key=u, value=IIu with fixed-size blob
ADD_EXECUTABLE(test_IIu_schema "${PATH_TEST}/test_IIu_schema.cpp")
TARGET_INCLUDE_DIRECTORIES(test_IIu_schema PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_IIu_schema PUBLIC ${NAME_LIB} ${wt_shared_lib})
# Copy run_tests.sh to the build directory so it can be run from there
configure_file("${PATH_TEST}/run_tests.sh" "${CMAKE_BINARY_DIR}/run_tests.sh" COPYONLY)
