#include "compiler.h"
#include "typechecker.h"
#include "values.h"

#ifdef COMPILER_DEBUG
#include "debug.h"
#endif

typedef struct {
    Parser parser;
    Chunk* chunk;
    bool has_error;
} Compiler;

static void error(Compiler* compiler, const char* message, int line);
static void emit(Compiler* compiler, uint8_t bytecode, int line);
static void emit_bytes(Compiler* compiler, uint8_t first, uint8_t second, int line);
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

StmtVisitor compiler_stmt_visitor = (StmtVisitor){
    .visit_expr = compile_expr,
    .visit_var = compile_var,
};

#define ACCEPT_STMT(compiler, stmt) stmt_dispatch(&compiler_stmt_visitor, compiler, stmt)
#define ACCEPT_EXPR(compiler, expr) expr_dispatch(&compiler_expr_visitor, compiler, expr)

static void error(Compiler* compiler, const char* message, int line) {
    fprintf(stderr, "[Line %d] Compile error: %s\n", line, message);
    compiler->has_error = true;
}

void init_compiler(Compiler* compiler, const char* source, Chunk* output) {
    init_parser(&compiler->parser, source);
    compiler->chunk = output;
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
    emit(&compiler, OP_RETURN, -1);
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

static void emit(Compiler* compiler, uint8_t bytecode, int line) {
    chunk_write(compiler->chunk, bytecode, line);
}

static void emit_bytes(Compiler* compiler, uint8_t first, uint8_t second, int line) {
    chunk_write(compiler->chunk, first, line);
    chunk_write(compiler->chunk, second, line);
}

static void compile_expr(void* ctx, ExprStmt* expr) {
    ACCEPT_EXPR(ctx, expr->inner);
}

static void compile_var(void* ctx, VarStmt* var) {
    //@todo implement
}

// @todo Maybe can this function be rewrited in a way that express the difference
// between reserved words and real literals
static void compile_literal(void* ctx, LiteralExpr* literal) {
    Compiler* compiler = (Compiler*) ctx;
    Value value;
    switch (literal->literal.type) {
    // We start with reserved words that have its own opcode.
    case TOKEN_TRUE: {
        emit(compiler, OP_TRUE, literal->literal.line);
        return;
    }
    case TOKEN_FALSE: {
        emit(compiler, OP_FALSE, literal->literal.line);
        return;
    }
    case TOKEN_NIL: {
        emit(compiler, OP_NIL, literal->literal.line);
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
        error(compiler, "Unkown literal expression", literal->literal.line);
        return;
    }
    emit(compiler, OP_CONSTANT, literal->literal.line);
    uint8_t value_pos = valuearray_write(&compiler->chunk->constants, value);
    emit(compiler, value_pos, literal->literal.line);
}

static void compile_binary(void* ctx, BinaryExpr* binary) {
    Compiler* compiler = (Compiler*) ctx;
    ACCEPT_EXPR(compiler, binary->left);
    ACCEPT_EXPR(compiler, binary->right);

#define EMIT(byte) emit(compiler, byte, binary->op.line)
#define EMIT_BYTES(first, second) emit_bytes(compiler, first, second, binary->op.line)

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
        error(compiler, "Unkown binary operator in expression", binary->op.line);
        return;
    }

#undef EMIT
#undef EMIT_BYTES
}

static void compile_unary(void* ctx, UnaryExpr* unary) {
    Compiler* compiler = (Compiler*) ctx;
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
        error(compiler, "Unkown unary operator in expression", unary->op.line);
        return;
    }
    ACCEPT_EXPR(compiler, unary->expr);
    emit(compiler, op, unary->op.line);
}
