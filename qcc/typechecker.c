#include "typechecker.h"
#include <stdarg.h>
#include "type.h"
#include "common.h"
#include "lexer.h"
#include "expr.h"
#include "symbol.h"

typedef struct {
    ScopedSymbolTable* symbols;
    Type last_type;
    bool has_error;
} Typechecker;

static void error_last_type_match(Typechecker* checker, Token* where, Type first, const char* message);
static void error(Typechecker* checker, Token* token, const char* message, ...);

static void start_scope(Typechecker* checker);
static void end_scope(Typechecker* checker);
static Symbol* lookup_str(Typechecker* checker, const char* name, int length);

static void typecheck_literal(void* ctx, LiteralExpr* literal);
static void typecheck_identifier(void* ctx, IdentifierExpr* identifier);
static void typecheck_binary(void* ctx, BinaryExpr* binary);
static void typecheck_unary(void* ctx, UnaryExpr* unary);
static void typecheck_assignment(void* ctx, AssignmentExpr* assignment);
static void typecheck_call(void* ctx, CallExpr* call);

ExprVisitor typechecker_expr_visitor = (ExprVisitor){
    .visit_literal = typecheck_literal,
    .visit_binary = typecheck_binary,
    .visit_unary = typecheck_unary,
    .visit_identifier = typecheck_identifier,
    .visit_assignment = typecheck_assignment,
    .visit_call = typecheck_call,
};

static void typecheck_expr(void* ctx, ExprStmt* expr);
static void typecheck_var(void* ctx, VarStmt* var);
static void typecheck_print(void* ctx, PrintStmt* print);
static void typecheck_block(void* ctx, BlockStmt* block);
static void typecheck_function(void* ctx, FunctionStmt* function);
static void typecheck_return(void* ctx, ReturnStmt* function);

StmtVisitor typechecker_stmt_visitor = (StmtVisitor){
    .visit_expr = typecheck_expr,
    .visit_var = typecheck_var,
    .visit_print = typecheck_print,
    .visit_block = typecheck_block,
    .visit_function = typecheck_function,
    .visit_return = typecheck_return,
};

#define ACCEPT_STMT(typechecker, stmt) stmt_dispatch(&typechecker_stmt_visitor, typechecker, stmt)
#define ACCEPT_EXPR(typechecker, expr) expr_dispatch(&typechecker_expr_visitor, typechecker, expr)

static void error_last_type_match(Typechecker* checker, Token* where, Type first, const char* message) {
    Type last_type = checker->last_type;
    error(checker, where, "The type '");
    type_print(first);
    printf("' does not match with type '");
    type_print(last_type);
    printf("' %s\n", message);
}

static void error(Typechecker* checker, Token* token, const char* message, ...) {
    checker->has_error = true;
    checker->last_type = TYPE_UNKNOWN;
    va_list params;
    va_start(params, message);
    printf("[Line %d] Type error: ",token->line);
    vprintf(message, params);
    va_end(params);
}

static void start_scope(Typechecker* checker) {
    symbol_start_scope(checker->symbols);
}

static void end_scope(Typechecker* checker) {
    symbol_end_scope(checker->symbols);
}

static Symbol* lookup_str(Typechecker* checker, const char* name, int length) {
    return scoped_symbol_lookup_str(checker->symbols, name, length);
}

bool typecheck(Stmt* ast, ScopedSymbolTable* symbols) {
    Typechecker checker;
    checker.symbols = symbols;
    checker.has_error = false;
    symbol_reset_scopes(checker.symbols);
    ACCEPT_STMT(&checker, ast);
    return !checker.has_error;
}

static void typecheck_block(void* ctx, BlockStmt* block) {
    Typechecker* checker = (Typechecker*) ctx;
    start_scope(checker);
    ACCEPT_STMT(ctx, block->stmts);
    end_scope(checker);
}

static void typecheck_print(void* ctx, PrintStmt* print) {
    ACCEPT_EXPR(ctx, print->inner);
}

static void typecheck_expr(void* ctx, ExprStmt* expr) {
    ACCEPT_EXPR(ctx, expr->inner);
}

static void typecheck_var(void* ctx, VarStmt* var) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* symbol = lookup_str(checker, var->identifier.start, var->identifier.length);
    assert(symbol != NULL);
    if (var->definition == NULL) {
        if (symbol->type == TYPE_UNKNOWN) {
            error(
                checker,
                &var->identifier,
                "Variables without definition cannot be untyped. The type of variable '%.*s' cannot be inferred.\n",
                var->identifier.length,
                var->identifier.start);
        }
        return;
    }
    ACCEPT_EXPR(ctx, var->definition);
    if (symbol->type == checker->last_type) {
        return;
    }
    if (symbol->type == TYPE_UNKNOWN) {
        symbol->type = checker->last_type;
        return;
    }
    error_last_type_match(
        checker,
        &var->identifier,
        symbol->type,
        "in variable declaration.");
}

