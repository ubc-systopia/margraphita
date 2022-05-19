add_library(graph_utils INTERFACE) 
include_directories($PATH_SRC)
INCLUDE_DIRECTORIES("${PATH_INCLUDE}")
target_include_directories(graph_utils INTERFACE ${PATH_SRC})
