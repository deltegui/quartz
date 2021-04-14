#!/bin/sh

if [ "$1" = "--help" ]; then
    echo "Use -ed to enable debug"
    exit 0
fi

CC="clang -g"
LIBS=-lcmocka
BIN="./a.out"
AND_EXEC="&& $BIN"

MACROS=""
if [ "$1" = "-ed" ]; then
    MACROS="-D DEBUG"
fi

KERNEL=`uname -s`
if [ $KERNEL = "Linux" ]; then
    LIBS="-lcmocka -lm"
fi

SOURCES=`find ../*.c -maxdepth 1 ! -name qcc.c | tr '\n' ' '`
echo $SOURCES

# printf "\n\n-------- [LEXER TESTS] --------\n"
# sh -c "$CC $MACROS ./lexer_test.c $SOURCES $LIBS $AND_EXEC"

printf "\n\n-------- [PARSER TESTS] --------\n"
sh -c "$CC $MACROS ./parser_test.c $SOURCES $LIBS $AND_EXEC"

# printf "\n\n-------- [COMPILER TESTS] --------\n"
# sh -c "$CC $MACROS ./compiler_test.c $SOURCES $LIBS $AND_EXEC"
# 
# printf "\n\n-------- [TABLE TESTS] --------\n"
# sh -c "$CC $MACROS ./table_test.c $SOURCES $LIBS $AND_EXEC"
# 
# printf "\n\n-------- [SYMBOL TESTS] --------\n"
# sh -c "$CC $MACROS ./symbol_test.c $SOURCES $LIBS $AND_EXEC"
# 
# rm $BIN
