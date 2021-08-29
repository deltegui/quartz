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

testn() {
    printf "\n\n-------- [$1 TESTS] --------\n"
    sh -c "$CC $MACROS ./$2_test.c $SOURCES $LIBS $AND_EXEC"
}

test() {
    upcase=`echo "$1" | tr a-z A-Z`
    testn $upcase $1
}
