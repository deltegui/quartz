#ifndef QUARTZ_PARSER_H
#define QUARTZ_PARSER_H

#include "lexer.h"
#include "expr.h"

namespace Quartz {

class Parser {
    Lexer* lexer;
    Token current;
    Token next;

    void advance();
    bool consume(TokenType type, const char* msg);

    Expr* expression();
    Expr* sum_expr();
    Expr* mul_expr();
    Expr* group_expr();
    Expr* primary();

public:
    Parser(Lexer* lexer);
    Expr* parse();
};

}

#endif