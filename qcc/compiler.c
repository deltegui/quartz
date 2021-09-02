#include "compiler.h"
#include <string.h> // for memset
#include "typechecker.h"
#include "values.h"
#include "symbol.h" // to initialize and free

#ifdef COMPILER_DEBUG
#include "debug.h"
#endif

typedef enum {
    MODE_SCRIPT,
    MODE_FUNCTION,
} CompilerMode;

typedef struct {
    Parser parser;
    ScopedSymbolTable symbols;

    ObjFunction* func;
    CompilerMode mode;

    int last_line;
    bool has_error;

    int locals[UINT8_COUNT];
    int scope_depth;
    int function_scope_depth;
    int next_local_index;
} Compiler;

struct IdentifierOps {
    uint8_t op_global;
    uint8_t op_global_long;
    uint8_t op_local;
    uint8_t op_upvalue;
};

static void error(Compiler* const compiler, const char* message);

static void init_compiler(Compiler* const compiler, CompilerMode mode, const char* source);
static void free_compiler(Compiler* const compiler);
static void init_inner_compiler(Compiler* const inner, Compiler* const outer, const Token* fn_identifier, int upvalue_size);

static Chunk* current_chunk(Compiler* const compiler);

static void start_scope(Compiler* const compiler);
static void end_scope(Compiler* const compiler);
static Symbol* lookup_str(Compiler* const compiler, const char* name, int length);

static void emit(Compiler* const compiler, uint8_t bytecode);
static void emit_short(Compiler* const compiler, uint8_t bytecode, uint8_t param);
static void emit_long(Compiler* const compiler, uint8_t bytecode, uint16_t param);
static void emit_param(Compiler* const compiler, uint8_t op_short, uint8_t op_long,  uint16_t param);
static void emit_bind_upvalues(Compiler* const compiler, Symbol* fn_sym, Token fn);
static bool last_emitted_byte_equals(Compiler* const compiler, uint8_t byte);
static void emit_closed_variables(Compiler* const compiler, int depth);
static void emit_close_stack_upvalue(Compiler* const compiler, Symbol* var_token);
static int get_upvalue_index_in_function(Compiler* const compiler, Symbol* var_token, Symbol* fn_ref);
static Token symbol_to_token_identifier(Symbol* symbol);

static uint16_t make_constant(Compiler* const compiler, Value value);
static uint16_t identifier_constant(Compiler* const compiler, const Token* identifier);
static void number_constant_use(Compiler* const compiler, int number);

static void update_param_index(Compiler* const compiler, Symbol* symbol);
static void update_symbol_variable_info(Compiler* const compiler, Symbol* var_sym, uint16_t var_index);
static uint16_t get_variable_index(Compiler* const compiler, const Token* identifier);
static void emit_variable_declaration(Compiler* const compiler, uint16_t index);
static void identifier_use_symbol(Compiler* const compiler, Symbol* sym, const struct IdentifierOps* ops);
static void identifier_use(Compiler* const compiler, Token identifier, const struct IdentifierOps* ops);
static void ensure_function_returns_value(Compiler* const compiler, Symbol* fn_sym);
static int get_current_function_upvalue_index(Compiler* const compiler, Symbol* var);

static void compile_assignment(void* ctx, AssignmentExpr* assignment);
static void compile_identifier(void* ctx, IdentifierExpr* identifier);
static void compile_literal(void* ctx, LiteralExpr* literal);
static void compile_binary(void* ctx, BinaryExpr* binary);
static void compile_unary(void* ctx, UnaryExpr* unary);
static void compile_call(void* ctx, CallExpr* call);

ExprVisitor compiler_expr_visitor = (ExprVisitor){
    .visit_literal = compile_literal,
    .visit_binary = compile_binary,
    .visit_unary = compile_unary,
    .visit_identifier = compile_identifier,
    .visit_assignment = compile_assignment,
    .visit_call = compile_call,
};

