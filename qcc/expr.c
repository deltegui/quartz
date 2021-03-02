#include "expr.h"

Expr* create_expr(ExprType type, void* expr_node) {
    Expr* expr = (Expr*) malloc(sizeof(Expr));
    switch(type) {
    case EXPR_BINARY:
        expr->type = EXPR_BINARY;
        expr->binary = *(BinaryExpr*)expr_node;
        break;
    case EXPR_LITERAL:
        expr->type = EXPR_LITERAL;
        expr->literal = *(LiteralExpr*)expr_node;
        break;
    }
    return expr;
}

// Normally, an Expr* is a tree-like data
// structure, so this function should
// iterate over all nodes and free memory
// for each one. A node pointer can be NULL
// so we must protect ourselves from that.
void free_expr(Expr* expr) {
    if (expr == NULL) {
        return;
    }
    switch(expr->type) {
    case EXPR_BINARY:
        free_expr(expr->binary.left);
        free_expr(expr->binary.right);
        break;
    case EXPR_LITERAL:
        // There is nothing to free
        break;
    }
    free(expr);
}

// Here we emulate visitor pattern in C. The DISPATCH macro
// will call the correct function from ExprVisitor. Notice that
// the expanded code &expr->node_type, node_type is not
// a property from expr, is a parameter of the macro. So the call
// DISPATCH(visit_literal, literal) is accessing to the literal
// property of the union in Expr struct.
// The void* ctx is a struct that have information needed to handle
// the expression.
void expr_dispatch(ExprVisitor* visitor, void* ctx, Expr* expr) {
    if (expr == NULL) {
        return;
    }
#define DISPATCH(fn_visitor, node_type) visitor->fn_visitor(ctx, &expr->node_type)
    switch (expr->type) {
    case EXPR_LITERAL: DISPATCH(visit_literal, literal); break;
    case EXPR_BINARY: DISPATCH(visit_binary, binary); break;
    }
#undef DISPATCH
}