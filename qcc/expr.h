#ifndef QUARTZ_EXPR_H
#define QUARTZ_EXPR_H

#include "lexer.h"

typedef enum {
    EXPR_BINARY,
    EXPR_LITERAL,
} ExprType;

struct _Expr;

typedef struct {
    struct _Expr* left;
    Token op;
    struct _Expr* right;
} BinaryExpr;

typedef struct {
    Token literal;
} LiteralExpr;

typedef struct _Expr {
    ExprType type;
    union {
        BinaryExpr binary;
        LiteralExpr literal;
    };
} Expr;

typedef struct {
    void (*visit_binary)(void* ctx, BinaryExpr* binary);
    void (*visit_literal)(void* ctx, LiteralExpr* literal);
} ExprVisitor;

#define IS_BINARY(expr) (expr.type == EXPR_BINARY)
#define IS_LITERAL(expr) (expr.type == EXPR_LITERAL)

#define CREATE_BINARY_EXPR(binary) expr_create(EXPR_BINARY, &binary)
#define CREATE_LITERAL_EXPR(literal) expr_create(EXPR_LITERAL, &literal)

Expr* expr_create(ExprType type, void* expr_node);
void expr_free(Expr* expr);
void expr_dispatch(ExprVisitor* visitor, void* ctx, Expr* expr);

#endif
