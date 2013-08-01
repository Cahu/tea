cmake_minimum_required (VERSION 2.6)

project (TEA) 

find_package (SDL)
find_package (OpenGL)

set (CMAKE_CXX_FLAGS "-g -Wall")

add_subdirectory (src)
add_subdirectory (lib)
