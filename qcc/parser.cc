#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

namespace Quartz {

Parser::Parser(Lexer* lexer) {
    this->lexer = lexer;
    this->next = lexer->next_token();
}

void Parser::advance() {
    this->current = this->next;
    this->next = this->lexer->next_token();
}

bool Parser::consume(TokenType expected, const char* msg) {
    if (this->next.type != expected) {
        return false;
    }
    this->advance();
    return true;
}

Expr* Parser::parse() {
    if (this->next.type == TokenType::End || this->next.type == TokenType::Error) {
        return NULL;
    }
    return this->expression();
}

Expr* Parser::expression() {
    return this->sum_expr();
}

Expr* Parser::sum_expr() {
    Expr* left = mul_expr();
    Operator op;
    switch (this->next.type) {
    case TokenType::Plus:
        op = Operator::Sum;
        break;
    case TokenType::Minus:
        op = Operator::Sub;
        break;
    default:
        return left;
    }
    this->advance();
    Expr* right = sum_expr();
    BinaryExpr* binary = new BinaryExpr();
    binary->left = left;
    binary->op = op;
    binary->right = right;
    return binary;
}

Expr* Parser::mul_expr() {
    Expr* left = group_expr();
    Operator op;
    switch (this->next.type) {
    case TokenType::Star:
        op = Operator::Mul;
        break;
    case TokenType::Slash:
        op = Operator::Div;
        break;
    default:
        return left;
    }
    this->advance();
    Expr* right = sum_expr();
    BinaryExpr* binary = new BinaryExpr();
    binary->left = left;
    binary->op = op;
    binary->right = right;
    return binary;
}

Expr* Parser::group_expr() {
    if (this->next.type != TokenType::LeftParen) {
        return this->primary();
    }
    this->advance(); // Consume (
    Expr* inner = this->expression();
    bool ok = this->consume(
        TokenType::RightParen,
        "Expected ')' to enclose '(' in group expression");
    if (!ok) {
        return NULL;
    }
    GroupingExpr* grouping = new GroupingExpr;
    grouping->inner = inner;
    return grouping;
}

Expr* Parser::primary() {
    LiteralExpr* expr = new LiteralExpr();
    expr->literal = this->next;
    this->advance();
    return expr;
}

}