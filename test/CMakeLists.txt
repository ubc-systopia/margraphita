include_directories("${PATH_SRC}" "${PATH_INCLUDE}")
message("WT SO IS ${wt_shared_lib}")

# Create Test executables
# Test standard_graph
ADD_EXECUTABLE(test_standard_graph "${PATH_TEST}/test_standard_graph.cpp")
TARGET_INCLUDE_DIRECTORIES(test_standard_graph PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_standard_graph PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test AdjList
ADD_EXECUTABLE(test_adj_list "${PATH_TEST}/test_adj_list.cpp")
TARGET_INCLUDE_DIRECTORIES(test_adj_list PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_adj_list PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test Edge Key
ADD_EXECUTABLE(test_edgekey "${PATH_TEST}/test_edgekey.cpp")
TARGET_INCLUDE_DIRECTORIES(test_edgekey PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_edgekey PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test UNordered EdgeLog
ADD_EXECUTABLE(test_elist "${PATH_TEST}/test_uoedgelist.cpp")
TARGET_INCLUDE_DIRECTORIES(test_elist PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_elist PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test Partitioning
ADD_EXECUTABLE(test_partition "${PATH_TEST}/test_partition.cpp")
TARGET_INCLUDE_DIRECTORIES(test_partition PRIVATE ${PATH_SRC} ${UTILS})
TARGET_LINK_LIBRARIES(test_partition PUBLIC ${NAME_LIB} ${wt_shared_lib})

# Test Concurrency
ADD_EXECUTABLE(test_concurrency "${PATH_TEST}/test_concurrency.cpp")
TARGET_INCLUDE_DIRECTORIES(test_concurrency PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(test_concurrency PUBLIC ${NAME_LIB} ${wt_shared_lib})

#Test Cursor (this is a generic testing thing for WT examples to fit our usecase)
ADD_EXECUTABLE(cursor_test "${PATH_TEST}/cursor_test.cpp")
TARGET_INCLUDE_DIRECTORIES(cursor_test PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES(cursor_test PUBLIC ${NAME_LIB} ${wt_shared_lib})