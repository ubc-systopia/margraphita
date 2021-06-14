cmake_minimum_required(VERSION 3.16)
project(graphapi    VERSION 1.0 
                    DESCRIPTION "C++ GraphAPI for WiredTiger"
                    LANGUAGES CXX)
SET(CMAKE_CXX_FLAGS "-Wall -Wpedantic -g -O0 -std=gnu++2a")
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
#############################################################################
# Initialize the wiredtiger library (submodule)                             #
# This recipe taken and modified from                                       #
# https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html  #
#############################################################################
find_package(Git QUIET)
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/libs/wiredtiger/wt")
  if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    #update the submodules as needed
    option(GIT_SUBMODULE "check submodules during build" ON)
    if(GIT_SUBMODULE)
      message(STATUS "submodule updated")
      execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                      RESULT_VARIABLE GIT_SUBMOD_RESULT)
      if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
      endif()
    endif()
  endif()

  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/libs/wiredtiger/.git")
    message(FATAL_ERROR "The submodule was not downloaded. Init the git submodule manually")
  endif()

  execute_process(COMMAND ./autogen.sh
      RESULT_VARIABLE result
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/wiredtiger )
  if(result)
    message(FATAL_ERROR "Running autogen failed for Wiredtiger submodule with error: ${result}")
  endif()

  execute_process(COMMAND ./configure
      RESULT_VARIABLE result
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/wiredtiger)
  if(result)
    message(FATAL_ERROR "Running autoconf failed for Wiredtiger submodule with error: ${result}")
  endif()

  execute_process(COMMAND make -j16
      RESULT_VARIABLE result
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/wiredtiger)
  if(result)
    message(FATAL_ERROR "Running make failed for Wiredtiger submodule with error: ${result}")
  endif()
endif()
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
SET(BENCHMARK   "${PROJECT_SOURCE_DIR}/benchmark")
SET(UTILS       "${PROJECT_SOURCE_DIR}/utils")

#Create GraphAPI static library
add_subdirectory(src)

#Create Test executables
add_subdirectory(test)

#Compile utilities
add_subdirectory(utils)

#Compile all benchmarks
add_subdirectory(benchmark)
