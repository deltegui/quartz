#ifndef QUARTZ_PARSER_H
#define QUARTZ_PARSER_H

#include "common.h"
#include "lexer.h"
#include "expr.h"
#include "stmt.h"
#include "symbol.h"

typedef struct {
    ScopedSymbolTable* symbols;
    Lexer lexer;
    Token current;
    Token prev;
    bool panic_mode;
    bool has_error;
    int function_deep_count;
} Parser;

void init_parser(Parser* parser, const char* source, ScopedSymbolTable* symbols);
Stmt* parse(Parser* parser);

#endif
