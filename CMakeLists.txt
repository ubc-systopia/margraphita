cmake_minimum_required(VERSION 3.16)
project(graphapi VERSION 1.0
  DESCRIPTION "C++ GraphAPI for WiredTiger"
  LANGUAGES CXX)

include(FetchContent)
message("CXX compiler version ${CMAKE_CXX_COMPILER_VERSION}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  MESSAGE("Building on MacOSX")
  add_compile_definitions(MACOSX)
  SET(CMAKE_CXX_FLAGS "-Wall -Wpedantic -g")
  SET(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
  SET(OMP_INCLUDE_DIR "/usr/local/opt/libomp/include")
  SET(OpenMP_CXX_LIB_NAMES "omp")
  SET(OpenMP_omp_LIBRARY /usr/local/opt/libomp/lib/libomp.dylib)

  SET(LIB_THREADS omp)

else()
  MESSAGE("Building on Linux")
  add_compile_definitions(LINUX)
  SET(CMAKE_CXX_FLAGS "-Wall -Wpedantic -fopenmp -g")
  find_package(Threads) # To include pthreads
  SET(LIB_THREADS Threads::Threads)
endif()

set(CMAKE_CXX_STANDARD 20)

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
  MESSAGE("Generating a debug build")
  MESSAGE("Generating a debug build for WiredTiger")
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build_debug/wt")
    MESSAGE("${PROJECT_SOURCE_DIR}/libs/wiredtiger/build_debug/wt")
    include(ExternalProject)
    ExternalProject_Add(wiredtiger
        BUILD_ALWAYS false
        GIT_REPOSITORY https://github.com/wiredtiger/wiredtiger
        GIT_TAG 11.0.0
        DOWNLOAD_DIR "${PROJECT_SOURCE_DIR}/libs/wiredtiger"
        BINARY_DIR "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build_debug"
        CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Debug"
        INSTALL_COMMAND ""
        TEST_COMMAND ""
    )
  endif()

  SET(WIREDTIGER_LIB "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build_debug")

  if(stat)
    MESSAGE("Enabling statistics")
    add_compile_definitions(STAT)
  endif()

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -g -fno-inline-functions -fno-omit-frame-pointer")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")

  # IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  #   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
  #   SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
  #   SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
  # ENDIF()
ELSE()
  MESSAGE("Generating a release build (-O3)")
  MESSAGE("Generating a Release build for WiredTiger")

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

  SET(WIREDTIGER_LIB "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(wt_shared_lib ${WIREDTIGER_LIB}/libwiredtiger.11.0.0.dylib)
else()
  set(wt_shared_lib ${WIREDTIGER_LIB}/libwiredtiger.so.11.0.0)
endif()

find_package(Git QUIET)

## ######################################################
## Include fmtlib and Boost Serialization              #
## ######################################################
#FetchContent_Declare(fmt
#        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
#        GIT_TAG master
#        )
#FetchContent_MakeAvailable(fmt)
#
find_package(Boost COMPONENTS serialization REQUIRED)

if(Boost_FOUND)
 message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
 message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
 message(STATUS "Boost_VERSION: ${Boost_VERSION}")

 include_directories(${Boost_INCLUDE_DIRS})
endif ()

# ######################################################
# Define paths and targets for GraphAPI lib and tests #
# ######################################################
SET(NAME_LIB "GraphAPI")

SET(PATH_BUILD "${PROJECT_SOURCE_DIR}/build")
SET(PATH_INCLUDE "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build"
  "${PROJECT_SOURCE_DIR}/libs/wiredtiger/build/include")
SET(PATH_SRC "${PROJECT_SOURCE_DIR}/src")
SET(PATH_TEST "${PROJECT_SOURCE_DIR}/test")
SET(BENCHMARK "${PROJECT_SOURCE_DIR}/benchmark")
SET(UTILS "${PROJECT_SOURCE_DIR}/utils")
SET(PMAP "${UTILS}/parallel_hashmap")

message("WT SO IS ${wt_shared_lib}")

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
configure_file(${PROJECT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR}/preprocess COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR}/benchmark COPYONLY)
