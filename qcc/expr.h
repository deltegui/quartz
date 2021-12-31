#ifndef QUARTZ_EXPR_H_
#define QUARTZ_EXPR_H_

#include "lexer.h"
#include "vector.h"

typedef enum {
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_LITERAL,
    EXPR_IDENTIFIER,
    EXPR_ASSIGNMENT,
    EXPR_CALL,
    EXPR_NEW,
    EXPR_PROP,
    EXPR_PROP_ASSIGMENT,
} ExprKind;

struct s_expr;

typedef struct {
    struct s_expr* left;
    Token op;
    struct s_expr* right;
} BinaryExpr;

typedef struct {
    Token literal;
} LiteralExpr;

typedef struct {
    Token name;
} IdentifierExpr;

typedef struct {
    Token name;
    struct s_expr* value;
} AssignmentExpr;

typedef struct {
    Token op;
    struct s_expr* expr;
} UnaryExpr;

typedef struct {
    Vector params;
    // Token identifier;
    struct s_expr* callee;
} CallExpr;

typedef struct {
    Vector params;
    Token klass;
} NewExpr;

typedef struct {
    Token identifier;
    Token prop;
} PropExpr;

typedef struct {
    Token identifier;
    Token prop;
    struct s_expr* value;
} PropAssigmentExpr;

typedef struct s_expr {
    ExprKind kind;
    union {
        CallExpr call;
        BinaryExpr binary;
        LiteralExpr literal;
        UnaryExpr unary;
        IdentifierExpr identifier;
        AssignmentExpr assignment;
        NewExpr new_;
        PropExpr prop;
        PropAssigmentExpr prop_assigment;
    };
} Expr;

typedef struct {
    void (*visit_binary)(void* ctx, BinaryExpr* binary);
    void (*visit_literal)(void* ctx, LiteralExpr* literal);
    void (*visit_unary)(void* ctx, UnaryExpr* unary);
    void (*visit_identifier)(void* ctx, IdentifierExpr* identifier);
    void (*visit_assignment)(void* ctx, AssignmentExpr* identifier);
    void (*visit_call)(void* ctx, CallExpr* call);
    void (*visit_new)(void* ctx, NewExpr* new_);
    void (*visit_prop)(void* ctx, PropExpr* prop);
    void (*visit_prop_assigment)(void* ctx, PropAssigmentExpr* prop_assigment);
} ExprVisitor;

#define EXPR_IS_BINARY(expr) ((expr).kind == EXPR_BINARY)
#define EXPR_IS_LITERAL(expr) ((expr).kind == EXPR_LITERAL)
#define EXPR_IS_UNARY(expr) ((expr).kind == EXPR_UNARY)
#define EXPR_IS_IDENTIFIER(expr) ((expr).kind == EXPR_IDENTIFIER)
#define EXPR_IS_ASSIGNMENT(expr) ((expr).kind == EXPR_ASSIGNMENT)
#define EXPR_IS_CALL(expr) ((expr).kind == EXPR_CALL)
#define EXPR_IS_NEW(expr) ((expr).kind == EXPR_NEW)
#define EXPR_IS_PROP(expr) ((expr).kind == EXPR_PROP)
#define EXPR_IS_PROP_ASSIGMENT(expr) ((expr).kind == EXPR_PROP_ASSIGMENT)

#define CREATE_BINARY_EXPR(binary) create_expr(EXPR_BINARY, &binary)
#define CREATE_LITERAL_EXPR(literal) create_expr(EXPR_LITERAL, &literal)
#define CREATE_UNARY_EXPR(unary) create_expr(EXPR_UNARY, &unary)
#define CREATE_INDENTIFIER_EXPR(identifier) create_expr(EXPR_IDENTIFIER, &identifier)
#define CREATE_ASSIGNMENT_EXPR(assignment) create_expr(EXPR_ASSIGNMENT, &assignment)
#define CREATE_CALL_EXPR(call) create_expr(EXPR_CALL, &call)
#define CREATE_NEW_EXPR(new_) create_expr(EXPR_NEW, &new_)
#define CREATE_PROP_EXPR(prop) create_expr(EXPR_PROP, &prop)
#define CREATE_PROP_ASSIGMENT_EXPR(prop_assigment) create_expr(EXPR_PROP_ASSIGMENT, &prop_assigment)

Expr* create_expr(ExprKind type, const void* const expr_node);
void free_expr(Expr* const expr);
void expr_dispatch(ExprVisitor* visitor, void* ctx, Expr* expr);

#define VECTOR_AS_EXPRS(vect) VECTOR_AS(vect, Expr*)
#define VECTOR_ADD_EXPR(vect, expr_ptr) VECTOR_ADD(vect, expr_ptr, Expr*)

#endif
