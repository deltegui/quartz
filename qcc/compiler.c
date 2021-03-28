#include "compiler.h"
#include "typechecker.h"
#include "values.h"

#ifdef COMPILER_DEBUG
#include "debug.h"
#endif

typedef struct {
    Parser parser;
    Chunk* chunk;
    int last_line;
    bool has_error;
} Compiler;

static void error(Compiler* compiler, const char* message);
static void emit(Compiler* compiler, uint8_t bytecode);
static void emit_bytes(Compiler* compiler, uint8_t first, uint8_t second);
static void init_compiler(Compiler* compiler, const char* source, Chunk* output);

static void compile_literal(void* ctx, LiteralExpr* literal);
static void compile_binary(void* ctx, BinaryExpr* binary);
static void compile_unary(void* ctx, UnaryExpr* unary);

ExprVisitor compiler_expr_visitor = (ExprVisitor){
    .visit_literal = compile_literal,
    .visit_binary = compile_binary,
    .visit_unary = compile_unary,
};

static void compile_expr(void* ctx, ExprStmt* expr);
static void compile_var(void* ctx, VarStmt* var);
static void compile_print(void* ctx, PrintStmt* print);

StmtVisitor compiler_stmt_visitor = (StmtVisitor){
    .visit_expr = compile_expr,
    .visit_var = compile_var,
    .visit_print = compile_print,
};

#define ACCEPT_STMT(compiler, stmt) stmt_dispatch(&compiler_stmt_visitor, compiler, stmt)
#define ACCEPT_EXPR(compiler, expr) expr_dispatch(&compiler_expr_visitor, compiler, expr)

static void error(Compiler* compiler, const char* message) {
    fprintf(stderr, "[Line %d] Compile error: %s\n", compiler->last_line, message);
    compiler->has_error = true;
}

void init_compiler(Compiler* compiler, const char* source, Chunk* output) {
    init_parser(&compiler->parser, source);
    compiler->chunk = output;
    compiler->last_line = 1;
    compiler->has_error = false;
}

CompilationResult compile(const char* source, Chunk* output_chunk) {
    Compiler compiler;
    init_compiler(&compiler, source, output_chunk);
    Stmt* ast = parse(&compiler.parser);
    if (compiler.parser.has_error) {
        free_stmt(ast); // Although parser had errors, the ast exists.
        return PARSING_ERROR;
    }
    if (!typecheck(ast)) {
        free_stmt(ast);
        return TYPE_ERROR;
    }
    ACCEPT_STMT(&compiler, ast);
    emit(&compiler, OP_RETURN);
#ifdef COMPILER_DEBUG
    valuearray_print(&compiler.chunk->constants);
    chunk_print(compiler.chunk);
#endif
    free_stmt(ast);
    if (compiler.has_error) {
        return COMPILATION_ERROR;
    }
    return COMPILATION_OK;
}

static void emit(Compiler* compiler, uint8_t bytecode) {
    chunk_write(compiler->chunk, bytecode, compiler->last_line);
}

static void emit_bytes(Compiler* compiler, uint8_t first, uint8_t second) {
    chunk_write(compiler->chunk, first, compiler->last_line);
    chunk_write(compiler->chunk, second, compiler->last_line);
}

static void compile_print(void* ctx, PrintStmt* print) {
    Compiler* compiler = (Compiler*)ctx;
    ACCEPT_EXPR(compiler, print->inner);
    emit(compiler, OP_PRINT);
}

static void compile_expr(void* ctx, ExprStmt* expr) {
    Compiler* compiler = (Compiler*)ctx;
    ACCEPT_EXPR(compiler, expr->inner);
    emit(compiler, OP_POP);
}

static void compile_var(void* ctx, VarStmt* var) {
    //@todo implement
}

// @todo Maybe can this function be rewrited in a way that express the difference
// between reserved words and real literals
static void compile_literal(void* ctx, LiteralExpr* literal) {
    Compiler* compiler = (Compiler*) ctx;
    compiler->last_line = literal->literal.line;
    Value value;
    switch (literal->literal.type) {
    // We start with reserved words that have its own opcode.
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
    // Continue creating linerals
    case TOKEN_NUMBER: {
        double d = (double) strtod(literal->literal.start, NULL);
        value = NUMBER_VALUE(d);
        break;
    }
    case TOKEN_STRING: {
        ObjString* str = copy_string(literal->literal.start, literal->literal.length);
        value = OBJ_VALUE(str);
        break;
    }
    default:
        error(compiler, "Unkown literal expression");
        return;
    }
    emit(compiler, OP_CONSTANT);
    uint8_t value_pos = valuearray_write(&compiler->chunk->constants, value);
    emit(compiler, value_pos);
}

static void compile_binary(void* ctx, BinaryExpr* binary) {
    Compiler* compiler = (Compiler*) ctx;
    compiler->last_line = binary->op.line;

    ACCEPT_EXPR(compiler, binary->left);
    ACCEPT_EXPR(compiler, binary->right);

#define EMIT(byte) emit(compiler, byte)
#define EMIT_BYTES(first, second) emit_bytes(compiler, first, second)

    switch(binary->op.type) {
    case TOKEN_PLUS: EMIT(OP_ADD); break;
    case TOKEN_MINUS: EMIT(OP_SUB); break;
    case TOKEN_STAR: EMIT(OP_MUL); break;
    case TOKEN_SLASH: EMIT(OP_DIV); break;
    case TOKEN_AND: EMIT(OP_AND); break;
    case TOKEN_OR: EMIT(OP_OR); break;
    case TOKEN_PERCENT: EMIT(OP_MOD); break;
    case TOKEN_EQUAL_EQUAL: EMIT(OP_EQUAL); break;
    case TOKEN_BANG_EQUAL: EMIT_BYTES(OP_EQUAL, OP_NOT); break;
    case TOKEN_LOWER: EMIT(OP_LOWER); break;
    case TOKEN_LOWER_EQUAL: EMIT_BYTES(OP_GREATER, OP_NOT); break;
    case TOKEN_GREATER: EMIT(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: EMIT_BYTES(OP_LOWER, OP_NOT); break;
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
    switch(unary->op.type) {
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
