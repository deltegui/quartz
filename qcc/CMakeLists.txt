cmake_minimum_required(VERSION 2.8)

project(quartz_compiler)

set(CMAKE_CXX_STANDARD 11)

get_filename_component(qcclib_SRC src ABSOLUTE)
message(${qcclib_SRC})

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(bin)