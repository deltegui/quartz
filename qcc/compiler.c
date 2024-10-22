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
    Vector loop_len; // Vector<int>
    Vector breaks; // Vector<int>
} BreakContext;

static BreakContext* create_break_ctx();
static void free_break_ctx(BreakContext* const break_ctx);
static int break_ctx_pop_loop(BreakContext* const break_ctx);
static int break_ctx_pop_break(BreakContext* const break_ctx);
static void break_ctx_push_loop(BreakContext* const break_ctx);
static void break_ctx_push_break(BreakContext* const break_ctx, int break_point);

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
    int loop_scope_depth;
    int next_local_index;

    bool is_in_loop;
    Symbol* current_self;

    // Vars needed to call a property
    bool want_to_call;
    int prop_index;

    bool in_assignment;

    BreakContext* break_ctx;
    int continue_ctx;
} Compiler;

#define BREAK_CTX_POP_LOOP(compiler) break_ctx_pop_loop(compiler->break_ctx);
#define BREAK_CTX_POP_BREAK(compiler) break_ctx_pop_break(compiler->break_ctx);
#define BREAK_CTX_PUSH_LOOP(compiler) break_ctx_push_loop(compiler->break_ctx);
#define BREAK_CTX_PUSH_BREAK(compiler, pos) break_ctx_push_break(compiler->break_ctx, pos);

#define CONTINUE_CTX_NOT_DEFINED -1
#define CONTINUE_CTX(compiler, pos, ...)\
    do {\
        int prev_continue = compiler->continue_ctx;\
        compiler->continue_ctx = pos;\
        __VA_ARGS__\
        compiler->continue_ctx = prev_continue;\
    } while (false)

#define LOOP(compiler, ...)\
    do {\
        int prev_loop_scope = compiler->loop_scope_depth;\
        compiler->loop_scope_depth = 0;\
        compiler->is_in_loop = true;\
        __VA_ARGS__\
        compiler->is_in_loop = false;\
        compiler->loop_scope_depth = prev_loop_scope;\
    } while (false)

#define HAVE_SELF(compiler) (compiler->current_self != NULL)
#define ENABLE_SELF(compiler, self_symbol, ...)\
    do {\
        compiler->current_self = symbol;\
        __VA_ARGS__\
        compiler->current_self = NULL;\
    } while(false)

#define NEXT_LOCAL_INDEX(compiler) (compiler->next_local_index++)

#define IN_ASSIGNMENT(compiler, ...)\
    do {\
        compiler->in_assignment = true;\
        __VA_ARGS__\
        compiler->in_assignment = false;\
    } while(false)

#define PROP_INDEX_NOT_DEFINED -1
#define WANT_TO_CALL(compiler, ...)\
    do {\
        int old_prop_index = compiler->prop_index;\
        bool old_want = compiler->want_to_call;\
        compiler->want_to_call = true;\
        __VA_ARGS__\
        compiler->want_to_call = old_want;\
        compiler->prop_index = old_prop_index;\
    } while(false)

struct IdentifierOps {
    uint8_t op_global;
    uint8_t op_global_long;
    uint8_t op_local;
    uint8_t op_upvalue;
};

static void error(Compiler* const compiler, const char* message);

static void init_compiler(Compiler* const compiler, CompilerMode mode, FileImport ctx);
static void free_compiler(Compiler* const compiler);
static void init_inner_compiler(Compiler* const inner, Compiler* const outer, const Token* fn_identifier, Symbol* fn_sym);

static Chunk* current_chunk(Compiler* const compiler);

static void start_scope(Compiler* const compiler);
static void end_scope(Compiler* const compiler);
static void pop_all_locals(Compiler* const compiler, int scope);
static void reset_loop_locals(Compiler* const compiler);
static Symbol* lookup_str(Compiler* const compiler, const char* name, int length);
static Symbol* lookup_with_class_str(Compiler* const compiler, const char* name, int length);
static Symbol* fn_lookup_str(Compiler* const compiler, const char* name, int length);

static int emit(Compiler* const compiler, uint8_t bytecode);
static int emit_short(Compiler* const compiler, uint8_t bytecode, uint8_t param);
static int emit_long(Compiler* const compiler, uint8_t bytecode, uint16_t param);
static int emit_param(Compiler* const compiler, uint8_t op_short, uint8_t op_long,  uint16_t param);
static void emit_bind_upvalues(Compiler* const compiler, Symbol* fn_sym, Token fn);
static bool last_emitted_byte_equals(Compiler* const compiler, uint8_t byte);
static void emit_closed_variables(Compiler* const compiler, int depth);
static void emit_close_stack_upvalue(Compiler* const compiler, Symbol* var_token);
static void patch_breaks(Compiler* const compiler);
static void patch_jump(Compiler* const compiler, int patch);
static int emit_jump_to(Compiler* const compiler, uint8_t jump_op, uint16_t to);
static void check_jump_distance(Compiler* const compiler, int distance);
static int get_upvalue_index_in_function(Compiler* const compiler, Symbol* var_token, Symbol* fn_ref);
static Token symbol_to_token_identifier(Symbol* symbol);
static void patch_chunk(Compiler* const compiler, int position, uint8_t value);
static void patch_chunk_long(Compiler* const compiler, int position, uint16_t value);

