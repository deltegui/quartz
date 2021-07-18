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
    int next_local_index;
} Compiler;

struct IdentifierOps {
    uint8_t op_global;
    uint8_t op_global_long;
    uint8_t op_local;
};

static void error(Compiler* compiler, const char* message);

static void init_compiler(Compiler* compiler, CompilerMode mode, const char* source);
static void free_compiler(Compiler* compiler);
static void init_inner_compiler(Compiler* inner, Compiler* outer, Token* fn_identifier);

static Chunk* current_chunk(Compiler* compiler);

static void start_scope(Compiler* compiler);
static void end_scope(Compiler* compiler);
static Symbol* lookup_str(Compiler* compiler, const char* name, int length);

static void emit(Compiler* compiler, uint8_t bytecode);
static void emit_short(Compiler* compiler, uint8_t bytecode, uint8_t param);
static void emit_long(Compiler* compiler, uint8_t bytecode, uint16_t param);
static void emit_param(Compiler* compiler, uint8_t op_short, uint8_t op_long,  uint16_t param);

static uint16_t make_constant(Compiler* compiler, Value value);
static uint16_t identifier_constant(Compiler* compiler, Token* identifier);

static void update_param_index(Compiler* compiler, Symbol* symbol);
static uint16_t get_variable_index(Compiler* compiler, Token* identifier);
static void emit_variable_declaration(Compiler* compiler, uint16_t index);
static void identifier_use(Compiler* compiler, Token identifier, struct IdentifierOps* ops);

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

static void error(Compiler* compiler, const char* message) {
    fprintf(stderr, "[Line %d] Compile error: %s\n", compiler->last_line, message);
    compiler->has_error = true;
}

static void init_compiler(Compiler* compiler, CompilerMode mode, const char* source) {
    init_scoped_symbol_table(&compiler->symbols);
    init_parser(&compiler->parser, source, &compiler->symbols);

    compiler->func = new_function("<GLOBAL>", 8);
    compiler->mode = mode;

    compiler->last_line = 1;
    compiler->has_error = false;

    compiler->scope_depth = 0;
    compiler->next_local_index = 1; // Is expected to always have GLOBAL in pos 0
    memset(compiler->locals, 0, UINT8_COUNT);
}

static void free_compiler(Compiler* compiler) {
    free_scoped_symbol_table(&compiler->symbols);
}

static void init_inner_compiler(Compiler* inner, Compiler* outer, Token* fn_identifier) {
    inner->parser = outer->parser;
    inner->symbols = outer->symbols;
    inner->func = new_function(fn_identifier->start, fn_identifier->length);
    inner->last_line = outer->last_line;
    inner->has_error = false;
    inner->scope_depth = outer->scope_depth;
    inner->next_local_index = 1; // We expect to always have a function in pos 0
    memset(inner->locals, 0, UINT8_COUNT);
}

static Chunk* current_chunk(Compiler* compiler) {
    return &compiler->func->chunk;
}

CompilationResult compile(const char* source, ObjFunction** result) {
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

static void start_scope(Compiler* compiler) {
    compiler->scope_depth++;
    symbol_start_scope(&compiler->symbols);
}

static void end_scope(Compiler* compiler) {
    compiler->next_local_index -= compiler->locals[compiler->scope_depth];
    while (compiler->locals[compiler->scope_depth] > 0) {
        emit(compiler, OP_POP);
        compiler->locals[compiler->scope_depth]--;
    }
    compiler->scope_depth--;
    symbol_end_scope(&compiler->symbols);
}

static Symbol* lookup_str(Compiler* compiler, const char* name, int length) {
    return scoped_symbol_lookup_str(&compiler->symbols, name, length);
}

static void emit(Compiler* compiler, uint8_t bytecode) {
    chunk_write(current_chunk(compiler), bytecode, compiler->last_line);
}

static void emit_short(Compiler* compiler, uint8_t bytecode, uint8_t param) {
    emit(compiler, bytecode);
    emit(compiler, param);
}

static void emit_long(Compiler* compiler, uint8_t bytecode, uint16_t param) {
    uint8_t high = param >> 0x8;
    uint8_t low = param & 0x00FF;
    emit(compiler, bytecode);
    emit(compiler, high);
    emit(compiler, low);
}

static void emit_param(Compiler* compiler, uint8_t op_short, uint8_t op_long,  uint16_t param) {
    if (param > UINT8_MAX) {
        emit_long(compiler, op_long, param);
    } else {
        emit_short(compiler, op_short, param);
    }
}

static uint16_t make_constant(Compiler* compiler, Value value) {
    int constant_index = chunk_add_constant(current_chunk(compiler), value);
    if (constant_index > UINT16_COUNT) {
        error(compiler, "Too many constants for chunk!");
        return 0;
    }
    return (uint16_t)constant_index;
}

static uint16_t identifier_constant(Compiler* compiler, Token* identifier) {
    Value value = OBJ_VALUE(copy_string(identifier->start, identifier->length));
    value.type = TYPE_STRING;
    return make_constant(compiler, value);
}

static void compile_print(void* ctx, PrintStmt* print) {
    Compiler* compiler = (Compiler*)ctx;
    ACCEPT_EXPR(compiler, print->inner);
    emit(compiler, OP_PRINT);
}

static void compile_block(void* ctx, BlockStmt* block) {
    Compiler* compiler = (Compiler*)ctx;
    start_scope(compiler);
    if (compiler->scope_depth > UINT8_MAX) {
        error(compiler, "Too many scopes!");
        return;
    }
    ACCEPT_STMT(compiler, block->stmts);
    end_scope(compiler);
}

static void compile_expr(void* ctx, ExprStmt* expr) {
    Compiler* compiler = (Compiler*)ctx;
    ACCEPT_EXPR(compiler, expr->inner);
    emit(compiler, OP_POP);
}

static void compile_function(void* ctx, FunctionStmt* function) {
    // TODO there is many lines of code in common with compile_var
    Compiler* compiler = (Compiler*) ctx;
    uint16_t fn_index = get_variable_index(compiler, &function->identifier);

    Symbol* symbol = lookup_str(compiler, function->identifier.start, function->identifier.length);
    assert(symbol != NULL);
    symbol->global = compiler->scope_depth == 0;
    symbol->constant_index = fn_index;

    Compiler inner;
    init_inner_compiler(&inner, compiler, &function->identifier);
    start_scope(&inner);
    update_param_index(&inner, symbol);
    ACCEPT_STMT(&inner, function->body);
    end_scope(&inner);

    if (inner.has_error) {
        compiler->has_error = true;
    }

    Value fn_value = OBJ_VALUE(inner.func);
    fn_value.type = TYPE_FUNCTION; // TODO A better and less error prone way to give types at runtime?
    uint16_t default_value = make_constant(compiler, fn_value);
    emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, default_value);

    emit_variable_declaration(compiler, fn_index);
}

