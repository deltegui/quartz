#!/bin/sh

if [ "$1" = "--help" ]; then
    echo "Use -ed to enable debug"
    exit 0
fi

CC=clang
LIBS=-lcmocka
BIN="./a.out"
AND_EXEC="&& $BIN"

MACROS=""
if [ "$1" = "-ed" ]; then
    MACROS="-D DEBUG"
fi

echo "\n\n-------- [LEXER TESTS] --------"
sh -c "$CC $MACROS ./lexer_test.c ../lexer.c ../token.c $LIBS $AND_EXEC"

echo "\n\n-------- [PARSER TESTS] --------"
sh -c "$CC $MACROS ./parser_test.c ../parser.c ../expr.c ../lexer.c ../token.c $LIBS $AND_EXEC"

rm $BIN
