FILE(GLOB SOURCES_LIB "${OMP_INCLUDE_DIR}/omp.h"
        "${PATH_SRC}/graph.cpp"
        "${PATH_SRC}/adj_list.cpp"
        "${PATH_SRC}/edgekey_split.cpp"
        "${PATH_SRC}/common_util.cpp"
        "${PATH_SRC}/graph_engine.cpp"
)
#removing headers from the list of sources
#"${PATH_SRC}/iterator.h" "${PATH_SRC}/common_util.h" "${PATH_SRC}/graph.h" "${PATH_SRC}/adj_list.h"
#"${PATH_SRC}/graph_engine.h" "${PATH_SRC}/edgekey_split.h")
#"${PATH_SRC}/graph_exception.h" "${PATH_SRC}/standard_graph.cpp" "${PATH_SRC}/standard_graph.h"
#"${PATH_SRC}/edgekey.cpp" "${PATH_SRC}/edgekey.h"
#
#
# Create GraphAPI.a (static library)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${PATH_SRC} ${OMP_INCLUDE_DIR})
else()
    INCLUDE_DIRECTORIES(${PATH_INCLUDE} ${PATH_SRC} ${PATH_LIBRARY})
endif()

ADD_LIBRARY(${NAME_LIB} STATIC "${SOURCES_LIB}")
TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC ${wt_shared_lib} dl ${OpenMP_omp_LIBRARY} fmt::fmt)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

foreach(dir ${dirs})
    message(STATUS "dir='${dir}'")
endforeach()

# # This adds -ldl to linker list
# TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC dl)
#
# # This adds -lpthread to linker list
# TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC ${CMAKE_THREAD_LIBS_INIT})

# # ######################################################
# # Add install targets for GraphAPI lib and tests     #
# # ######################################################
SET(PATH_INSTALL "${CMAKE_BINARY_DIR}/install")
SET(INSTALL_LIB_DIR "${PATH_INSTALL}/lib")
SET(INSTALL_INCLUDE_DIR "${PATH_INSTALL}/include")

message(STATUS "INSTALL_LIB_DIR: ${INSTALL_LIB_DIR}")
message(STATUS "INSTALL_INCLUDE_DIR: ${INSTALL_INCLUDE_DIR}")

install(TARGETS ${NAME_LIB}
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
)
install(DIRECTORY ${PATH_INCLUDE}/ DESTINATION ${INSTALL_INCLUDE_DIR}
        FILES_MATCHING PATTERN "*.h"
        PATTERN ".git" EXCLUDE
        PATTERN ".DS_Store" EXCLUDE
        PATTERN "CMakeLists.txt" EXCLUDE)
install(DIRECTORY ${PATH_SRC}/ DESTINATION ${INSTALL_INCLUDE_DIR}
        FILES_MATCHING PATTERN "*.h"
        PATTERN ".git" EXCLUDE
        PATTERN ".DS_Store" EXCLUDE
        PATTERN "CMakeLists.txt" EXCLUDE)

#SET(PATH_INSTALL "${}/install")
#SET(INSTALL_LIB_DIR "${PATH_INSTALL}/lib")
#SET(INSTALL_INCLUDE_DIR "${PATH_INSTALL}/include")
#
#message(STATUS "INSTALL_LIB_DIR: ${INSTALL_LIB_DIR}")
#message(STATUS "INSTALL_INCLUDE_DIR: ${INSTALL_INCLUDE_DIR}")
