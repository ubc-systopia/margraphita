FILE ( GLOB SOURCES_LIB   "${PATH_SRC}/common.cpp"
                    "${PATH_SRC}/graph_exception.cpp"
                    "${PATH_SRC}/standard_graph.cpp")



#Create GraphAPI.a (static library)

INCLUDE_DIRECTORIES("${PATH_INCLUDE}")
ADD_LIBRARY( ${NAME_LIB}  STATIC  "${SOURCES_LIB}")
set_target_properties(${NAME_LIB} PROPERTIES COMPILE_FLAGS "-Wall -Wpedantic -g -O0 -std=gnu++17")
TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC ${WIREDTIGER_STATIC_LIB}/.libs/libwiredtiger.a)
#This adds -ldl to linker list
TARGET_LINK_LIBRARIES(${NAME_LIB} PUBLIC dl) 
#This adds -lpthread to linker list
TARGET_LINK_LIBRARIES (${NAME_LIB} PUBLIC ${CMAKE_THREAD_LIBS_INIT}) 