static void compile_expr(void* ctx, ExprStmt* expr);
static void compile_var(void* ctx, VarStmt* var);
static void compile_print(void* ctx, PrintStmt* print);
static void compile_block(void* ctx, BlockStmt* block);
static void compile_function(void* ctx, FunctionStmt* function);
static void compile_return(void* ctx, ReturnStmt* return_);

StmtVisitor compiler_stmt_visitor = (StmtVisitor){
    .visit_expr = compile_expr,
    .visit_var = compile_var,
    .visit_print = compile_print,
    .visit_block = compile_block,
    .visit_function = compile_function,
    .visit_return = compile_return,
};

#define ACCEPT_STMT(compiler, stmt) stmt_dispatch(&compiler_stmt_visitor, compiler, stmt)
#define ACCEPT_EXPR(compiler, expr) expr_dispatch(&compiler_expr_visitor, compiler, expr)

struct IdentifierOps ops_get_identifier = (struct IdentifierOps) {
    .op_global = OP_GET_GLOBAL,
    .op_global_long = OP_GET_GLOBAL_LONG,
    .op_local = OP_GET_LOCAL,
    .op_upvalue = OP_GET_UPVALUE,
};

struct IdentifierOps ops_set_identifier = (struct IdentifierOps) {
    .op_global = OP_SET_GLOBAL,
    .op_global_long = OP_SET_GLOBAL_LONG,
    .op_local = OP_SET_LOCAL,
    .op_upvalue = OP_SET_UPVALUE,
};

static void error(Compiler* const compiler, const char* message) {
    fprintf(stderr, "[Line %d] Compile error: %s\n", compiler->last_line, message);
    compiler->has_error = true;
}

static void init_compiler(Compiler* const compiler, CompilerMode mode, const char* source) {
    init_scoped_symbol_table(&compiler->symbols);
    init_parser(&compiler->parser, source, &compiler->symbols);

    compiler->func = new_function("<GLOBAL>", 8, 0);
    compiler->mode = mode;

    compiler->last_line = 1;
    compiler->has_error = false;

    compiler->scope_depth = 0;
    compiler->function_scope_depth = 0;
    compiler->next_local_index = 1; // Is expected to always have GLOBAL in pos 0
    memset(compiler->locals, 0, UINT8_COUNT);
}

static void free_compiler(Compiler* const compiler) {
    free_scoped_symbol_table(&compiler->symbols);
}

static void init_inner_compiler(Compiler* const inner, Compiler* const outer, const Token* fn_identifier, int upvalue_size) {
    inner->symbols = outer->symbols;
    inner->parser = outer->parser;

    inner->func = new_function(fn_identifier->start, fn_identifier->length, upvalue_size);
    inner->mode = MODE_FUNCTION;

    inner->last_line = outer->last_line;
    inner->has_error = false;

    inner->scope_depth = outer->scope_depth;
    inner->function_scope_depth = 0;
    inner->next_local_index = 1; // We expect to always have a function in pos 0
    memset(inner->locals, 0, UINT8_COUNT);
}

static Chunk* current_chunk(Compiler* const compiler) {
    return &compiler->func->chunk;
}

CompilationResult compile(const char* source, ObjFunction** const result) {
#define END_WITH(final) do {\
        free_compiler(&compiler);\
        free_stmt(ast);\
        *result = final;\
} while (false)

    Compiler compiler;
    init_compiler(&compiler, MODE_SCRIPT, source);
    Stmt* ast = parse(&compiler.parser);
    if (compiler.parser.has_error) {
        END_WITH(NULL);
        return PARSING_ERROR;
    }
    if (!typecheck(ast, &compiler.symbols)) {
        END_WITH(NULL);
        return TYPE_ERROR;
    }
    symbol_reset_scopes(&compiler.symbols);
    ACCEPT_STMT(&compiler, ast);
    emit(&compiler, OP_END);
#ifdef COMPILER_DEBUG
    scoped_symbol_table_print(&compiler.symbols);
    if (!compiler.has_error) {
        chunk_print(&compiler.func->chunk);
    }
