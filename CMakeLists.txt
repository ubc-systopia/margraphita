cmake_minimum_required(VERSION 3.16)
project(graphapi VERSION 1.0
  DESCRIPTION "C++ GraphAPI for WiredTiger"
  LANGUAGES CXX)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  MESSAGE("Building on MacOSX")
  add_compile_definitions(MACOSX)
else()
  MESSAGE("Building on Linux")
  add_compile_definitions(LINUX)
endif()

set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "-Wall -Wpedantic -fopenmp -g")

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
  MESSAGE("Generating a debug build")

  if(stat)
    MESSAGE("Enabling statistics")
    add_compile_definitions(STAT)
  endif()

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fno-inline-functions -fno-omit-frame-pointer")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")

  IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
  ENDIF()
ELSE()
  MESSAGE("Generating a release build (-O3)")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

find_package(Threads) # To include pthreads
find_package(Git QUIET)

MESSAGE("${PROJECT_SOURCE_DIR}/libs/wiredtiger/build/wt")
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build/wt")
    MESSAGE("${PROJECT_SOURCE_DIR}/libs/wiredtiger/build/wt")
    include(ExternalProject)
    ExternalProject_Add(wiredtiger
        BUILD_ALWAYS false
        GIT_REPOSITORY https://github.com/wiredtiger/wiredtiger
        GIT_TAG 11.0.0
        DOWNLOAD_DIR "${PROJECT_SOURCE_DIR}/libs/wiredtiger"
        BINARY_DIR "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build"
        INSTALL_COMMAND ""
        TEST_COMMAND ""
    )
endif()

# ######################################################
# Define paths and targets for GraphAPI lib and tests #
# ######################################################
SET(NAME_LIB "GraphAPI")

SET(WIREDTIGER_LIB "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build")
SET(PATH_BUILD "${PROJECT_SOURCE_DIR}/build")
SET(PATH_INCLUDE "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build"
  "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build/include")
SET(PATH_SRC "${PROJECT_SOURCE_DIR}/src")
SET(PATH_TEST "${PROJECT_SOURCE_DIR}/test")
SET(BENCHMARK "${PROJECT_SOURCE_DIR}/benchmark")
SET(UTILS "${PROJECT_SOURCE_DIR}/utils")
SET(PMAP "${UTILS}/parallel_hashmap")

# Create GraphAPI static library
add_subdirectory(src)

# Compile utilities
add_subdirectory(utils)

# Create Test executables
add_subdirectory(test)

# Benchmarks executables
add_subdirectory(benchmark)

# Preprocess executables
add_subdirectory(preprocess)

configure_file(${PROJECT_SOURCE_DIR}/.githooks/pre-commit ${PROJECT_SOURCE_DIR}/.git/hooks COPYONLY)
