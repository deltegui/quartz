#include "expr.h"

Expr* expr_create(ExprType type, void* expr_node) {
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

void expr_free(Expr* expr) {
    if (expr == NULL) {
        return;
    }
    switch(expr->type) {
    case EXPR_BINARY:
        expr_free(expr->binary.left);
        expr_free(expr->binary.right);
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
void expr_dispatch(ExprVisitor* visitor, Expr* expr) {
    if (expr == NULL) {
        return;
    }
#define DISPATCH(fn_visitor, node_type) visitor->fn_visitor(&expr->node_type)
    switch(expr->type) {
    case EXPR_LITERAL:
        DISPATCH(visit_literal, literal);
        break;
    case EXPR_BINARY:
        DISPATCH(visit_binary, binary);
        break;
    }
#undef DISPATCH
}