cmake_minimum_required(VERSION 2.8)

set(THIS qcc_test)

project(THIS)

enable_testing()

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cc ${CMAKE_CURRENT_SOURCE_DIR}/../*.cc)
list(FILTER SOURCES EXCLUDE REGEX "qcc.cc")

add_subdirectory(gtest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ${qcclib_SRC})

add_executable(${THIS} ${SOURCES})

target_link_libraries(${THIS} gtest)

add_test(NAME ${THIS} COMMAND ${THIS})