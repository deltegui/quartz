#ifndef QUARTZ_PARSER_H
#define QUARTZ_PARSER_H

#include "common.h"
#include "lexer.h"
#include "expr.h"
#include "stmt.h"

typedef struct {
    Lexer lexer;
    Token current;
    Token prev;
    bool panic_mode;
    bool has_error;
} Parser;

void init_parser(Parser* parser, const char* source);
Stmt* parse(Parser* parser);

#endif