#endif
    END_WITH(compiler.func);
    if (compiler.has_error) {
        return COMPILATION_ERROR;
    }
    return COMPILATION_OK;

#undef END_WITH
}

static void start_scope(Compiler* const compiler) {
    compiler->scope_depth++;
    symbol_start_scope(&compiler->symbols);
}

static void end_scope(Compiler* const compiler) {
    compiler->next_local_index -= compiler->locals[compiler->scope_depth];
    while (compiler->locals[compiler->scope_depth] > 0) {
        emit(compiler, OP_POP);
        compiler->locals[compiler->scope_depth]--;
    }
    compiler->scope_depth--;
    symbol_end_scope(&compiler->symbols);
}

static Symbol* lookup_str(Compiler* const compiler, const char* name, int length) {
    return scoped_symbol_lookup_str(&compiler->symbols, name, length);
}

static void emit(Compiler* const compiler, uint8_t bytecode) {
    chunk_write(current_chunk(compiler), bytecode, compiler->last_line);
}

static void emit_short(Compiler* const compiler, uint8_t bytecode, uint8_t param) {
    emit(compiler, bytecode);
    emit(compiler, param);
}

static void emit_long(Compiler* const compiler, uint8_t bytecode, uint16_t param) {
    uint8_t high = param >> 0x8;
    uint8_t low = param & 0x00FF;
    emit(compiler, bytecode);
    emit(compiler, high);
    emit(compiler, low);
}

static void emit_param(Compiler* const compiler, uint8_t op_short, uint8_t op_long,  uint16_t param) {
    if (param > UINT8_MAX) {
        emit_long(compiler, op_long, param);
    } else {
        emit_short(compiler, op_short, param);
    }
}

static bool last_emitted_byte_equals(Compiler* const compiler, uint8_t byte) {
    return chunk_check_last_byte(current_chunk(compiler), byte);
}

static uint16_t make_constant(Compiler* const compiler, Value value) {
    int constant_index = chunk_add_constant(current_chunk(compiler), value);
    if (constant_index > UINT16_COUNT) {
        error(compiler, "Too many constants for chunk!");
        return 0;
    }
    return (uint16_t)constant_index;
}

static uint16_t identifier_constant(Compiler* const compiler, const Token* identifier) {
    Value value = OBJ_VALUE(
        copy_string(identifier->start, identifier->length),
        CREATE_TYPE_STRING());
    return make_constant(compiler, value);
}

static void number_constant_use(Compiler* const compiler, int number) {
    uint16_t upvalue_index = make_constant(compiler, NUMBER_VALUE(number));
    emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, upvalue_index);
}

static void compile_print(void* ctx, PrintStmt* print) {
    Compiler* compiler = (Compiler*)ctx;
    ACCEPT_EXPR(compiler, print->inner);
    emit(compiler, OP_PRINT);
}

static void compile_block(void* ctx, BlockStmt* block) {
    Compiler* compiler = (Compiler*)ctx;

    start_scope(compiler);
    compiler->function_scope_depth++;

    if (compiler->scope_depth > UINT8_MAX) {
        error(compiler, "Too many scopes!");
        return;
    }
    ACCEPT_STMT(compiler, block->stmts);

    emit_closed_variables(compiler, 0);

    compiler->function_scope_depth--;
    end_scope(compiler);
}

static void emit_closed_variables(Compiler* const compiler, int depth) {
    UpvalueIterator it;
    init_upvalue_iterator(&it, &compiler->symbols, depth);
    for (;;) {
        Symbol* var_sym = upvalue_iterator_next(&it);
        if (var_sym == NULL) {
            break;
        }
        emit_close_stack_upvalue(compiler, var_sym);
        Symbol** refs = SYMBOL_SET_GET_ELEMENTS(var_sym->upvalue_fn_refs);
        int size = SYMBOL_SET_SIZE(var_sym->upvalue_fn_refs);
        for (uint32_t i = 0; i < size; i++) {
            int index = get_upvalue_index_in_function(compiler, var_sym, refs[i]);
            number_constant_use(compiler, index);
            identifier_use_symbol(compiler, refs[i], &ops_get_identifier);
            emit(compiler, OP_BIND_CLOSED);
        }
        emit(compiler, OP_POP);
    }
}