static void typecheck_identifier(void* ctx, IdentifierExpr* identifier) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* symbol = lookup_str(checker, identifier->name.start, identifier->name.length);
    assert(symbol != NULL);
    checker->last_type = symbol->type;
}

static void typecheck_assignment(void* ctx, AssignmentExpr* assignment) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* symbol = lookup_str(checker, assignment->name.start, assignment->name.length);
    assert(symbol != NULL);

    ACCEPT_EXPR(checker, assignment->value);

    if (symbol->type != checker->last_type) {
        error_last_type_match(
            checker,
            &assignment->name,
            symbol->type,
            "in variable assignment.");
        return;
    }
    checker->last_type = symbol->type;
}

static void typecheck_call(void* ctx, CallExpr* call) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* symbol = lookup_str(checker, call->identifier.start, call->identifier.length);
    assert(symbol != NULL);

    for (int i = 0; i < call->params.size; i++) {
        ACCEPT_EXPR(checker, call->params.params[i].expr);
        Type def_type = symbol->function.param_types.params[i].type;
        Type last = checker->last_type;
        if (last != def_type) {
            error(checker, &call->identifier, "Type of param number %d in function call (", i);
            type_print(last);
            printf(") does not match with function definition (");
            type_print(def_type);
            printf(")\n");
        }
    }

    checker->last_type = symbol->function.return_type;
}

static void typecheck_function(void* ctx, FunctionStmt* function) {
    Typechecker* checker = (Typechecker*) ctx;
    start_scope(checker);
    ACCEPT_STMT(ctx, function->body);
    end_scope(checker);
    Symbol* symbol = lookup_str(checker, function->identifier.start, function->identifier.length);
    assert(symbol != NULL);
    assert(symbol->kind == SYMBOL_FUNCTION);
    if (symbol->function.return_type != checker->last_type) {
        error_last_type_match(
            checker,
            &function->identifier,
            symbol->function.return_type,
            "in function return");
    }
    checker->last_type = symbol->function.return_type;
}

static void typecheck_return(void* ctx, ReturnStmt* return_) {
    // TODO what happends if other stmts alters checker->last_type after a return?
    ACCEPT_EXPR(ctx, return_->inner);
}

static void typecheck_literal(void* ctx, LiteralExpr* literal) {
    Typechecker* checker = (Typechecker*) ctx;

    switch (literal->literal.kind) {
    case TOKEN_NUMBER: {
        checker->last_type = TYPE_NUMBER;
        return;
    }
    case TOKEN_TRUE:
    case TOKEN_FALSE: {
        checker->last_type = TYPE_BOOL;
        return;
    }
    case TOKEN_NIL: {
        checker->last_type = TYPE_NIL;
        return;
    }
    case TOKEN_STRING: {
        checker->last_type = TYPE_STRING;
        return;
    }
    default: {
        error(checker, &literal->literal, "Unknown type in expression");
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

#define ERROR(msg) error(checker, &binary->op, "%s", msg);\
    printf(" for types: '");\
    type_print(left_type);\
    printf("' and '");\
    type_print(right_type);\
    printf("'\n")

    switch (binary->op.kind) {
    case TOKEN_PLUS: {
        if (left_type == TYPE_STRING && right_type == TYPE_STRING) {
            checker->last_type = TYPE_STRING;
            return;
        }
        // just continue to TYPE_NUMBER
    }
    case TOKEN_LOWER:
    case TOKEN_LOWER_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_PERCENT:
    case TOKEN_SLASH: {
        if (left_type == TYPE_NUMBER && right_type == TYPE_NUMBER) {
            checker->last_type = TYPE_NUMBER;
            return;
        }
        ERROR("Invalid types for numeric operation");
        return;
    }
    case TOKEN_AND:
    case TOKEN_OR: {
        if (left_type == TYPE_BOOL && right_type == TYPE_BOOL) {
            checker->last_type = TYPE_BOOL;
            return;
        }
        ERROR("Invalid types for boolean operation");
        return;
    }
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL: {
        if (left_type == right_type) {
            checker->last_type = TYPE_BOOL;
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

#define ERROR(msg) error(checker, &unary->op, "%s", msg);\
    printf(" for type: '");\
    type_print(inner_type);\
    printf("'\n")

    switch (unary->op.kind) {
    case TOKEN_BANG: {
        if (inner_type == TYPE_BOOL) {
            checker->last_type = TYPE_BOOL;
            return;
        }
        ERROR("Invalid type for not operation");
        return;
    }
    case TOKEN_PLUS:
    case TOKEN_MINUS: {
        if (inner_type == TYPE_NUMBER) {
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