static void update_param_index(Compiler* compiler, Symbol* symbol) {
    for (int i = 0; i < symbol->function.params.size; i++) {
        Token param = symbol->function.params.params[i].identifier;
        Symbol* param_sym = lookup_str(compiler, param.start, param.length);
        param_sym->constant_index = compiler->next_local_index;
        compiler->next_local_index++;
    }
}

static void compile_return(void* ctx, ReturnStmt* return_) {
    Compiler* compiler = (Compiler*) ctx;
    ACCEPT_EXPR(compiler, return_->inner);
    emit(compiler, OP_RETURN);
}

static void compile_var(void* ctx, VarStmt* var) {
    Compiler* compiler = (Compiler*) ctx;
    uint16_t variable_index = get_variable_index(compiler, &var->identifier);

    Symbol* symbol = lookup_str(compiler, var->identifier.start, var->identifier.length);
    assert(symbol != NULL);
    symbol->global = compiler->scope_depth == 0;
    symbol->constant_index = variable_index;

    if (var->definition == NULL) {
        uint16_t default_value = make_constant(compiler, value_default(symbol->type));
        emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, default_value);
    } else {
        ACCEPT_EXPR(compiler, var->definition);
    }
    emit_variable_declaration(compiler, variable_index);
}

static uint16_t get_variable_index(Compiler* compiler, Token* identifier) {
    if (compiler->scope_depth == 0) {
        return identifier_constant(compiler, identifier);
    }
    uint16_t index = compiler->next_local_index;
    compiler->locals[compiler->scope_depth]++;
    compiler->next_local_index++;
    return index;
}

static void emit_variable_declaration(Compiler* compiler, uint16_t index) {
    if (compiler->scope_depth == 0) {
        emit_param(compiler, OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_LONG, index);
    }
}

struct IdentifierOps ops_get_identifier = (struct IdentifierOps) {
    .op_global = OP_GET_GLOBAL,
    .op_global_long = OP_GET_GLOBAL_LONG,
    .op_local = OP_GET_LOCAL,
};

struct IdentifierOps ops_set_identifier = (struct IdentifierOps) {
    .op_global = OP_SET_GLOBAL,
    .op_global_long = OP_SET_GLOBAL_LONG,
    .op_local = OP_SET_LOCAL,
};

static void compile_identifier(void* ctx, IdentifierExpr* identifier) {
    Compiler* compiler = (Compiler*) ctx;
    identifier_use(compiler, identifier->name, &ops_get_identifier);
}

static void compile_assignment(void* ctx, AssignmentExpr* assignment) {
    Compiler* compiler = (Compiler*) ctx;
    ACCEPT_EXPR(compiler, assignment->value);
    identifier_use(compiler, assignment->name, &ops_set_identifier);
}

static void identifier_use(Compiler* compiler, Token identifier, struct IdentifierOps* ops) {
    Symbol* symbol = lookup_str(compiler, identifier.start, identifier.length);
    assert(symbol != NULL);
    assert(symbol->constant_index < UINT16_MAX);
#ifdef COMPILER_DEBUG
    token_print(identifier);
    printf("Index %d\n", symbol->constant_index);
#endif
    if (symbol->global) {
        emit_param(compiler, ops->op_global, ops->op_global_long, symbol->constant_index);
    } else {
        assert(symbol->constant_index < UINT8_MAX);
        emit_short(compiler, ops->op_local, symbol->constant_index);
    }
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
        value = OBJ_VALUE(str);
        value.type = TYPE_STRING;
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
    int i = 0;
    for (; i < call->params.size; i++) {
        ACCEPT_EXPR(compiler, call->params.params[i].expr);
    }
    if (i > UINT8_MAX) {
        error(compiler, "Parameter count exceeds the max number of parameters: 254");
    }
    emit_short(compiler, OP_CALL, i);
}