static uint16_t make_constant(Compiler* const compiler, Value value);
static uint16_t identifier_constant(Compiler* const compiler, const Token* identifier);
static uint8_t make_type(Compiler* const compiler, Type* type);

static void update_param_index(Compiler* const compiler, Symbol* symbol);
static void update_symbol_variable_info(Compiler* const compiler, Symbol* var_sym, uint16_t var_index);
static uint16_t get_variable_index(Compiler* const compiler, const Token* identifier);
static void emit_variable_declaration(Compiler* const compiler, uint16_t index);
static void identifier_use_symbol(Compiler* const compiler, Symbol* sym, const struct IdentifierOps* ops);
static void identifier_use(Compiler* const compiler, Token identifier, const struct IdentifierOps* ops);
static void ensure_function_returns_value(Compiler* const compiler, Symbol* fn_sym);
static int get_current_function_upvalue_index(Compiler* const compiler, Symbol* var);
static Value do_compile_function(Compiler* const compiler, FunctionStmt* function, uint16_t index);
static void preindex_class_props(Compiler* const compiler, ListStmt* body);
static Value compile_class_var_prop(Compiler* const compiler, VarStmt* var, uint16_t index);
static void call_with_params(Compiler* const compiler, Vector* params);

static void compile_assignment(void* ctx, AssignmentExpr* assignment);
static void compile_identifier(void* ctx, IdentifierExpr* identifier);
static void compile_literal(void* ctx, LiteralExpr* literal);
static void compile_binary(void* ctx, BinaryExpr* binary);
static void compile_unary(void* ctx, UnaryExpr* unary);
static void compile_call(void* ctx, CallExpr* call);
static void compile_new(void* ctx, NewExpr* new_);
static void compile_prop(void* ctx, PropExpr* prop);
static void compile_prop_assigment(void* ctx, PropAssigmentExpr* prop_assigment);
static void compile_array(void* ctx, ArrayExpr* arr);
static void compile_cast(void* ctx, CastExpr* cast);

ExprVisitor compiler_expr_visitor = (ExprVisitor){
    .visit_literal = compile_literal,
    .visit_binary = compile_binary,
    .visit_unary = compile_unary,
    .visit_identifier = compile_identifier,
    .visit_assignment = compile_assignment,
    .visit_call = compile_call,
    .visit_new = compile_new,
    .visit_prop = compile_prop,
    .visit_prop_assigment = compile_prop_assigment,
    .visit_array = compile_array,
    .visit_cast = compile_cast,
};

static void compile_expr(void* ctx, ExprStmt* expr);
static void compile_var(void* ctx, VarStmt* var);
static void compile_block(void* ctx, BlockStmt* block);
static void compile_function(void* ctx, FunctionStmt* function);
static void compile_return(void* ctx, ReturnStmt* return_);
static void compile_if(void* ctx, IfStmt* if_);
static void compile_for(void* ctx, ForStmt* for_);
static void compile_while(void* ctx, WhileStmt* while_);
static void compile_loopg(void* ctx, LoopGotoStmt* loopg);
static void compile_typealias(void* ctx, TypealiasStmt* alias);
static void compile_import(void* ctx, ImportStmt* import);
static void compile_native(void* ctx, NativeFunctionStmt* native);
static void compile_class(void* ctx, ClassStmt* klass);
static void compile_native_class(void* ctx, NativeClassStmt* native_class);

