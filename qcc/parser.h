#ifndef QUARTZ_PARSER_H
#define QUARTZ_PARSER_H

#include "common.h"
#include "lexer.h"
#include "expr.h"

typedef struct {
    Lexer* lexer;
    Token current;
    Token next;
    bool has_error;
} Parser;

Parser* create_parser(Lexer* lexer);
void free_parser(Parser* parser);
Expr* parse(Parser* parser);

#endif