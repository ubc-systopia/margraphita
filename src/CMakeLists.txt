FILE(GLOB SOURCES_LIB "${OMP_INCLUDE_DIR}/omp.h"
    "${PATH_SRC}/common.cpp"
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
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    INCLUDE_DIRECTORIES("${PATH_INCLUDE}" "${OMP_INCLUDE_DIR}")
else()
    INCLUDE_DIRECTORIES("${PATH_INCLUDE}")
endif()
ADD_LIBRARY(${NAME_LIB} STATIC "${SOURCES_LIB}" )
TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC ${wt_shared_lib} dl ${OpenMP_omp_LIBRARY})

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

## This adds -ldl to linker list
#TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC dl)
#
## This adds -lpthread to linker list
#TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