StmtVisitor compiler_stmt_visitor = (StmtVisitor){
    .visit_expr = compile_expr,
    .visit_var = compile_var,
    .visit_block = compile_block,
    .visit_function = compile_function,
    .visit_return = compile_return,
    .visit_if = compile_if,
    .visit_for = compile_for,
    .visit_while = compile_while,
    .visit_loopg = compile_loopg,
    .visit_typealias = compile_typealias,
    .visit_import = compile_import,
    .visit_native = compile_native,
    .visit_class = compile_class,
    .visit_native_class = compile_native_class,
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

#define VECTOR_AS_INT(vect) VECTOR_AS(vect, int)
#define VECTOR_ADD_INT(vect, i) VECTOR_ADD(vect, i, int)

static BreakContext* create_break_ctx() {
    BreakContext* break_ctx = (BreakContext*) malloc(sizeof(BreakContext));
    init_vector(&break_ctx->loop_len, sizeof(int));
    init_vector(&break_ctx->breaks, sizeof(int));
    return break_ctx;
}

static void free_break_ctx(BreakContext* const break_ctx) {
    free_vector(&break_ctx->loop_len);
    free_vector(&break_ctx->breaks);
    free(break_ctx);
}

static int break_ctx_pop_loop(BreakContext* const break_ctx) {
    int* loop_len = VECTOR_AS_INT(&break_ctx->loop_len);
    return loop_len[--break_ctx->loop_len.size];
}

static int break_ctx_pop_break(BreakContext* const break_ctx) {
    int* breaks = VECTOR_AS_INT(&break_ctx->breaks);
    return breaks[--break_ctx->breaks.size];
}

static void break_ctx_push_loop(BreakContext* const break_ctx) {
    VECTOR_ADD_INT(&break_ctx->loop_len, 0);
}

static void break_ctx_push_break(BreakContext* const break_ctx, int break_point) {
    int* loop_len = VECTOR_AS_INT(&break_ctx->loop_len);
    loop_len[break_ctx->loop_len.size - 1]++;
    VECTOR_ADD_INT(&break_ctx->breaks, break_point);
}

static void error(Compiler* const compiler, const char* message) {
    fprintf(stderr, "[Line %d] Compile error: %s\n", compiler->last_line, message);
    compiler->has_error = true;
}

static void init_compiler(Compiler* const compiler, CompilerMode mode, FileImport ctx) {
    init_scoped_symbol_table(&compiler->symbols);
    init_parser(&compiler->parser, ctx, &compiler->symbols);

    compiler->func = new_function("<GLOBAL>", 8, 0, CREATE_TYPE_UNKNOWN());
    compiler->mode = mode;

    compiler->last_line = 1;
    compiler->has_error = false;

    compiler->scope_depth = 0;
    compiler->function_scope_depth = 0;
    compiler->loop_scope_depth = 0;
    compiler->is_in_loop = false;
    compiler->next_local_index = 1; // Is expected to always have GLOBAL in pos 0
    memset(compiler->locals, 0, UINT8_COUNT);

    compiler->current_self = NULL;
    compiler->want_to_call = false;
    compiler->prop_index = PROP_INDEX_NOT_DEFINED;

    compiler->in_assignment = false;

    compiler->break_ctx = create_break_ctx();
    compiler->continue_ctx = CONTINUE_CTX_NOT_DEFINED;
}

static void free_compiler(Compiler* const compiler) {
    free_scoped_symbol_table(&compiler->symbols);
    free_break_ctx(compiler->break_ctx);
}

static void init_inner_compiler(Compiler* const inner, Compiler* const outer, const Token* fn_identifier, Symbol* fn_sym) {
    inner->symbols = outer->symbols;
    inner->parser = outer->parser;

    inner->func = new_function(
        fn_identifier->start,
        fn_identifier->length,
        SYMBOL_SET_SIZE(fn_sym->function.upvalues),
        fn_sym->type);
    inner->mode = MODE_FUNCTION;

    inner->last_line = outer->last_line;
    inner->has_error = false;

    inner->scope_depth = outer->scope_depth;
    inner->function_scope_depth = 0;
    inner->loop_scope_depth = 0;
    inner->is_in_loop = false;
    inner->next_local_index = 1; // We expect to always have a function in pos 0
    memset(inner->locals, 0, UINT8_COUNT);

    inner->current_self = outer->current_self;
    inner->want_to_call = false;
    inner->prop_index = PROP_INDEX_NOT_DEFINED;

    inner->in_assignment = false;

    inner->break_ctx = outer->break_ctx;
    inner->continue_ctx = outer->continue_ctx;
}

static Chunk* current_chunk(Compiler* const compiler) {
    return &compiler->func->chunk;
}

CompilationResult compile(FileImport ctx, ObjFunction** const result) {
#define END_WITH(final) do {\
        free_compiler(&compiler);\
        free_stmt(ast);\
        *result = final;\
} while (false)

    Compiler compiler;
    init_compiler(&compiler, MODE_SCRIPT, ctx);
    Stmt* ast = parse(&compiler.parser);
    if (compiler.parser.has_error) {
        END_WITH(NULL);
        return PARSING_ERROR;
    }
    if (!typecheck(source, ast, &compiler.symbols)) {
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
    compiler->locals[compiler->scope_depth] = 0;
    symbol_start_scope(&compiler->symbols);
}

static void end_scope(Compiler* const compiler) {
    compiler->next_local_index -= compiler->locals[compiler->scope_depth];
    pop_all_locals(compiler, compiler->scope_depth);
    compiler->scope_depth--;
    symbol_end_scope(&compiler->symbols);
}

static void pop_all_locals(Compiler* const compiler, int scope) {
    int current_locals = compiler->locals[scope];
    for (; current_locals > 0; current_locals--) {
        emit(compiler, OP_POP);
    }
}

static void reset_loop_locals(Compiler* const compiler) {
    for (int i = 0; i < compiler->loop_scope_depth; i++) {
        pop_all_locals(compiler, compiler->scope_depth - i);
    }
}

static Symbol* lookup_str(Compiler* const compiler, const char* name, int length) {
    return scoped_symbol_lookup_str(&compiler->symbols, name, length);
}

static Symbol* lookup_with_class_str(Compiler* const compiler, const char* name, int length) {
    return scoped_symbol_lookup_with_class_str(&compiler->symbols, name, length);
}

static Symbol* fn_lookup_str(Compiler* const compiler, const char* name, int length) {
    return scoped_symbol_lookup_function_str(&compiler->symbols, name, length);
}

static int emit(Compiler* const compiler, uint8_t bytecode) {
    return chunk_write(current_chunk(compiler), bytecode, compiler->last_line);
}

static int emit_short(Compiler* const compiler, uint8_t bytecode, uint8_t param) {
    emit(compiler, bytecode);
    return emit(compiler, param);
}

static int emit_long(Compiler* const compiler, uint8_t bytecode, uint16_t param) {
    uint8_t high = param >> 0x8;
    uint8_t low = param & 0x00FF;
    emit(compiler, bytecode);
    emit(compiler, high);
    return emit(compiler, low);
}

static int emit_param(Compiler* const compiler, uint8_t op_short, uint8_t op_long,  uint16_t param) {
    if (param > UINT8_MAX) {
        return emit_long(compiler, op_long, param);
    } else {
        return emit_short(compiler, op_short, param);
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

static uint8_t make_type(Compiler* const compiler, Type* type) {
    int index = chunk_add_type(current_chunk(compiler), type);
    if (index > UINT8_COUNT) {
        error(compiler, "Too many types for chunk!");
        return 0;
    }
    return (uint8_t)index;
}

#define BLOCK(compiler, ...)\
    do {\
        start_scope(compiler);\
        compiler->function_scope_depth++;\
        if (compiler->is_in_loop) {\
            compiler->loop_scope_depth++;\
        }\
        __VA_ARGS__\
        compiler->function_scope_depth--;\
        if (compiler->is_in_loop) {\
            compiler->loop_scope_depth--;\
        }\
        end_scope(compiler);\
    } while(false)

static void compile_block(void* ctx, BlockStmt* block) {
    Compiler* compiler = (Compiler*)ctx;
    BLOCK(compiler, {
        if (compiler->scope_depth > UINT8_MAX) {
            error(compiler, "Too many scopes!");
            return;
        }
        ACCEPT_STMT(compiler, block->stmts);
        emit_closed_variables(compiler, 0);
    });
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
        SYMBOL_SET_FOREACH(var_sym->upvalue_fn_refs, {
            int index = get_upvalue_index_in_function(compiler, var_sym, elements[i]);
            identifier_use_symbol(compiler, elements[i], &ops_get_identifier);
            emit(compiler, OP_BIND_CLOSED);
            emit(compiler, index);
        });
        emit(compiler, OP_POP);
    }
}

static void emit_close_stack_upvalue(Compiler* const compiler, Symbol* var_sym) {
    identifier_use_symbol(compiler, var_sym, &ops_get_identifier);
    emit(compiler, OP_CLOSE);
}

static int get_upvalue_index_in_function(Compiler* const compiler, Symbol* var_name, Symbol* fn_ref) {
    assert(fn_ref != NULL);
    assert(fn_ref->kind == SYMBOL_FUNCTION);
    assert(var_name != NULL);
    return symbol_get_function_upvalue_index(fn_ref, var_name);
}

static Token symbol_to_token_identifier(Symbol* symbol) {
    return (Token){
        .kind = TOKEN_IDENTIFIER,
        .start = SYMBOL_NAME_START(symbol->name),
        .length = SYMBOL_NAME_LENGTH(symbol->name),
        .line = symbol->line,
        .column = symbol->column,
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

    Symbol* symbol = lookup_with_class_str(compiler, function->identifier.start, function->identifier.length);
    assert(symbol != NULL);

    Value fn_value = do_compile_function(compiler, function, fn_index);
    uint16_t default_value = make_constant(compiler, fn_value);
    emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, default_value);

    emit_variable_declaration(compiler, fn_index);

    emit_bind_upvalues(compiler, symbol, function->identifier);
}

static Value do_compile_function(Compiler* const compiler, FunctionStmt* function, uint16_t index) {
    Symbol* symbol = lookup_with_class_str(compiler, function->identifier.start, function->identifier.length);
    assert(symbol != NULL);
    update_symbol_variable_info(compiler, symbol, index);

    Compiler inner;
    init_inner_compiler(&inner, compiler, &function->identifier, symbol);
    start_scope(&inner);
    update_param_index(&inner, symbol);
    ACCEPT_STMT(&inner, function->body);
    ensure_function_returns_value(&inner, symbol);
    end_scope(&inner);

    if (inner.has_error) {
        compiler->has_error = true;
    }

    return OBJ_VALUE(inner.func, symbol->type);
}

static void compile_native(void* ctx, NativeFunctionStmt* native) {
    Compiler* compiler = (Compiler*) ctx;

    Symbol* symbol = lookup_str(compiler, native->name, native->length);
    assert(symbol != NULL);

    Token identifier;
    identifier.kind = TOKEN_IDENTIFIER;
    identifier.start = native->name;
    identifier.length = native->length;
    identifier.line = symbol->line;
    identifier.column = symbol->column;

    uint16_t native_index = get_variable_index(compiler, &identifier);
    update_symbol_variable_info(compiler, symbol, native_index);

    ObjNative* obj = new_native(
        native->name,
        native->length,
        native->function,
        symbol->type);

    uint16_t default_value = make_constant(compiler, OBJ_VALUE(obj, symbol->type));
    emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, default_value);
    emit_variable_declaration(compiler, native_index);
}

static void compile_class(void* ctx, ClassStmt* klass) {
    Compiler* compiler = (Compiler*) ctx;

    Symbol* symbol = lookup_str(compiler, klass->identifier.start, klass->identifier.length);
    assert(symbol != NULL);
    assert(symbol->kind == SYMBOL_CLASS);
    uint16_t klass_index = get_variable_index(compiler, &klass->identifier);
    update_symbol_variable_info(compiler, symbol, klass_index);

    ObjClass* obj = new_class(klass->identifier.start, klass->identifier.length, symbol->type);
    start_scope(compiler);

    assert(STMT_IS_LIST(*klass->body));
    ListStmt* body = klass->body->list;

    preindex_class_props(compiler, body);

    ENABLE_SELF(compiler, symbol, {
        for (int i = 0; i < body->size; i++) {
            Stmt* prop = body->stmts[i];
            Value value;

            switch (prop->kind) {
            case STMT_FUNCTION: {
                value = do_compile_function(compiler, &prop->function, i);
                break;
            }
            case STMT_VAR: {
                value = compile_class_var_prop(compiler, &prop->var, i);
                break;
            }
            default: {
                assert(false); // You must not reach this line
                error(compiler, "Unexpected node inside class body. Expected to be function or variable");
                return;
            }
            }

            OBJ_ADD_PROP(obj, value);
        }
    });

    end_scope(compiler);

    if (body->size >= UINT8_MAX) {
        error(compiler, "Too much properties for a single class");
    }

    uint16_t default_value = make_constant(compiler, OBJ_VALUE(obj, symbol->type));
    emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, default_value);
    emit_variable_declaration(compiler, klass_index);
}

