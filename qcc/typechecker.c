#include "typechecker.h"
#include <stdarg.h>
#include "type.h"
#include "common.h"
#include "lexer.h"
#include "expr.h"
#include "symbol.h"

typedef struct {
    Token name;
    int scope_distance;
} FuncMeta;

#define VECTOR_AS_FUNC_META(vect) VECTOR_AS(vect, FuncMeta)
#define VECTOR_ADD_FUNC_META(vect, meta) VECTOR_ADD(vect, meta, FuncMeta)

typedef struct {
    ScopedSymbolTable* symbols;
    Type* last_type;
    bool has_error;

    Vector function_stack; // Vector<FuncMeta>
    int function_stack_top;
} Typechecker;

#define TYPECHECK_IS_GLOBAL_FN(checker) (checker->function_stack_top == 0)

static void function_stack_pop(Typechecker* const checker);
static FuncMeta* function_stack_peek(Typechecker* const checker);
static void function_stack_push(Typechecker* const checker, Token fn_token);
static void function_stack_start_scope(Typechecker* const checker);
static void function_stack_end_scope(Typechecker* const checker);
static void error_last_type_match(Typechecker* const checker, Token* where, Type* first, const char* message);
static void error(Typechecker* const checker, Token* token, const char* message, ...);

static void start_scope(Typechecker* const checker);
static void end_scope(Typechecker* const checker);
static Symbol* lookup_str(Typechecker* const checker, const char* name, int length);
static Symbol* lookup_levels(Typechecker* const checker, SymbolName name, int level);
static void typecheck_params_arent_void(Typechecker* const checker, Symbol* symbol);
static void check_and_mark_upvalue(Typechecker* const checker, Symbol* var);
static bool var_is_current_function_local(Typechecker* const checker, Symbol* var);

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

static void function_stack_pop(Typechecker* const checker) {
    assert(checker->function_stack_top > 0);
    // TODO function_stack_top is needed?
    checker->function_stack_top--;
    checker->function_stack.size--;
}

static FuncMeta* function_stack_peek(Typechecker* const checker) {
    assert(checker->function_stack.size > 0);
    FuncMeta* meta = VECTOR_AS_FUNC_META(&checker->function_stack);
    return &meta[checker->function_stack_top - 1];
}

static void function_stack_push(Typechecker* const checker, Token fn_token) {
    FuncMeta meta;
    meta.name = fn_token;
    meta.scope_distance = 0;
    VECTOR_ADD_FUNC_META(&checker->function_stack, meta);
    checker->function_stack_top++;
}

static void function_stack_start_scope(Typechecker* const checker) {
    if (TYPECHECK_IS_GLOBAL_FN(checker)) {
        return;
    }
    FuncMeta* meta = function_stack_peek(checker);
    meta->scope_distance++;
}

static void function_stack_end_scope(Typechecker* const checker) {
    if (TYPECHECK_IS_GLOBAL_FN(checker)) {
        return;
    }
    FuncMeta* meta = function_stack_peek(checker);
    meta->scope_distance--;
}

static void error_last_type_match(Typechecker* const checker, Token* where, Type* first, const char* message) {
    Type* last_type = checker->last_type;
    error(checker, where, "The type '");
    type_print(first);
    printf("' does not match with type '");
    type_print(last_type);
    printf("' %s\n", message);
}

static void error(Typechecker* const checker, Token* token, const char* message, ...) {
    checker->has_error = true;
    checker->last_type = CREATE_TYPE_UNKNOWN();
    va_list params;
    va_start(params, message);
    printf("[Line %d] Type error: ",token->line);
    vprintf(message, params);
    va_end(params);
}

static void start_scope(Typechecker* const checker) {
    symbol_start_scope(checker->symbols);
}

static void end_scope(Typechecker* const checker) {
    symbol_end_scope(checker->symbols);
}

static Symbol* lookup_str(Typechecker* const checker, const char* name, int length) {
    return scoped_symbol_lookup_str(checker->symbols, name, length);
}

static Symbol* lookup_levels(Typechecker* const checker, SymbolName name, int level) {
    return scoped_symbol_lookup_levels(checker->symbols, &name, level);
}

bool typecheck(Stmt* ast, ScopedSymbolTable* symbols) {
    Typechecker checker;
    checker.symbols = symbols;
    checker.has_error = false;
    checker.function_stack_top = 0;
    init_vector(&checker.function_stack, sizeof(Token));
    symbol_reset_scopes(checker.symbols);
    ACCEPT_STMT(&checker, ast);
    free_vector(&checker.function_stack);
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
        if (TYPE_IS_UNKNOWN(symbol->type)) {
            error(
                checker,
                &var->identifier,
                "Variables without definition cannot be untyped. The type of variable '%.*s' cannot be inferred.\n",
                var->identifier.length,
                var->identifier.start);
        }
        if (TYPE_IS_VOID(symbol->type)) {
            error(
                checker,
                &var->identifier,
                "Variables cannot be of type Void. Invalid type for variable '%.*s'\n",
                var->identifier.length,
                var->identifier.start);
        }
        return;
    }
    ACCEPT_EXPR(ctx, var->definition);
    if (TYPE_IS_VOID(checker->last_type)) {
        error(
            checker,
            &var->identifier,
            "Cannot declare Void variable\n");
        return;
    }
    if (type_equals(symbol->type, checker->last_type)) {
        return;
    }
    if (TYPE_IS_UNKNOWN(symbol->type)) {
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
    if (symbol->declaration_line == identifier->name.line) {
        error(
            checker,
            &identifier->name,
            "Use of identifier inside declaration\n");
    }

    check_and_mark_upvalue(checker, symbol);

    checker->last_type = symbol->type;
}

