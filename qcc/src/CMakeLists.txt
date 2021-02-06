cmake_minimum_required(VERSION 2.8)

set(THIS qcclib)

project(${THIS})

set(SOURCES
    lexer.cc
)

set(HEADERS
    lexer.h
)

add_library(${THIS} STATIC ${SOURCES} ${HEADERS})