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
        printf("%s\n", msg);
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
    printf("Expression!\n");
    return this->sum_expr();
}

Expr* Parser::sum_expr() {
    printf("Sum Expr!\n");
    Expr* left = mul_expr();
    Operator op;
    switch(this->next.type) {
    case TokenType::Plus:
        printf("Operator is +\n");
        op = Operator::Sum;
        break;
    case TokenType::Minus:
        printf("Operator is -\n");
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
    printf("MUL expr!\n");
    Expr* left = group_expr();
    Operator op;
    switch(this->next.type) {
    case TokenType::Star:
        printf("Operator is *\n");
        op = Operator::Mul;
        break;
    case TokenType::Slash:
        printf("Operator is /\n");
        op = Operator::Div;
        break;
    default:
        return left;
    }
    this->advance();
    Expr* right = sum_expr();
    BinaryExpr* binary = new BinaryExpr();;
    binary->left = left;
    binary->op = op;
    binary->right = right;
    return binary;
}

Expr* Parser::group_expr() {
    printf("Group expr!\n");
    if (this->next.type != TokenType::LeftParen) {
        printf("FUCK THERE ISNT A OPEN PAREN, IS PRIMARY!\n");
        return this->primary();
    }
    printf("I swallow an open paren\n");
    this->advance(); // Consume (
    Expr* inner = this->expression();
    bool ok = this->consume(
        TokenType::RightParen,
        "Expected ')' to enclose '(' in group expression");
    if (!ok) {
        printf("C mamo!\n");
        return NULL;
    }
    GroupingExpr* grouping = new GroupingExpr;
    grouping->inner = inner;
    return grouping;
}

Expr* Parser::primary() {
    printf("Primary expr!\n");
    LiteralExpr* expr = new LiteralExpr();
    expr->literal = this->next;
    print_token(expr->literal);
    this->advance();
    return expr;
}

}