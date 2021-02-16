cmake_minimum_required(VERSION 3.16)
project(graphapi    VERSION 1.0 
                    DESCRIPTION "C++ GraphAPI for WiredTiger"
                    LANGUAGES CXX)
SET(CMAKE_CXX_FLAGS "-Wall -Wpedantic -g -O0 -std=gnu++17")
find_package (Threads) #To include pthreads
####################################################
# Download and unpack googletest at configure time #
####################################################
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

#######################################################
# Define paths and targets for GraphAPI lib and tests #
#######################################################

SET(NAME_LIB   "GraphAPI")

SET(WIREDTIGER_STATIC_LIB "${PROJECT_SOURCE_DIR}/libs/wiredtiger")
SET(PATH_BUILD "${PROJECT_SOURCE_DIR/build}")
SET(PATH_INCLUDE "${PROJECT_SOURCE_DIR}/libs/wiredtiger"
                 "${PROJECT_SOURCE_DIR}/libs/wiredtiger/src/include" )
SET(PATH_SRC    "${PROJECT_SOURCE_DIR}/src")
SET(PATH_TEST   "${PROJECT_SOURCE_DIR}/test")

#Create GraphAPI static library
add_subdirectory(src)

#Create Test executables
add_subdirectory(test)
