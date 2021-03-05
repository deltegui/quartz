#include "typechecker.h"
#include "common.h"
#include "lexer.h"

typedef enum {
    INT_TYPE,
    FLOAT_TYPE,
    BOOL_TYPE,
    UNKNOWN_TYPE,
} Type;

typedef struct {
    Type last_type;
    bool has_error;
} Typechecker;

static bool is_number_type(Type type);
static void print_type(Type type);
static void error(Typechecker* checker, const char* msg, Token* token);

static void typecheck_literal(void* ctx, LiteralExpr* literal);
static void typecheck_binary(void* ctx, BinaryExpr* binary);
static void typecheck_unary(void* ctx, UnaryExpr* unary);

ExprVisitor typechecker_visitor = (ExprVisitor){
    .visit_literal = typecheck_literal,
    .visit_binary = typecheck_binary,
    .visit_unary = typecheck_unary,
};

#define ACCEPT(typechecker, expr) expr_dispatch(&typechecker_visitor, typechecker, expr)

static void print_type(Type type) {
    switch (type) {
    case INT_TYPE: printf("Int"); return;
    case FLOAT_TYPE: printf("Float"); return;
    case BOOL_TYPE: printf("Bool"); return;
    case UNKNOWN_TYPE: printf("Unknown"); return;
    }
}

static void error(Typechecker* checker, const char* msg, Token* token) {
    printf(
        "[Line %d] Type error: %s: '%.*s'",
        token->line,
        msg,
        token->length,
        token->start);
    checker->has_error = true;
    checker->last_type = UNKNOWN_TYPE;
}

bool typecheck(Expr* ast) {
    Typechecker checker;
    checker.has_error = false;
    ACCEPT(&checker, ast);
    return !checker.has_error;
}

static void typecheck_literal(void* ctx, LiteralExpr* literal) {
    Typechecker* checker = (Typechecker*) ctx;

    switch (literal->literal.type) {
    case TOKEN_INTEGER: {
        checker->last_type = INT_TYPE;
        return;
    }
    case TOKEN_FLOAT: {
        checker->last_type = FLOAT_TYPE;
        return;
    }
    case TOKEN_TRUE:
    case TOKEN_FALSE: {
        checker->last_type = BOOL_TYPE;
        return;
    }
    default: {
        error(checker, "Unknown type in expression", &literal->literal);
        printf("\n");
        return;
    }
    }
}

static bool is_number_type(Type type) {
    return type == INT_TYPE || type == FLOAT_TYPE;
}

static void typecheck_binary(void* ctx, BinaryExpr* binary) {
    Typechecker* checker = (Typechecker*) ctx;
    ACCEPT(checker, binary->left);
    Type left_type = checker->last_type;
    ACCEPT(checker, binary->right);
    Type right_type = checker->last_type;

#define ERROR(msg) error(checker, msg, &binary->op);\
    printf(" for types: '");\
    print_type(left_type);\
    printf("' and '");\
    print_type(right_type);\
    printf("'\n")

    switch (binary->op.type) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_PERCENT: {
        if (left_type == INT_TYPE && right_type == INT_TYPE) {
            checker->last_type = INT_TYPE;
            return;
        }
        // continue to TOKEN_SLASH. DO NOT BREAK OR RETURN HERE.
    }
    case TOKEN_SLASH: {
        if (is_number_type(left_type) && is_number_type(right_type)) {
            checker->last_type = FLOAT_TYPE;
            return;
        }
        ERROR("Invalid types for numeric operation");
        return;
    }
    case TOKEN_AND:
    case TOKEN_OR: {
        if (left_type == BOOL_TYPE && right_type == BOOL_TYPE) {
            checker->last_type = BOOL_TYPE;
            return;
        }
        ERROR("Invalid types for boolean operation");
        return;
    }
    default: {
        ERROR("Unkown binary operation");
        return;
    }
    }

#undef ERROR
}

static void typecheck_unary(void* ctx, UnaryExpr* unary) {
    Typechecker* checker = (Typechecker*) ctx;
    ACCEPT(checker, unary->expr);
    Type inner_type = checker->last_type;

#define ERROR(msg) error(checker, msg, &unary->op);\
    printf(" for type: '");\
    print_type(inner_type);\
    printf("'\n")

    switch (unary->op.type) {
    case TOKEN_BANG: {
        if (inner_type == BOOL_TYPE) {
            checker->last_type = BOOL_TYPE;
            return;
        }
        ERROR("Invalid type for not operation");
        return;
    }
    case TOKEN_PLUS:
    case TOKEN_MINUS: {
        if (is_number_type(inner_type)) {
            checker->last_type = inner_type;
            return;
        }
        ERROR("Cannot apply plus or minus unary operation");
        return;
    }
    default: {
        ERROR("Unkown unary operation");
        return;
    }
    }

#undef ERROR
}