static void emit_close_stack_upvalue(Compiler* const compiler, Symbol* var_sym) {
    identifier_use_symbol(compiler, var_sym, &ops_get_identifier);
    emit(compiler, OP_CLOSE);
}

static int get_upvalue_index_in_function(Compiler* const compiler, Symbol* var_name, Symbol* fn_ref) {
    // TODO check that this is not needed
    // Symbol* fn_sym = lookup(compiler, fn_ref->name);
    // assert(fn_sym != NULL);
    // assert(fn_sym->kind == SYMBOL_FUNCTION);
    return symbol_get_function_upvalue_index(fn_ref, var_name);
}

static Token symbol_to_token_identifier(Symbol* symbol) {
    return (Token){
        .kind = TOKEN_IDENTIFIER,
        .start = SYMBOL_NAME_START(symbol->name),
        .length = SYMBOL_NAME_LENGTH(symbol->name),
        .line = symbol->declaration_line,
    };
}

static void compile_expr(void* ctx, ExprStmt* expr) {
    Compiler* compiler = (Compiler*)ctx;
    ACCEPT_EXPR(compiler, expr->inner);
    emit(compiler, OP_POP);
}

static void compile_function(void* ctx, FunctionStmt* function) {
    Compiler* compiler = (Compiler*) ctx;
    uint16_t fn_index = get_variable_index(compiler, &function->identifier);

    Symbol* symbol = lookup_str(compiler, function->identifier.start, function->identifier.length);
    assert(symbol != NULL);
    update_symbol_variable_info(compiler, symbol, fn_index);

    Compiler inner;
    init_inner_compiler(&inner, compiler, &function->identifier, SYMBOL_GET_FUNCTION_UPVALUE_SIZE(symbol));
    start_scope(&inner);
    update_param_index(&inner, symbol);
    ACCEPT_STMT(&inner, function->body);
    ensure_function_returns_value(&inner, symbol);
    end_scope(&inner);

    if (inner.has_error) {
        compiler->has_error = true;
    }

    Value fn_value = OBJ_VALUE(inner.func, symbol->type);
    uint16_t default_value = make_constant(compiler, fn_value);
    emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, default_value);

    emit_variable_declaration(compiler, fn_index);

    emit_bind_upvalues(compiler, symbol, function->identifier);
}

static void emit_bind_upvalues(Compiler* const compiler, Symbol* fn_sym, Token fn) {
    Symbol** upvalues = SYMBOL_SET_GET_ELEMENTS(fn_sym->function.upvalues);
    int size = SYMBOL_SET_SIZE(fn_sym->function.upvalues);
    for (int i = 0; i < size; i++) {
        // TODO this shouldn`t be necessary
        // Symbol* upvalue_sym = lookup_str(compiler, upvalues[i].start, upvalues[i].length);
        // assert(upvalue_sym != NULL);
        Symbol* upvalue_sym = upvalues[i];
        number_constant_use(compiler, upvalue_sym->constant_index);
        number_constant_use(compiler, i);
        identifier_use(compiler, fn, &ops_get_identifier);
        emit(compiler, OP_BIND_UPVALUE);
    }
}

static void ensure_function_returns_value(Compiler* const compiler, Symbol* fn_sym) {
    if (last_emitted_byte_equals(compiler, OP_RETURN)) {
        return;
    }
    if (TYPE_IS_VOID(TYPE_FN_RETURN(fn_sym->type))) {
        emit(compiler, OP_NIL);
        emit(compiler, OP_RETURN);
    }
}

static void update_param_index(Compiler* const compiler, Symbol* symbol) {
    Token* param_names = VECTOR_AS_TOKENS(&symbol->function.param_names);
    for (uint32_t i = 0; i < symbol->function.param_names.size; i++) {
        Token param = param_names[i];
        Symbol* param_sym = lookup_str(compiler, param.start, param.length);
        param_sym->constant_index = compiler->next_local_index;
        compiler->next_local_index++;
    }
}

