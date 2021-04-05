#include "typechecker.h"
#include "type.h"
#include "common.h"
#include "lexer.h"
#include "expr.h"
#include "symbol.h"

typedef struct {
    Type last_type;
    bool has_error;
} Typechecker;

static void error(Typechecker* checker, const char* msg, Token* token);

static void typecheck_literal(void* ctx, LiteralExpr* literal);
static void typecheck_identifier(void* ctx, IdentifierExpr* identifier);
static void typecheck_binary(void* ctx, BinaryExpr* binary);
static void typecheck_unary(void* ctx, UnaryExpr* unary);
static void typecheck_assignment(void* ctx, AssignmentExpr* assignment);

ExprVisitor typechecker_expr_visitor = (ExprVisitor){
    .visit_literal = typecheck_literal,
    .visit_binary = typecheck_binary,
    .visit_unary = typecheck_unary,
    .visit_identifier = typecheck_identifier,
    .visit_assignment = typecheck_assignment,
};

static void typecheck_expr(void* ctx, ExprStmt* expr);
static void typecheck_var(void* ctx, VarStmt* var);
static void typecheck_print(void* ctx, PrintStmt* print);

StmtVisitor typechecker_stmt_visitor = (StmtVisitor){
    .visit_expr = typecheck_expr,
    .visit_var = typecheck_var,
    .visit_print = typecheck_print,
};

#define ACCEPT_STMT(typechecker, stmt) stmt_dispatch(&typechecker_stmt_visitor, typechecker, stmt)
#define ACCEPT_EXPR(typechecker, expr) expr_dispatch(&typechecker_expr_visitor, typechecker, expr)

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

bool typecheck(Stmt* ast) {
    Typechecker checker;
    checker.has_error = false;
    ACCEPT_STMT(&checker, ast);
    return !checker.has_error;
}

static void typecheck_print(void* ctx, PrintStmt* print) {
    ACCEPT_EXPR(ctx, print->inner);
}

static void typecheck_expr(void* ctx, ExprStmt* expr) {
    ACCEPT_EXPR(ctx, expr->inner);
}

static void typecheck_var(void* ctx, VarStmt* var) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* symbol = CSYMBOL_LOOKUP_STR(var->identifier.start, var->identifier.length);
    assert(symbol != NULL);
    if (var->definition == NULL) {
        if (symbol->type == UNKNOWN_TYPE) {
            error(checker, "Variables without declaration cannot be untyped. The variable type cannot be inferred", &var->identifier);
            printf("\n");
        }
        return;
    }
    ACCEPT_EXPR(ctx, var->definition);
    if (symbol->type == checker->last_type) {
        return;
    }
    if (symbol->type == UNKNOWN_TYPE) {
        symbol->type = checker->last_type;
        return;
    }
    error(checker, "Variable type does not match with asignment type", &var->identifier);
    printf("\n");
}

static void typecheck_identifier(void* ctx, IdentifierExpr* identifier) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* symbol = CSYMBOL_LOOKUP_STR(identifier->name.start, identifier->name.length);
    assert(symbol != NULL);
    checker->last_type = symbol->type;
}

static void typecheck_assignment(void* ctx, AssignmentExpr* assignment) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* symbol = CSYMBOL_LOOKUP_STR(assignment->name.start, assignment->name.length);
    assert(symbol != NULL);

    ACCEPT_EXPR(checker, assignment->value);

    if (symbol->type != checker->last_type) {
        error(checker, "Variable type does not match with asignment type", &assignment->name);
        printf("\n");
        return;
    }
    checker->last_type = symbol->type;
}

static void typecheck_literal(void* ctx, LiteralExpr* literal) {
    Typechecker* checker = (Typechecker*) ctx;

    switch (literal->literal.kind) {
    case TOKEN_NUMBER: {
        checker->last_type = NUMBER_TYPE;
        return;
    }
    case TOKEN_TRUE:
    case TOKEN_FALSE: {
        checker->last_type = BOOL_TYPE;
        return;
    }
    case TOKEN_NIL: {
        checker->last_type = NIL_TYPE;
        return;
    }
    case TOKEN_STRING: {
        checker->last_type = STRING_TYPE;
        return;
    }
    default: {
        error(checker, "Unknown type in expression", &literal->literal);
        printf("\n");
        return;
    }
    }
}

static void typecheck_binary(void* ctx, BinaryExpr* binary) {
    Typechecker* checker = (Typechecker*) ctx;
    ACCEPT_EXPR(checker, binary->left);
    Type left_type = checker->last_type;
    ACCEPT_EXPR(checker, binary->right);
    Type right_type = checker->last_type;

#define ERROR(msg) error(checker, msg, &binary->op);\
    printf(" for types: '");\
    type_print(left_type);\
    printf("' and '");\
    type_print(right_type);\
    printf("'\n")

    switch (binary->op.kind) {
    case TOKEN_PLUS: {
        if (left_type == STRING_TYPE && right_type == STRING_TYPE) {
            checker->last_type = STRING_TYPE;
            return;
        }
        // just continue to NUMBER_TYPE
    }
    case TOKEN_LOWER:
    case TOKEN_LOWER_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_PERCENT:
    case TOKEN_SLASH: {
        if (left_type == NUMBER_TYPE && right_type == NUMBER_TYPE) {
            checker->last_type = NUMBER_TYPE;
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
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL: {
        if (left_type == right_type) {
            checker->last_type = BOOL_TYPE;
            return;
        }
        ERROR("Elements with different types arent coparable");
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
    ACCEPT_EXPR(checker, unary->expr);
    Type inner_type = checker->last_type;

#define ERROR(msg) error(checker, msg, &unary->op);\
    printf(" for type: '");\
    type_print(inner_type);\
    printf("'\n")

    switch (unary->op.kind) {
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
        if (inner_type == NUMBER_TYPE) {
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
