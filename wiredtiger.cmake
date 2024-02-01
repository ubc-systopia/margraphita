# check if debug flag is set and if so build wiredtiger in debug mode
MESSAGE(STATUS "from wiredtiger.cmake: CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

# if(CMAKE_BUILD_TYPE MATCHES Debug)
# SET(WT_CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Debug")
# SET(PREFIX_DIR "${PROJECT_SOURCE_DIR}/libs/wiredtiger/wt_debug")
# else()
# set(WT_CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release")
# SET(PREFIX_DIR "${PROJECT_SOURCE_DIR}/libs/wiredtiger/wt_release")
# endif()
SET(PREFIX_DIR "${PROJECT_SOURCE_DIR}/libs/wiredtiger/wt_release")

MESSAGE(STATUS "from wiredtiger.cmake: PREFIX_DIR is ${PREFIX_DIR}")

set(WT_DOWNLOAD_DIR "${PREFIX_DIR}/wiredtiger-src")
set(WT_LOG_DIR "${PREFIX_DIR}/wiredtiger-log")
set(WT_STAMP_DIR "${PREFIX_DIR}/wiredtiger-stamp")
set(WT_TMP_DIR "${PREFIX_DIR}/wiredtiger-tmp")
set(WT_SOURCE_DIR "${PREFIX_DIR}/wiredtiger-src")
set(WT_INSTALL_DIR "${PREFIX_DIR}/wiredtiger-install")
set(WT_BINARY_DIR "${PREFIX_DIR}/wiredtiger-build")

set(WT_BINARY "${WT_BINARY_DIR}/wt")
message(STATUS "WiredTiger binary is ${WT_BINARY}")
if(NOT EXISTS ${WT_BINARY})
  MESSAGE(STATUS "WiredTiger binary ${WT_BINARY} not found, downloading...")
  include(ExternalProject)
  ExternalProject_Add(wiredtiger
    BUILD_ALWAYS false
    GIT_REPOSITORY https://github.com/wiredtiger/wiredtiger
    GIT_TAG 11.0.0
    PREFIX "${PREFIX_DIR}"
    DOWNLOAD_DIR "${WT_DOWNLOAD_DIR}"
    LOG_DIR "${WT_DOWNLOAD_DIR}"
    STAMP_DIR "${WT_STAMP_DIR}"
    TMP_DIR "${WT_TMP_DIR}"
    SOURCE_DIR "${WT_SOURCE_DIR}"
    INSTALL_DIR "${WT_INSTALL_DIR}"
    BINARY_DIR "${WT_BINARY_DIR}"

    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${WT_INSTALL_DIR}
    -DCMAKE_INSTALL_LIBDIR:PATH=lib
    -DCMAKE_INSTALL_RPATH:PATH=<INSTALL_DIR>/lib
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
  )
endif()

SET(WIREDTIGER_LIB "${WT_INSTALL_DIR}")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(wt_shared_lib ${WIREDTIGER_LIB}/lib/libwiredtiger.11.0.0.dylib)
else()
  set(wt_shared_lib ${WIREDTIGER_LIB}/lib/libwiredtiger.so.11.0.0)
endif()