static void preindex_class_props(Compiler* const compiler, ListStmt* body) {
    for (int i = 0; i < body->size; i++) {
        Stmt* prop = body->stmts[i];
        SymbolName name;

        switch (prop->kind) {
        case STMT_FUNCTION: {
            name = create_symbol_name(prop->function.identifier.start, prop->function.identifier.length);
            break;
        }
        case STMT_VAR: {
            name = create_symbol_name(prop->var.identifier.start, prop->var.identifier.length);
            break;
        }
        default: {
            assert(false); // You must not reach this line
            return;
        }
        }

        Symbol* symbol = scoped_symbol_lookup_levels(&compiler->symbols, &name, 0);
        assert(symbol != NULL);
        symbol->constant_index = i;
    }
}

static Value compile_class_var_prop(Compiler* const compiler, VarStmt* var, uint16_t index) {
    Symbol* symbol = lookup_with_class_str(compiler, var->identifier.start, var->identifier.length);
    assert(symbol != NULL);
    update_symbol_variable_info(compiler, symbol, index);
    assert(var->definition == NULL);
    return value_default(symbol->type);
}

static void compile_native_class(void* ctx, NativeClassStmt* native_class) {
    Compiler* compiler = (Compiler*) ctx;
    start_scope(compiler); // Start and end scope to access class body. Nothing to see there.
    end_scope(compiler);
}

