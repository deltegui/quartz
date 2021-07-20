cmake_minimum_required(VERSION 3.17)
project(qcc C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(qcc
        astprint.c
        astprint.h
        common.h
        expr.c
        expr.h
        lexer.c
        lexer.h
        parser.c
        parser.h
        qcc.c
        sysexits.h
        token.c
        token.h)
