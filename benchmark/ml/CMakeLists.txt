add_executable(fg_ml_runner fg_ml_runner.cpp)
target_include_directories(fg_ml_runner PRIVATE
    ${PATH_SRC} ${UTILS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../linkpred/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../linkpred/pipeline-generators
    ${CMAKE_CURRENT_SOURCE_DIR}/../linkpred/direct/fof
    ${CMAKE_CURRENT_SOURCE_DIR}/../linkpred/direct/khop
    ${CMAKE_CURRENT_SOURCE_DIR}/../linkpred/direct/exhaustive)
target_link_libraries(fg_ml_runner PUBLIC ${NAME_LIB} graph_utils)
