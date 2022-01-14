#include "expr.h"

static void free_params(Vector* params);

Expr* create_expr(ExprKind kind, const void* const expr_node) {
    Expr* expr = (Expr*) malloc(sizeof(Expr));

#define CASE_EXPR(k, f, s)\
    case k:\
        expr->kind = k;\
        expr->f = *(s*)expr_node;\
        break

    switch(kind) {
    CASE_EXPR(EXPR_BINARY, binary, BinaryExpr);
    CASE_EXPR(EXPR_LITERAL, literal, LiteralExpr);
    CASE_EXPR(EXPR_UNARY, unary, UnaryExpr);
    CASE_EXPR(EXPR_IDENTIFIER, identifier, IdentifierExpr);
    CASE_EXPR(EXPR_ASSIGNMENT, assignment, AssignmentExpr);
    CASE_EXPR(EXPR_CALL, call, CallExpr);
    CASE_EXPR(EXPR_NEW, new_, NewExpr);
    CASE_EXPR(EXPR_PROP, prop, PropExpr);
    CASE_EXPR(EXPR_PROP_ASSIGMENT, prop_assigment, PropAssigmentExpr);
    }
    return expr;

#undef CASE_EXPR
}

// Normally, an Expr* is a tree-like data
// structure, so this function should
// iterate over all nodes and free memory
// for each one. A node pointer can be NULL
// so we must protect ourselves from that.
void free_expr(Expr* const expr) {
    if (expr == NULL) {
        return;
    }
    switch(expr->kind) {
    case EXPR_BINARY:
        free_expr(expr->binary.left);
        free_expr(expr->binary.right);
        break;
    case EXPR_PROP:
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
        free_expr(expr->call.callee);
        free_params(&expr->call.params);
        break;
    case EXPR_NEW:
        free_params(&expr->new_.params);
        break;
    case EXPR_PROP_ASSIGMENT:
        free_expr(expr->prop_assigment.value);
        break;
    }
    free(expr);
}

static void free_params(Vector* params) {
    Expr** exprs = VECTOR_AS_EXPRS(params);
    for (uint32_t i = 0; i < params->size; i++) {
        free_expr(exprs[i]);
    }
    free_vector(params);
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
    case EXPR_NEW: DISPATCH(visit_new, new_); break;
    case EXPR_PROP: DISPATCH(visit_prop, prop); break;
    case EXPR_PROP_ASSIGMENT: DISPATCH(visit_prop_assigment, prop_assigment); break;
    }
#undef DISPATCH
}