static void compile_return(void* ctx, ReturnStmt* return_) {
    Compiler* compiler = (Compiler*) ctx;

    emit_closed_variables(compiler, compiler->function_scope_depth);

    if (return_->inner != NULL) {
        ACCEPT_EXPR(compiler, return_->inner);
    } else {
        emit(compiler, OP_NIL);
    }
    emit(compiler, OP_RETURN);
}

static void compile_var(void* ctx, VarStmt* var) {
    Compiler* compiler = (Compiler*) ctx;
    uint16_t variable_index = get_variable_index(compiler, &var->identifier);

    Symbol* symbol = lookup_str(compiler, var->identifier.start, var->identifier.length);
    assert(symbol != NULL);
    update_symbol_variable_info(compiler, symbol, variable_index);

    if (var->definition == NULL) {
        uint16_t default_value = make_constant(compiler, value_default(symbol->type));
        emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, default_value);
    } else {
        ACCEPT_EXPR(compiler, var->definition);
    }
    emit_variable_declaration(compiler, variable_index);
}

static void update_symbol_variable_info(Compiler* const compiler, Symbol* var_sym, uint16_t var_index) {
    var_sym->global = compiler->scope_depth == 0;
    var_sym->constant_index = var_index;
}

static uint16_t get_variable_index(Compiler* const compiler, const Token* identifier) {
    if (compiler->scope_depth == 0) {
        return identifier_constant(compiler, identifier);
    }
    uint16_t index = compiler->next_local_index;
    compiler->locals[compiler->scope_depth]++;
    compiler->next_local_index++;
    return index;
}

static void emit_variable_declaration(Compiler* const compiler, uint16_t index) {
    if (compiler->scope_depth == 0) {
        emit_param(compiler, OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_LONG, index);
    }
}

static void compile_identifier(void* ctx, IdentifierExpr* identifier) {
    Compiler* compiler = (Compiler*) ctx;
    identifier_use(compiler, identifier->name, &ops_get_identifier);
}

static void compile_assignment(void* ctx, AssignmentExpr* assignment) {
    Compiler* compiler = (Compiler*) ctx;
    ACCEPT_EXPR(compiler, assignment->value);
    identifier_use(compiler, assignment->name, &ops_set_identifier);
}

static void identifier_use_symbol(Compiler* const compiler, Symbol* sym, const struct IdentifierOps* ops) {
    Token var_token = symbol_to_token_identifier(sym);
    identifier_use(compiler, var_token, ops);
}

static void identifier_use(Compiler* const compiler, Token identifier, const struct IdentifierOps* ops) {
    Symbol* symbol = lookup_str(compiler, identifier.start, identifier.length);
    assert(symbol != NULL);
    assert(symbol->constant_index < UINT16_MAX);
#ifdef COMPILER_DEBUG
    token_print(identifier);
    printf("Index %d\n", symbol->constant_index);
#endif
    if (symbol->global) {
        emit_param(compiler, ops->op_global, ops->op_global_long, symbol->constant_index);
        return;
    }
    int index = get_current_function_upvalue_index(compiler, symbol);
    if (index != -1) {
        emit_short(compiler, ops->op_upvalue, index);
        return;
    }
    assert(symbol->constant_index < UINT8_MAX);
    emit_short(compiler, ops->op_local, symbol->constant_index);
}

// static int symbol_is_closed(Compiler* const compiler, Symbol* symbol) {
//     Token fn_token = (Token){
//         .kind = TOKEN_IDENTIFIER,
//         .start = compiler->func->name->chars,
//         .length = compiler->func->name->length,
//         .line = 0, // doesnt matter. TODO if so, search for the symbol.
//     };
//     return symbol_is_closed(symbol, fn_token);
// }

static int get_current_function_upvalue_index(Compiler* const compiler, Symbol* var) {
    if (compiler->mode == MODE_SCRIPT) {
        return -1;
    }
    Symbol* fn_sym = lookup_str(
        compiler,
        compiler->func->name->chars,
        compiler->func->name->length);
    assert(fn_sym != NULL);
    return symbol_get_function_upvalue_index(fn_sym, var);
}

