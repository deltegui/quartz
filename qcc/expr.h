#ifndef QUARTZ_EXPR_H
#define QUARTZ_EXPR_H

#include "lexer.h"

typedef enum {
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_LITERAL,
} ExprKind;

struct _Expr;

typedef struct {
    struct _Expr* left;
    Token op;
    struct _Expr* right;
} BinaryExpr;

typedef struct {
    Token literal;
} LiteralExpr;

typedef struct {
    Token op;
    struct _Expr* expr;
} UnaryExpr;

typedef struct _Expr {
    ExprKind kind;
    union {
        BinaryExpr binary;
        LiteralExpr literal;
        UnaryExpr unary;
    };
} Expr;

typedef struct {
    void (*visit_binary)(void* ctx, BinaryExpr* binary);
    void (*visit_literal)(void* ctx, LiteralExpr* literal);
    void (*visit_unary)(void* ctx, UnaryExpr* unary);
} ExprVisitor;

#define IS_BINARY(expr) (expr.type == EXPR_BINARY)
#define IS_LITERAL(expr) (expr.type == EXPR_LITERAL)
#define IS_UNARY(expr) (expr.type == EXPR_LITERAL)

#define CREATE_BINARY_EXPR(binary) create_expr(EXPR_BINARY, &binary)
#define CREATE_LITERAL_EXPR(literal) create_expr(EXPR_LITERAL, &literal)
#define CREATE_UNARY_EXPR(unary) create_expr(EXPR_UNARY, &unary)

Expr* create_expr(ExprKind type, void* expr_node);
void free_expr(Expr* expr);
void expr_dispatch(ExprVisitor* visitor, void* ctx, Expr* expr);

#endif
