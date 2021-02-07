cmake_minimum_required(VERSION 2.8)

set(THIS qcclib)

project(${THIS})

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(${THIS} STATIC ${SOURCES} ${HEADERS})