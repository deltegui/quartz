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

#define CREATE_BINARY_EXPR(binary) create_expr(EXPR_BINARY, &binary)
#define CREATE_LITERAL_EXPR(literal) create_expr(EXPR_LITERAL, &literal)

Expr* create_expr(ExprType type, void* expr_node);
void free_expr(Expr* expr);
void expr_dispatch(ExprVisitor* visitor, void* ctx, Expr* expr);

#endif
