SET(NAME_STD_GRAPH_TEST  "test_standard_graph")
SET(NAME_COMMON_TEST "test_common")

FILE (GLOB SOURCES_TEST_STD_GRAPH  "${PATH_TEST}/test_standard_graph.cpp")
FILE (GLOB SOURCES_TEST_COMMON  "${PATH_TEST}/test_common.cpp")

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories($PATH_SRC)
INCLUDE_DIRECTORIES("${PATH_INCLUDE}")

#Create Test executables
ADD_EXECUTABLE( ${NAME_STD_GRAPH_TEST} "${SOURCES_TEST_STD_GRAPH}")
TARGET_INCLUDE_DIRECTORIES (${NAME_STD_GRAPH_TEST} PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES( ${NAME_STD_GRAPH_TEST} PUBLIC ${NAME_LIB}
                                            gtest
                                            gtest_main)


ADD_EXECUTABLE( ${NAME_COMMON_TEST} "${SOURCES_TEST_COMMON}")
TARGET_INCLUDE_DIRECTORIES (${NAME_COMMON_TEST} PRIVATE ${PATH_SRC})
TARGET_LINK_LIBRARIES( ${NAME_COMMON_TEST} ${NAME_LIB}
                                            gtest
                                            gtest_main)

