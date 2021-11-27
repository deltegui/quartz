#include "typechecker.h"
#include <stdarg.h>
#include "type.h"
#include "common.h"
#include "lexer.h"
#include "expr.h"
#include "symbol.h"
#include "error.h"

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

    bool is_defining_variable;
    Symbol* defining_variable;

    const char* source;
} Typechecker;

#define TYPECHECK_IS_GLOBAL_FN(checker) (checker->function_stack.size == 0)

static bool function_returns(Stmt* fn_ast);

static void function_stack_pop(Typechecker* const checker);
static FuncMeta* function_stack_peek(Typechecker* const checker);
static void function_stack_push(Typechecker* const checker, Token fn_token);
static void function_stack_start_scope(Typechecker* const checker);
static void function_stack_end_scope(Typechecker* const checker);
static void error_last_type_match(Typechecker* const checker, Token* where, Type* first, const char* message);
static void error_ctx(Typechecker* const checker, Token* token);
static void error_param_number(Typechecker* const checker, Token* token, Type* type, Type* actual_type, int param_num);
static void error(Typechecker* const checker, Token* token, const char* message, ...);
static void have_error(Typechecker* const checker);

static void start_scope(Typechecker* const checker);
static void end_scope(Typechecker* const checker);
static Symbol* lookup_str(Typechecker* const checker, const char* name, int length);
static Symbol* lookup_levels(Typechecker* const checker, SymbolName name, int level);
static void typecheck_params_arent_void(Typechecker* const checker, Symbol* symbol);
static void check_and_mark_upvalue(Typechecker* const checker, Symbol* var);
static bool var_is_current_function_local(Typechecker* const checker, Symbol* var);

static void start_variable_definition(Typechecker* const checker, Symbol* var);
static void end_variable_definition(Typechecker* const checker);

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

static void typecheck_typealias(void* ctx, TypealiasStmt* alias);
static void typecheck_loopg(void* ctx, LoopGotoStmt* loopg);
static void typecheck_expr(void* ctx, ExprStmt* expr);
static void typecheck_var(void* ctx, VarStmt* var);
static void typecheck_block(void* ctx, BlockStmt* block);
static void typecheck_function(void* ctx, FunctionStmt* function);
static void typecheck_return(void* ctx, ReturnStmt* function);
static void typecheck_if(void* ctx, IfStmt* if_);
static void typecheck_for(void* ctx, ForStmt* for_);
static void typecheck_while(void* ctx, WhileStmt* while_);
static void typecheck_import(void* ctx, ImportStmt* import);
static void typecheck_native(void* ctx, NativeFunctionStmt* native);
static void typecheck_class(void* ctx, ClassStmt* klass);

StmtVisitor typechecker_stmt_visitor = (StmtVisitor){
    .visit_expr = typecheck_expr,
    .visit_var = typecheck_var,
    .visit_block = typecheck_block,
    .visit_function = typecheck_function,
    .visit_return = typecheck_return,
    .visit_if = typecheck_if,
    .visit_for = typecheck_for,
    .visit_while = typecheck_while,
    .visit_loopg = typecheck_loopg,
    .visit_typealias = typecheck_typealias,
    .visit_import = typecheck_import,
    .visit_native = typecheck_native,
    .visit_class = typecheck_class,
};

#define ACCEPT_STMT(typechecker, stmt) stmt_dispatch(&typechecker_stmt_visitor, typechecker, stmt)
#define ACCEPT_EXPR(typechecker, expr) expr_dispatch(&typechecker_expr_visitor, typechecker, expr)

static void function_stack_pop(Typechecker* const checker) {
    assert(checker->function_stack.size > 0);
    checker->function_stack.size--;
}

static FuncMeta* function_stack_peek(Typechecker* const checker) {
    assert(checker->function_stack.size > 0);
    FuncMeta* meta = VECTOR_AS_FUNC_META(&checker->function_stack);
    return &meta[checker->function_stack.size - 1];
}