static void typecheck_assignment(void* ctx, AssignmentExpr* assignment) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* symbol = lookup_str(checker, assignment->name.start, assignment->name.length);
    assert(symbol != NULL);

    ACCEPT_EXPR(checker, assignment->value);

    if (TYPE_IS_VOID(checker->last_type)) {
        error(
            checker,
            &assignment->name,
            "Cannot assign variable to Void\n");
        return;
    }
    if (! type_equals(symbol->type, checker->last_type)) {
        error_last_type_match(
            checker,
            &assignment->name,
            symbol->type,
            "in variable assignment.");
        return;
    }

    check_and_mark_upvalue(checker, symbol);

    checker->last_type = symbol->type;
}

static void typecheck_call(void* ctx, CallExpr* call) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* symbol = lookup_str(checker, call->identifier.start, call->identifier.length);
    assert(symbol != NULL);
    assert(symbol->type != NULL);

    if (! TYPE_IS_FUNCTION(symbol->type)) {
        error(
            checker,
            &call->identifier,
            "Calling '%.*s' which is not a function\n",
            call->identifier.length,
            call->identifier.start);
        return;
    }

    if (! symbol->assigned) {
        error(
            checker,
            &call->identifier,
            "Calling '%.*s' which is not assigned\n",
            call->identifier.length,
            call->identifier.start);
        return;
    }

    // TODO here is the first time that the size of param_type is not equal to param_names.
    // It is correct?
    // If so, its good idea to search for usages of param_type and param_names to show if you only
    // check the size of one assuming the other will have the same size.
    uint32_t fn_param_type_size = TYPE_FN_PARAMS(symbol->type).size;
    if (fn_param_type_size != call->params.size) {
        error(
            checker,
            &call->identifier,
            "Function '%.*s' expects %d params, but was called with %d params\n",
            call->identifier.length,
            call->identifier.start,
            fn_param_type_size,
            call->params.size);
        return;
    }

    Expr** exprs = VECTOR_AS_EXPRS(&call->params);
    Type** param_types = VECTOR_AS_TYPES(&TYPE_FN_PARAMS(symbol->type));
    assert(call->params.size == TYPE_FN_PARAMS(symbol->type).size);

    for (uint32_t i = 0; i < call->params.size; i++) {
        ACCEPT_EXPR(checker, exprs[i]);
        Type* def_type = param_types[i];
        Type* last = checker->last_type;
        if (! type_equals(last, def_type)) {
            error(checker, &call->identifier, "Type of param number %d in function call (", i);
            type_print(last);
            printf(") does not match with function definition (");
            type_print(def_type);
            printf(")\n");
        }
    }

    check_and_mark_upvalue(checker, symbol);

    checker->last_type = TYPE_FN_RETURN(symbol->type);
}

static void check_and_mark_upvalue(Typechecker* const checker, Symbol* var) {
#ifdef TYPECHECKER_DEBUG
    printf(
        "[TYPECHECKER DEBUG] Checking variable '%.*s'\n[TYPECHECKER DEBUG] Upvalue? ",
        SYMBOL_NAME_LENGTH(var->name),
        SYMBOL_NAME_START(var->name));
#endif
    if (checker->function_stack_top == 0) {
#ifdef TYPECHECKER_DEBUG
        printf("No, you are using it in global.\n");
#endif
        return;
    }
    if (var_is_current_function_local(checker, var)) {
#ifdef TYPECHECKER_DEBUG
        printf("No, is local to current function.\n");
#endif
        return;
    }
    if (var->global) {
#ifdef TYPECHECKER_DEBUG
        printf("No, is defined in global\n");
#endif
        return;
    }
#ifdef TYPECHECKER_DEBUG
    printf("Yes\n");
#endif
    FuncMeta* meta = function_stack_peek(checker);
    Symbol* fn_sym = lookup_str(checker, meta->name.start, meta->name.length);
    assert(fn_sym != NULL);
    assert(fn_sym->kind == SYMBOL_FUNCTION);
    scoped_symbol_upvalue(checker->symbols, fn_sym, var);
}

static bool var_is_current_function_local(Typechecker* const checker, Symbol* var) {
    FuncMeta* meta = function_stack_peek(checker);
    Symbol* var_sym = lookup_levels(checker, var->name, meta->scope_distance);
    return var_sym != NULL;
}

