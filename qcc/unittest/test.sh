#!/bin/bash

source ./runner.sh

test "lexer"
test "parser"
test "compiler"
test "table"
test "symbol"
testn "TYPEPOOL" "type"
test "ctable"

rm $BIN