static void function_stack_push(Typechecker* const checker, Token fn_token) {
    FuncMeta meta;
    meta.name = fn_token;
    meta.scope_distance = 0;
    VECTOR_ADD_FUNC_META(&checker->function_stack, meta);
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
    have_error(checker);
    fprintf(stderr, "[Line %d] Type error: The Type '", where->line);
    ERR_TYPE_PRINT(first);
    fprintf(stderr, "' does not match with type '");
    ERR_TYPE_PRINT(last_type);
    fprintf(stderr, "' %s\n", message);
    error_ctx(checker, where);
}

static void error_ctx(Typechecker* const checker, Token* token) {
    print_error_context(checker->source, token);
}

static void error_param_number(Typechecker* const checker, Token* token, Type* type, Type* actual_type, int param_num) {
    have_error(checker);
    fprintf(stderr, "[Line %d] Type error: ",token->line);
    fprintf(stderr, "Type of param number %d in function call (", param_num);
    ERR_TYPE_PRINT(type);
    fprintf(stderr, ") does not match with function definition (");
    ERR_TYPE_PRINT(actual_type);
    fprintf(stderr, ")\n");
    error_ctx(checker, token);
}

static void error(Typechecker* const checker, Token* token, const char* message, ...) {
    have_error(checker);
    va_list params;
    va_start(params, message);
    fprintf(stderr, "[Line %d] Type error: ",token->line);
    vfprintf(stderr, message, params);
    va_end(params);
    error_ctx(checker, token);
}

static void have_error(Typechecker* const checker) {
    checker->has_error = true;
    checker->last_type = CREATE_TYPE_UNKNOWN();
}

static void start_scope(Typechecker* const checker) {
    symbol_start_scope(checker->symbols);
    function_stack_start_scope(checker);
}

static void end_scope(Typechecker* const checker) {
    symbol_end_scope(checker->symbols);
    function_stack_end_scope(checker);
}

static Symbol* lookup_str(Typechecker* const checker, const char* name, int length) {
    return scoped_symbol_lookup_str(checker->symbols, name, length);
}

static Symbol* lookup_levels(Typechecker* const checker, SymbolName name, int level) {
    return scoped_symbol_lookup_levels(checker->symbols, &name, level);
}

bool typecheck(const char* source, Stmt* ast, ScopedSymbolTable* symbols) {
    Typechecker checker;
    checker.symbols = symbols;
    checker.has_error = false;
    checker.is_defining_variable = false;
    checker.defining_variable = NULL;
    checker.source = source;
    init_vector(&checker.function_stack, sizeof(FuncMeta));
    symbol_reset_scopes(checker.symbols);
    ACCEPT_STMT(&checker, ast);
    free_vector(&checker.function_stack);
    return !checker.has_error;
}

static void typecheck_loopg(void* ctx, LoopGotoStmt* loopg) {
}

static void typecheck_typealias(void* ctx, TypealiasStmt* alias) {
}