static void compile_native(void* ctx, NativeFunctionStmt* native) {
    Compiler* compiler = (Compiler*) ctx;

    Symbol* symbol = lookup_str(compiler, native->name, native->length);
    assert(symbol != NULL);

    Token identifier;
    identifier.kind = TOKEN_IDENTIFIER;
    identifier.start = native->name;
    identifier.length = native->length;
    identifier.line = symbol->declaration_line;
    identifier.column = 0;

    uint16_t native_index = get_variable_index(compiler, &identifier);
    update_symbol_variable_info(compiler, symbol, native_index);

    ObjNative* obj = new_native(
        native->name,
        native->length,
        native->function,
        symbol->type);

    uint16_t default_value = make_constant(compiler, OBJ_VALUE(obj, symbol->type));
    emit_param(compiler, OP_CONSTANT, OP_CONSTANT_LONG, default_value);
    emit_variable_declaration(compiler, native_index);
}

static void emit_bind_upvalues(Compiler* const compiler, Symbol* fn_sym, Token fn) {
    Symbol** upvalues = SYMBOL_SET_GET_ELEMENTS(fn_sym->function.upvalues);
    int size = SYMBOL_SET_SIZE(fn_sym->function.upvalues);
    for (int i = 0; i < size; i++) {
        Symbol* upvalue_sym = upvalues[i];
        identifier_use(compiler, fn, &ops_get_identifier);
        emit(compiler, OP_BIND_UPVALUE);
        emit(compiler, upvalue_sym->constant_index);
        emit(compiler, i);
    }
}