static void compile_literal(void* ctx, LiteralExpr* literal) {
    Compiler* compiler = (Compiler*) ctx;
    compiler->last_line = literal->literal.line;
    Value value;
    switch (literal->literal.kind) {
    case TOKEN_TRUE: {
        emit(compiler, OP_TRUE);
        return;
    }
    case TOKEN_FALSE: {
        emit(compiler, OP_FALSE);
        return;
    }
    case TOKEN_NIL: {
        emit(compiler, OP_NIL);
        return;
    }
    case TOKEN_NUMBER: {
        double d = (double) strtod(literal->literal.start, NULL);
        value = NUMBER_VALUE(d);
        break;
    }
    case TOKEN_STRING: {
        ObjString* str = copy_string(literal->literal.start, literal->literal.length);
        value = OBJ_VALUE(str, CREATE_TYPE_STRING());
        break;
    }
    default:
        error(compiler, "Unkown literal expression");
        return;
    }
    uint16_t value_pos = make_constant(compiler, value);
    emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, value_pos);
}

static void compile_binary(void* ctx, BinaryExpr* binary) {
    Compiler* compiler = (Compiler*) ctx;
    compiler->last_line = binary->op.line;

    ACCEPT_EXPR(compiler, binary->left);
    ACCEPT_EXPR(compiler, binary->right);

#define EMIT(byte) emit(compiler, byte)
#define EMIT_SHORT(first, second) emit_short(compiler, first, second)

    switch(binary->op.kind) {
    case TOKEN_PLUS: EMIT(OP_ADD); break;
    case TOKEN_MINUS: EMIT(OP_SUB); break;
    case TOKEN_STAR: EMIT(OP_MUL); break;
    case TOKEN_SLASH: EMIT(OP_DIV); break;
    case TOKEN_AND: EMIT(OP_AND); break;
    case TOKEN_OR: EMIT(OP_OR); break;
    case TOKEN_PERCENT: EMIT(OP_MOD); break;
    case TOKEN_EQUAL_EQUAL: EMIT(OP_EQUAL); break;
    case TOKEN_BANG_EQUAL: EMIT_SHORT(OP_EQUAL, OP_NOT); break;
    case TOKEN_LOWER: EMIT(OP_LOWER); break;
    case TOKEN_LOWER_EQUAL: EMIT_SHORT(OP_GREATER, OP_NOT); break;
    case TOKEN_GREATER: EMIT(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: EMIT_SHORT(OP_LOWER, OP_NOT); break;
    default:
        error(compiler, "Unkown binary operator in expression");
        return;
    }

#undef EMIT
#undef EMIT_BYTES
}

static void compile_unary(void* ctx, UnaryExpr* unary) {
    Compiler* compiler = (Compiler*) ctx;
    compiler->last_line = unary->op.line;

    OpCode op;
    switch(unary->op.kind) {
    case TOKEN_BANG:
        op = OP_NOT;
        break;
    case TOKEN_PLUS:
        op = OP_NOP;
        break;
    case TOKEN_MINUS:
        op = OP_NEGATE;
        break;
    default:
        error(compiler, "Unkown unary operator in expression");
        return;
    }
    ACCEPT_EXPR(compiler, unary->expr);
    emit(compiler, op);
}

static void compile_call(void* ctx, CallExpr* call) {
    Compiler* compiler = (Compiler*) ctx;
    identifier_use(compiler, call->identifier, &ops_get_identifier);
    Expr** exprs = VECTOR_AS_EXPRS(&call->params);
    uint32_t i = 0;
    for (; i < call->params.size; i++) {
        ACCEPT_EXPR(compiler, exprs[i]);
    }
    if (i > UINT8_MAX) {
        error(compiler, "Parameter count exceeds the max number of parameters: 254");
    }
    emit_short(compiler, OP_CALL, i);
}