static void typecheck_block(void* ctx, BlockStmt* block) {
    Typechecker* checker = (Typechecker*) ctx;
    start_scope(checker);
    ACCEPT_STMT(ctx, block->stmts);
    end_scope(checker);
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

    start_variable_definition(checker, symbol);
    ACCEPT_EXPR(ctx, var->definition);
    end_variable_definition(checker);

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
    if (checker->is_defining_variable && symbol == checker->defining_variable) {
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

    Type* type = RESOLVE_IF_TYPEALIAS(symbol->type);

    if (! TYPE_IS_FUNCTION(type)) {
        error(
            checker,
            &call->identifier,
            "Calling '%.*s' which is not a function\n",
            call->identifier.length,
            call->identifier.start);
        return;
    }

    uint32_t fn_param_type_size = TYPE_FN_PARAMS(type).size;
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
    Type** param_types = VECTOR_AS_TYPES(&TYPE_FN_PARAMS(type));
    assert(call->params.size == TYPE_FN_PARAMS(type).size);

    for (uint32_t i = 0; i < call->params.size; i++) {
        ACCEPT_EXPR(checker, exprs[i]);
        Type* def_type = param_types[i];
        Type* last = checker->last_type;
        if (! type_equals(last, def_type)) {
            error_param_number(
                checker,
                &call->identifier,
                last,
                def_type,
                i);
        }
    }

    check_and_mark_upvalue(checker, symbol);

    checker->last_type = TYPE_FN_RETURN(type);
}

static void check_and_mark_upvalue(Typechecker* const checker, Symbol* var) {
    assert(var->kind == SYMBOL_FUNCTION || var->kind == SYMBOL_VAR);

#ifdef TYPECHECKER_DEBUG
    printf(
        "[TYPECHECKER DEBUG] Checking variable '%.*s'\n[TYPECHECKER DEBUG] Upvalue? ",
        SYMBOL_NAME_LENGTH(var->name),
        SYMBOL_NAME_START(var->name));
#endif
    if (TYPECHECK_IS_GLOBAL_FN(checker)) {
    // if (var->global) {
#ifdef TYPECHECKER_DEBUG
        printf("No, it's in global.\n");
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
    // We use scope_distance - 1 because we count how many scopes we have until we reach
    // the closest function. But lookup_levels thinks that a scope_distance of 0 is just
    // search in the current scope, a scope_distance of 1 is current scope and one above
    // and so on.
    Symbol* var_sym = lookup_levels(checker, var->name, meta->scope_distance - 1);
    return var_sym != NULL;
}

static void start_variable_definition(Typechecker* const checker, Symbol* var) {
    checker->is_defining_variable = true;
    checker->defining_variable = var;
}

static void end_variable_definition(Typechecker* const checker) {
    checker->is_defining_variable = false;
    checker->defining_variable = NULL;
}

static void typecheck_function(void* ctx, FunctionStmt* function) {
    Typechecker* checker = (Typechecker*) ctx;
    function_stack_push(checker, function->identifier);

    start_scope(checker);
    ACCEPT_STMT(ctx, function->body);
    end_scope(checker);

    Symbol* symbol = lookup_str(checker, function->identifier.start, function->identifier.length);
    assert(symbol != NULL);
    assert(symbol->kind == SYMBOL_FUNCTION);
    typecheck_params_arent_void(checker, symbol);

    function_stack_pop(checker);

    checker->last_type = TYPE_FN_RETURN(symbol->type);

#define FN_RETURNS_SOMETHING() !(TYPE_IS_NIL(checker->last_type) || TYPE_IS_VOID(checker->last_type))
    if (FN_RETURNS_SOMETHING()) {
        if (!function_returns(function->body)) {
            error(
                checker,
                &function->identifier,
                "Missing return at the end of function body\n");
        }
    }
#undef FN_RETURNS_SOMETHING
}

static void typecheck_native(void* ctx, NativeFunctionStmt* native) {
}

static void typecheck_class(void* ctx, ClassStmt* klass) {
    Typechecker* checker = (Typechecker*) ctx;

    Symbol* class_sym = lookup_str(checker, klass->identifier.start, klass->identifier.length);
    assert(class_sym != NULL);
    assert(class_sym->kind == SYMBOL_OBJECT);

    ScopedSymbolTable* prev = checker->symbols;
    checker->symbols = class_sym->object.symbols;
    ACCEPT_STMT(ctx, klass->body);
    checker->symbols = prev;
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

static void typecheck_if(void* ctx, IfStmt* if_) {
    Typechecker* checker = (Typechecker*) ctx;
    ACCEPT_EXPR(ctx, if_->condition);
    if (! TYPE_IS_BOOL(checker->last_type)) {
        error_last_type_match(
            checker,
            &if_->token,
            CREATE_TYPE_BOOL(),
            "in if condition. The condition must evaluate to Bool.");
    }
    ACCEPT_STMT(ctx, if_->then);
    ACCEPT_STMT(ctx, if_->else_);
}

static void typecheck_for(void* ctx, ForStmt* for_) {
    Typechecker* checker = (Typechecker*) ctx;
    start_scope(checker);

    ACCEPT_STMT(checker, for_->init);
    ACCEPT_EXPR(checker, for_->condition);
    if (for_->condition != NULL && !TYPE_IS_BOOL(checker->last_type)) {
        error_last_type_match(
            checker,
            &for_->token,
            CREATE_TYPE_BOOL(),
            "in for condition. The condition must evaluate to Bool.");
    }
    ACCEPT_STMT(checker, for_->mod);
    ACCEPT_STMT(checker, for_->body);

    end_scope(checker);
}

static void typecheck_while(void* ctx, WhileStmt* while_) {
    Typechecker* checker = (Typechecker*) ctx;

    ACCEPT_EXPR(checker, while_->condition);
    if (!TYPE_IS_BOOL(checker->last_type)) {
        error_last_type_match(
            checker,
            &while_->token,
            CREATE_TYPE_BOOL(),
            "in while condition. The condition must evaluate to Bool.");
    }
    ACCEPT_STMT(checker, while_->body);
}

static void typecheck_import(void* ctx, ImportStmt* import) {
    ACCEPT_STMT(ctx, import->ast);
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

#define ERROR(msg) have_error(checker);\
    fprintf(stderr, "[Line %d] Type error: %s for types '", binary->op.line, msg);\
    ERR_TYPE_PRINT(left_type);\
    fprintf(stderr, "' and '");\
    ERR_TYPE_PRINT(right_type);\
    fprintf(stderr, "'\n");\
    error_ctx(checker, &binary->op)

    switch (binary->op.kind) {
    case TOKEN_PLUS: {
        if (TYPE_IS_STRING(left_type) && TYPE_IS_STRING(right_type)) {
            checker->last_type = CREATE_TYPE_STRING();
            return;
        }
        // just continue to TYPE_NUMBER
    }
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
    case TOKEN_LOWER:
    case TOKEN_LOWER_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL: {
        if (TYPE_IS_NUMBER(left_type) && TYPE_IS_NUMBER(right_type)) {
            checker->last_type = CREATE_TYPE_BOOL();
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

#define ERROR(msg) have_error(checker);\
    fprintf(stderr, "[Line %d] Type error: %s for type '", unary->op.line, msg);\
    ERR_TYPE_PRINT(inner_type);\
    fprintf(stderr, "'\n");\
    error_ctx(checker, &unary->op)

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

// TODO This part of code is used to ensure that mandatory returns are
// present in functions. Maybe would be better to use a CFG (Control Flow Graph)
// to detect missing returns or even realize that there is no need to add a return
// at the bottom of a function.

static void check_return_loopg(void* ctx, LoopGotoStmt* loopg);
static void check_return_expr(void* ctx, ExprStmt* expr);
static void check_return_var(void* ctx, VarStmt* var);
static void check_return_block(void* ctx, BlockStmt* block);
static void check_return_function(void* ctx, FunctionStmt* function);
static void check_return_return(void* ctx, ReturnStmt* function);
static void check_return_if(void* ctx, IfStmt* if_);
static void check_return_for(void* ctx, ForStmt* for_);
static void check_return_while(void* ctx, WhileStmt* while_);

StmtVisitor check_return_stmt_visitor = (StmtVisitor){
    .visit_expr = check_return_expr,
    .visit_var = check_return_var,
    .visit_block = check_return_block,
    .visit_function = check_return_function,
    .visit_return = check_return_return,
    .visit_if = check_return_if,
    .visit_for = check_return_for,
    .visit_while = check_return_while,
    .visit_loopg = check_return_loopg,
};

typedef struct {
    bool have_return;
} ReturnChecker;

static bool function_returns(Stmt* fn_ast) {
    ReturnChecker checker;
    checker.have_return = false;
    stmt_dispatch(&check_return_stmt_visitor, &checker, fn_ast);
    return checker.have_return;
}

static void check_return_loopg(void* ctx, LoopGotoStmt* loopg) {}
static void check_return_expr(void* ctx, ExprStmt* expr) {}
static void check_return_var(void* ctx, VarStmt* var) {}
static void check_return_function(void* ctx, FunctionStmt* function) {}
static void check_return_if(void* ctx, IfStmt* if_) {}
static void check_return_for(void* ctx, ForStmt* for_) {}
static void check_return_while(void* ctx, WhileStmt* while_) {}

static void check_return_block(void* ctx, BlockStmt* block) {
    // Function body can be a single stmt or a Block stmt.
    stmt_dispatch(&check_return_stmt_visitor, ctx, block->stmts);
}

static void check_return_return(void* ctx, ReturnStmt* function) {
    ReturnChecker* checker = (ReturnChecker*) ctx;
    checker->have_return = true;
}

