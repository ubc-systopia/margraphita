find_package(OpenMP REQUIRED)

add_executable(fg_ldbc_bench fg_ldbc_bench.cpp)

target_include_directories(fg_ldbc_bench PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(fg_ldbc_bench PRIVATE
    ${NAME_LIB}
    ${wt_shared_lib}
    OpenMP::OpenMP_CXX)
