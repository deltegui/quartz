cmake_minimum_required(VERSION 2.8)

set(THIS qcc)

project(${THIS})

include_directories(${qcclib_SRC})

add_executable(${THIS} qcc.cc)

target_link_libraries(${THIS} qcclib)