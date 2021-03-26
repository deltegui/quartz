#ifndef QUARTZ_PARSER_H
#define QUARTZ_PARSER_H

#include "common.h"
#include "lexer.h"
#include "expr.h"
#include "stmt.h"

typedef struct {
    Lexer lexer;
    Token current;
    Token next;
    bool has_error;
} Parser;

// Initalize an existing parser using a source to consume
void init_parser(Parser* parser, const char* source);

// Parse using a parser. It gives you a AST. If the return
// is NULL, there is an error or the source is empty.
// Check it using parser.has_error.
Stmt* parse(Parser* parser);

#endif
