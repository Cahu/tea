cmake_minimum_required (VERSION 2.6)

project (TEA)

set (CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${CMAKE_SOURCE_DIR}/cmake/
)

include_directories (${TEA_SOURCE_DIR}/inc)
include_directories (${TEA_SOURCE_DIR}/lib/tea)
include_directories (${TEA_SOURCE_DIR}/inc/utils)

find_package (GLM)
find_package (SDL)
find_package (GLEW)
find_package (OpenGL)

set (CMAKE_CXX_FLAGS "-g -Wall -Wextra")

add_subdirectory (src)
add_subdirectory (lib)
