CC=clang
LIBS=-lcmocka
BIN="./a.out"
AND_EXEC="&& $BIN"

sh -c "$CC ./lexer_test.c ../lexer.c ../token.c $LIBS $AND_EXEC"

rm $BIN