static void patch_chunk(Compiler* const compiler, int position, uint8_t value) {
    chunk_patch(current_chunk(compiler), position, value);
}

static void patch_chunk_long(Compiler* const compiler, int position, uint16_t value) {
    uint8_t high = value >> 0x8;
    uint8_t low = value & 0x00FF;
    patch_chunk(compiler, position-1, high);
    patch_chunk(compiler, position, low);
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
        param_sym->constant_index = NEXT_LOCAL_INDEX(compiler);
    }
    if (HAVE_SELF(compiler)) {
        Symbol* self = lookup_str(compiler, CLASS_SELF_NAME, CLASS_SELF_LENGTH);
        assert(self != NULL);
        self->constant_index = NEXT_LOCAL_INDEX(compiler);
    }
}

static void compile_return(void* ctx, ReturnStmt* return_) {
    Compiler* compiler = (Compiler*) ctx;

    emit_closed_variables(compiler, compiler->function_scope_depth);

    if (return_->inner != NULL) {
        IN_ASSIGNMENT(compiler, { // We assume a return is also an assigment
            ACCEPT_EXPR(compiler, return_->inner);
        });
    } else {
        emit(compiler, OP_NIL);
    }
    emit(compiler, OP_RETURN);
}

static void compile_if(void* ctx, IfStmt* if_) {
    Compiler* compiler = (Compiler*) ctx;
    bool have_else = if_->else_ != NULL;
    int patch_else_pos = 0;

    ACCEPT_EXPR(ctx, if_->condition);

    int patch_if_pos = emit_jump_to(compiler, OP_JUMP_IF_FALSE, 0);
    ACCEPT_STMT(ctx, if_->then);
    if (have_else) {
        patch_else_pos = emit_jump_to(compiler, OP_JUMP, 0);
    }
    patch_jump(compiler, patch_if_pos);

    if (have_else) {
        ACCEPT_STMT(ctx, if_->else_);
        patch_jump(compiler, patch_else_pos);
    }
}

static void compile_for(void* ctx, ForStmt* for_) {
    Compiler* compiler = (Compiler*) ctx;
    LOOP(compiler, {
        start_scope(compiler);
        BREAK_CTX_PUSH_LOOP(compiler);

        ACCEPT_STMT(compiler, for_->init);

        int loop_init = emit(compiler, OP_NOP);

        if (for_->condition != NULL) {
            ACCEPT_EXPR(compiler, for_->condition);
        } else {
            emit(compiler, OP_TRUE);
        }
        int patch_for_pos = emit_jump_to(compiler, OP_JUMP_IF_FALSE, 0);

        CONTINUE_CTX(compiler, loop_init, {
            ACCEPT_STMT(compiler, for_->body);
        });
        ACCEPT_STMT(compiler, for_->mod);

        emit_jump_to(compiler, OP_JUMP, loop_init);

        patch_jump(compiler, patch_for_pos);
        patch_breaks(compiler);

        end_scope(compiler);
    });
}

static void compile_while(void* ctx, WhileStmt* while_) {
    Compiler* compiler = (Compiler*) ctx;
    LOOP(compiler, {
        BREAK_CTX_PUSH_LOOP(compiler);

        int loop_init = emit(compiler, OP_NOP);

        ACCEPT_EXPR(compiler, while_->condition);
        int patch_for_pos = emit_jump_to(compiler, OP_JUMP_IF_FALSE, 0);

        CONTINUE_CTX(compiler, loop_init, {
            ACCEPT_STMT(compiler, while_->body);
        });

        emit_jump_to(compiler, OP_JUMP, loop_init);

        patch_jump(compiler, patch_for_pos);
        patch_breaks(compiler);
    });
}

