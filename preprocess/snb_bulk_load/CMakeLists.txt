# snb_bulk_load — multithreaded, RAM-efficient LDBC SNB full-schema bulk loader

add_executable(snb_bulk_load
    snb_bulk_load.cpp
    snb_id_map.h
    snb_prop_spool.h
)

target_include_directories(snb_bulk_load PRIVATE
    ${PATH_SRC}
    ${PATH_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(snb_bulk_load PUBLIC
    ${NAME_LIB}
    ${wt_shared_lib}
    graph_utils
)
