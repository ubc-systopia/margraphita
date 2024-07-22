cmake_minimum_required(VERSION 3.20)
project(graphapi VERSION 1.0
  DESCRIPTION "C++ GraphAPI for WiredTiger"
  LANGUAGES CXX
)
include(FetchContent)

# ################################################################################
# Compiler Information and flags
# ################################################################################
message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_C_COMPILER_ID: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# ################################################################################
# Check if we are on a Mac or Linux and set build type and CPP standard
# ###############################################################################
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  MESSAGE(STATUS "Building on MacOSX")
  add_compile_definitions(MACOSX)
  SET(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -g")
  SET(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
  SET(OMP_INCLUDE_DIR "/usr/local/opt/libomp/include")
  SET(OpenMP_CXX_LIB_NAMES "omp")
  SET(OpenMP_omp_LIBRARY /usr/local/opt/libomp/lib/libomp.dylib)
  SET(LIB_THREADS omp)

else()
  MESSAGE(STATUS "Building on Linux")
  add_compile_definitions(LINUX)

  # SET(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -g")
  # find_package(Threads)
  # SET(LIB_THREADS Threads::Threads)
endif()

set(CMAKE_CXX_STANDARD 20)

# ################################################################################
# Include WiredTiger
# ################################################################################
include(wiredtiger.cmake)

IF (CMAKE_BUILD_TYPE MATCHES Debug)
  MESSAGE(STATUS "Generating a debug build for the project")
  add_compile_definitions(DEBUG)
  SET(ITTAPI_LIB "/usr/local/lib/libittnotify.a")
  SET(ITTAPI_INCLUDE_DIR "/usr/local/include")
  if(stat)
    MESSAGE(STATUS "Enabling statistics on WT")
    add_compile_definitions(STAT)
  endif()

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Og -g -fno-inline-functions -fno-omit-frame-pointer  -fopenmp")
ELSE()
  MESSAGE("Generating a release build (-O3)")

  SET(CMAKE_BUILD_TYPE "Release")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -O3 -g -fopenmp")
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

MESSAGE(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

find_package(Git QUIET)

# # ######################################################
# # Include fmtlib and Boost Serialization              #
# # ######################################################
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 10.1.1
)
FetchContent_MakeAvailable(fmt)

#find_package(fmt)
find_package(Boost COMPONENTS serialization REQUIRED)


# find_package(Boost)
if(Boost_FOUND)
  message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
  message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
  message(STATUS "Boost_VERSION: ${Boost_VERSION}")

  include_directories(${Boost_INCLUDE_DIRS})
endif()

# ######################################################
# Define paths and targets for GraphAPI lib and tests #
# ######################################################
SET(NAME_LIB "GraphAPI")

if (B64)
  message(STATUS "Using 64 bit vertex ids")
else ()
  message(STATUS "Using 32 bit vertex ids")
endif ()


SET(PATH_BUILD "${PROJECT_SOURCE_DIR}/build")
SET(PATH_INCLUDE "${WT_INSTALL_DIR}/include")
SET(PATH_LIBRARY "${WT_INSTALL_DIR}/lib")
SET(PATH_SRC "${PROJECT_SOURCE_DIR}/src")
SET(PATH_TEST "${PROJECT_SOURCE_DIR}/test")
SET(BENCHMARK "${PROJECT_SOURCE_DIR}/benchmark")
SET(UTILS "${PROJECT_SOURCE_DIR}/utils")
SET(PMAP "${UTILS}/parallel_hashmap")

message(STATUS "WT SO IS ${wt_shared_lib}")

# Create GraphAPI static library
add_subdirectory(src)

# Compile utilities
add_subdirectory(utils)

# Create Test executables
add_subdirectory(test)

## Benchmarks executables
add_subdirectory(benchmark)

## Preprocess executables
add_subdirectory(preprocess)

#configure_file(${PROJECT_SOURCE_DIR}/.githooks/pre-commit ${PROJECT_SOURCE_DIR}/.git/hooks COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR}/preprocess COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR}/benchmark COPYONLY)