static void compile_loopg(void* ctx, LoopGotoStmt* loopg) {
    Compiler* compiler = (Compiler*) ctx;
    reset_loop_locals(compiler);
    if (loopg->kind == LOOP_BREAK) {
        int break_pos = emit_jump_to(compiler, OP_JUMP, 0);
        BREAK_CTX_PUSH_BREAK(compiler, break_pos);
    } else {
        assert(compiler->continue_ctx != CONTINUE_CTX_NOT_DEFINED);
        emit_jump_to(compiler, OP_JUMP, compiler->continue_ctx);
    }
}

static void compile_typealias(void* ctx, TypealiasStmt* alias) {
    // Nothing to see here
}

static void compile_import(void* ctx, ImportStmt* import) {
    ACCEPT_STMT(ctx, import->ast);
}

static void patch_breaks(Compiler* const compiler) {
    int breaks_in_loop = BREAK_CTX_POP_LOOP(compiler);
    if (breaks_in_loop <= 0) {
        return;
    }
    int jump_dst = emit(compiler, OP_NOP);
    for (int i = 0; i < breaks_in_loop; i++) {
        int break_to_patch = BREAK_CTX_POP_BREAK(compiler);
        check_jump_distance(compiler, jump_dst - break_to_patch);
        patch_chunk_long(compiler, break_to_patch, jump_dst);
    }
}

static void patch_jump(Compiler* const compiler, int patch) {
    int jump_dst = emit(compiler, OP_NOP);
    check_jump_distance(compiler, jump_dst - patch);
    patch_chunk_long(compiler, patch, jump_dst);
}

static int emit_jump_to(Compiler* const compiler, uint8_t jump_op, uint16_t to) {
    int jump_to_init = emit_long(compiler, jump_op, to);
    check_jump_distance(compiler, jump_to_init - (int)to);
    return jump_to_init;
}

static void check_jump_distance(Compiler* const compiler, int distance) {
    assert(distance > 0);
    if (distance > UINT16_MAX) {
        error(compiler, "Jump too large");
    }
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
        IN_ASSIGNMENT(compiler, {
            ACCEPT_EXPR(compiler, var->definition);
        });
    }
    emit_variable_declaration(compiler, variable_index);
}

static void update_symbol_variable_info(Compiler* const compiler, Symbol* var_sym, uint16_t var_index) {
    var_sym->constant_index = var_index;
}

static uint16_t get_variable_index(Compiler* const compiler, const Token* identifier) {
    if (compiler->scope_depth == 0) {
        return identifier_constant(compiler, identifier);
    }
    uint16_t index = NEXT_LOCAL_INDEX(compiler);
    compiler->locals[compiler->scope_depth]++;
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
    IN_ASSIGNMENT(compiler, {
        ACCEPT_EXPR(compiler, assignment->value);
    });
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
    printf("[COMPILER DEBUG] Identifier use: ");
    token_print(identifier);
    printf("[COMPILER DEBUG] Index %d\n", symbol->constant_index);
#endif
    if (symbol->global) {
        emit_param(compiler, ops->op_global, ops->op_global_long, symbol->constant_index);
        return;
    }
    assert(symbol->constant_index < UINT8_MAX);
    int index = get_current_function_upvalue_index(compiler, symbol);
    if (index != -1) {
        emit_short(compiler, ops->op_upvalue, index);
        return;
    }
    emit_short(compiler, ops->op_local, symbol->constant_index);
}