static void typecheck_function(void* ctx, FunctionStmt* function) {
    Typechecker* checker = (Typechecker*) ctx;
    function_stack_push(checker, function->identifier);

    start_scope(checker);
    function_stack_start_scope(checker);
    ACCEPT_STMT(ctx, function->body);
    function_stack_end_scope(checker);
    end_scope(checker);

    Symbol* symbol = lookup_str(checker, function->identifier.start, function->identifier.length);
    assert(symbol != NULL);
    assert(symbol->kind == SYMBOL_FUNCTION);
    typecheck_params_arent_void(checker, symbol);

    function_stack_pop(checker);

    checker->last_type = TYPE_FN_RETURN(symbol->type);
}

static void typecheck_params_arent_void(Typechecker* const checker, Symbol* symbol) {
    Vector* vector_types = &TYPE_FN_PARAMS(symbol->type);
    Vector* vector_names = &symbol->function.param_names;
    assert(vector_types->size == vector_names->size);

    Token* param_names = VECTOR_AS_TOKENS(vector_names);
    Type** param_types = VECTOR_AS_TYPES(vector_types);
    assert(vector_types->size == vector_names->size);

    for (uint32_t i = 0; i < vector_types->size; i++) {
        assert(param_names[i].length > 0);
        if (TYPE_IS_VOID(param_types[i])) {
            error(
                checker,
                &param_names[i],
                "Function param '%.*s' cannot be Void\n",
                param_names[i].length,
                param_names[i].start);
        }
    }
}

static void typecheck_return(void* ctx, ReturnStmt* return_) {
    Typechecker* checker = (Typechecker*) ctx;
    ACCEPT_EXPR(ctx, return_->inner);
    if (return_->inner == NULL) {
        checker->last_type = CREATE_TYPE_VOID();
    }
    FuncMeta* meta = function_stack_peek(checker);
    Token func_identifier = meta->name;
    Symbol* symbol = lookup_str(checker, func_identifier.start, func_identifier.length);
    assert(symbol != NULL);
    assert(symbol->kind == SYMBOL_FUNCTION);
    if (! type_equals(TYPE_FN_RETURN(symbol->type), checker->last_type)) {
        error_last_type_match(
            checker,
            &func_identifier,
            TYPE_FN_RETURN(symbol->type),
            "in function return");
    }
}

static void typecheck_literal(void* ctx, LiteralExpr* literal) {
    Typechecker* checker = (Typechecker*) ctx;

    switch (literal->literal.kind) {
    case TOKEN_NUMBER: {
        checker->last_type = CREATE_TYPE_NUMBER();
        return;
    }
    case TOKEN_TRUE:
    case TOKEN_FALSE: {
        checker->last_type = CREATE_TYPE_BOOL();
        return;
    }
    case TOKEN_NIL: {
        checker->last_type = CREATE_TYPE_NIL();
        return;
    }
    case TOKEN_STRING: {
        checker->last_type = CREATE_TYPE_STRING();
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
    Type* left_type = checker->last_type;
    ACCEPT_EXPR(checker, binary->right);
    Type* right_type = checker->last_type;

#define ERROR(msg) error(checker, &binary->op, "%s", msg);\
    printf(" for types: '");\
    type_print(left_type);\
    printf("' and '");\
    type_print(right_type);\
    printf("'\n")

    switch (binary->op.kind) {
    case TOKEN_PLUS: {
        if (TYPE_IS_STRING(left_type) && TYPE_IS_STRING(right_type)) {
            checker->last_type = CREATE_TYPE_STRING();
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
        if (TYPE_IS_NUMBER(left_type) && TYPE_IS_NUMBER(right_type)) {
            checker->last_type = CREATE_TYPE_NUMBER();
            return;
        }
        ERROR("Invalid types for numeric operation");
        return;
    }
    case TOKEN_AND:
    case TOKEN_OR: {
        if (TYPE_IS_BOOL(left_type) && TYPE_IS_BOOL(right_type)) {
            checker->last_type = CREATE_TYPE_BOOL();
            return;
        }
        ERROR("Invalid types for boolean operation");
        return;
    }
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL: {
        if (type_equals(left_type, right_type)) {
            checker->last_type = CREATE_TYPE_BOOL();
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
    Type* inner_type = checker->last_type;

#define ERROR(msg) error(checker, &unary->op, "%s", msg);\
    printf(" for type: '");\
    type_print(inner_type);\
    printf("'\n")

    switch (unary->op.kind) {
    case TOKEN_BANG: {
        if (TYPE_IS_BOOL(inner_type)) {
            checker->last_type = CREATE_TYPE_BOOL();
            return;
        }
        ERROR("Invalid type for not operation");
        return;
    }
    case TOKEN_PLUS:
    case TOKEN_MINUS: {
        if (TYPE_IS_NUMBER(inner_type)) {
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
