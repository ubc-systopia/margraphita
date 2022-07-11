FILE(GLOB SOURCES_LIB "${PATH_SRC}/common.cpp"
    "${PATH_SRC}/common.h"
    "${PATH_SRC}/graph.h"
    "${PATH_SRC}/graph.cpp"
    "${PATH_SRC}/graph_engine.h"
    "${PATH_SRC}/graph_engine.cpp"
    "${PATH_SRC}/lock.h"
    "${PATH_SRC}/lock.cpp"
    "${PATH_SRC}/graph_exception.cpp"
    "${PATH_SRC}/standard_graph.cpp"
    "${PATH_SRC}/standard_graph.h"
    "${PATH_SRC}/adj_list.cpp"
    "${PATH_SRC}/adj_list.h"
    "${PATH_SRC}/edgekey.cpp"
    "${PATH_SRC}/edgekey.h")

# Create GraphAPI.a (static library)
INCLUDE_DIRECTORIES("${PATH_INCLUDE}")
ADD_LIBRARY(${NAME_LIB} STATIC "${SOURCES_LIB}")
TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC ${WIREDTIGER_LIB}/libwiredtiger.so)

# This adds -ldl to linker list
TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC dl)

# This adds -lpthread to linker list
TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