static int get_current_function_upvalue_index(Compiler* const compiler, Symbol* var) {
    if (compiler->mode == MODE_SCRIPT) {
        return -1;
    }
    Symbol* fn_sym = fn_lookup_str(
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
    WANT_TO_CALL(compiler, {
        ACCEPT_EXPR(compiler, call->callee);
        call_with_params(compiler, &call->params);
    });
}

static void compile_prop(void* ctx, PropExpr* prop) {
    Compiler* compiler = (Compiler*) ctx;

    // TODO create a macro or something for this shit
    bool old_want = compiler->want_to_call;
    compiler->want_to_call = false;
    ACCEPT_EXPR(compiler, prop->object);
    compiler->want_to_call = old_want;

    Symbol* klass_sym = lookup_str(compiler, type_get_class_name(prop->object_type), type_get_class_length(prop->object_type));
    assert(klass_sym != NULL);
    Symbol* prop_symbol = symbol_lookup_str(klass_sym->klass.body, prop->prop.start, prop->prop.length);
    assert(prop_symbol != NULL);

    if (compiler->want_to_call) {
        // If someone wants to call this property, an OP_INVOKE will be emited
        // instead of OP_BINDED_METHOD or OP_GET_PROP (see compile_call function)
        compiler->prop_index = prop_symbol->constant_index;
        return;
    }

    uint8_t opcode = (compiler->in_assignment && TYPE_IS_FUNCTION(prop_symbol->type)) ?
        OP_BINDED_METHOD :
        OP_GET_PROP;
    emit_short(compiler, opcode, prop_symbol->constant_index);
}

static void compile_array(void* ctx, ArrayExpr* arr) {
    Compiler* compiler = (Compiler*) ctx;

    uint8_t index_type = make_type(compiler, arr->inner);
    emit_short(compiler, OP_ARRAY, index_type);

    Expr** exprs = VECTOR_AS_EXPRS(&arr->elements);
    IN_ASSIGNMENT(compiler, { // Assign to array positions is an assigment.
        for (uint32_t i = arr->elements.size; i > 0; i--) {
            ACCEPT_EXPR(compiler, exprs[i - 1]);
            emit(compiler, OP_ARRAY_PUSH);
        }
    });
}

static void compile_cast(void* ctx, CastExpr* cast) {
    Compiler* compiler = (Compiler*) ctx;
    ACCEPT_EXPR(compiler, cast->inner);
    uint8_t type_index = make_type(compiler, cast->type);
    emit_short(compiler, OP_CAST, type_index);
}

static void compile_prop_assigment(void* ctx, PropAssigmentExpr* prop_assignment) {
    Compiler* compiler = (Compiler*) ctx;

    ACCEPT_EXPR(compiler, prop_assignment->object);

    Symbol* klass_sym = lookup_str(
        compiler,
        TYPE_OBJECT_CLASS_NAME(prop_assignment->object_type),
        TYPE_OBJECT_CLASS_LENGTH(prop_assignment->object_type));
    assert(klass_sym != NULL);

    Symbol* prop_symbol = symbol_lookup_str(
        klass_sym->klass.body,
        prop_assignment->prop.start,
        prop_assignment->prop.length);
    assert(prop_symbol != NULL);

    ACCEPT_EXPR(compiler, prop_assignment->value);

    emit_short(compiler, OP_SET_PROP, prop_symbol->constant_index);
}

static void compile_new(void* ctx, NewExpr* new_) {
    Compiler* compiler = (Compiler*) ctx;

    Symbol* klass_sym = lookup_str(compiler, new_->klass.start, new_->klass.length);
    assert(klass_sym != NULL);
    assert(klass_sym->kind == SYMBOL_CLASS);
    assert(klass_sym->klass.body != NULL);

    identifier_use(compiler, new_->klass, &ops_get_identifier);
    emit(compiler, OP_NEW);

    Symbol* init_prop = SCOPED_SYMBOL_LOOKUP_OBJECT_INIT(klass_sym);
    if (init_prop == NULL) {
        emit(compiler, OP_POP); // OP_NEW pushes two times the object (the second to call init)
        return;
    }

    WANT_TO_CALL(compiler, {
        compiler->prop_index = init_prop->constant_index;
        call_with_params(compiler, &new_->params);
    });
    emit(compiler, OP_POP); // The result of calling init is always nil.
}

static void call_with_params(Compiler* const compiler, Vector* params) {
    Expr** exprs = VECTOR_AS_EXPRS(params);

    // Disable want_to_call while processing params. If you dont disable it and
    // one param is a property of an object, it will not emit OP_GET_PROP and
    // it will try to call that param.
    bool old_want = compiler->want_to_call;
    compiler->want_to_call = false;

    // Disable prop_index, which is used to know if is a method. In the params we can
    // have other function calls with other prop_index or a plain function with no prop_index.
    int old_prop_index = compiler->prop_index;
    compiler->prop_index = PROP_INDEX_NOT_DEFINED;

    uint32_t i = 0;
    IN_ASSIGNMENT(compiler, { // Param passing is an assigment to "params" vars
        for (; i < params->size; i++) {
            ACCEPT_EXPR(compiler, exprs[i]);
        }
    });

    compiler->prop_index = old_prop_index;
    compiler->want_to_call = old_want;

    if (i > UINT8_MAX) {
        error(compiler, "Parameter count exceeds the max number of parameters: 254");
    }

    if (compiler->prop_index != PROP_INDEX_NOT_DEFINED) {
        emit_short(compiler, OP_INVOKE, compiler->prop_index);
        emit(compiler, i);
    } else {
        emit_short(compiler, OP_CALL, i);
    }
}

