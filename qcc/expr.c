#include "expr.h"

Expr* create_expr(ExprKind kind, void* expr_node) {
    Expr* expr = (Expr*) malloc(sizeof(Expr));
    switch(kind) {
    case EXPR_BINARY:
        expr->kind = EXPR_BINARY;
        expr->binary = *(BinaryExpr*)expr_node;
        break;
    case EXPR_LITERAL:
        expr->kind = EXPR_LITERAL;
        expr->literal = *(LiteralExpr*)expr_node;
        break;
    case EXPR_UNARY:
        expr->kind = EXPR_UNARY;
        expr->unary = *(UnaryExpr*)expr_node;
        break;
    case EXPR_IDENTIFIER:
        expr->kind = EXPR_IDENTIFIER;
        expr->identifier = *(IdentifierExpr*)expr_node;
        break;
    case EXPR_ASSIGNMENT:
        expr->kind = EXPR_ASSIGNMENT;
        expr->assignment = *(AssignmentExpr*)expr_node;
        break;
    case EXPR_CALL:
        expr->kind = EXPR_CALL;
        expr->call = *(CallExpr*)expr_node;
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
    switch(expr->kind) {
    case EXPR_BINARY:
        free_expr(expr->binary.left);
        free_expr(expr->binary.right);
        break;
    case EXPR_IDENTIFIER:
    case EXPR_LITERAL:
        // There is nothing to free
        break;
    case EXPR_ASSIGNMENT:
        free_expr(expr->assignment.value);
        break;
    case EXPR_UNARY:
        free_expr(expr->unary.expr);
        break;
    case EXPR_CALL:
        for (int i = 0; i < expr->call.params.size; i++) {
            free_expr(expr->call.params.elements[i].expr);
        }
        free_vector(&expr->call.params);
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
    switch (expr->kind) {
    case EXPR_LITERAL: DISPATCH(visit_literal, literal); break;
    case EXPR_BINARY: DISPATCH(visit_binary, binary); break;
    case EXPR_UNARY: DISPATCH(visit_unary, unary); break;
    case EXPR_IDENTIFIER: DISPATCH(visit_identifier, identifier); break;
    case EXPR_ASSIGNMENT: DISPATCH(visit_assignment, assignment); break;
    case EXPR_CALL: DISPATCH(visit_call, call); break;
    }
#undef DISPATCH
}
