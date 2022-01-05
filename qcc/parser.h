#ifndef QUARTZ_PARSER_H_
#define QUARTZ_PARSER_H_

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

    // TODO scope depth is pretty common in other phasses!!
    int function_deep_count;
    int scope_depth;

    bool is_in_loop;
    Type* current_class_type;
} Parser;

void init_parser(Parser* const parser, const char* source, ScopedSymbolTable* symbols);
Stmt* parse(Parser* const parser);

#endif
