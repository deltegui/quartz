#include "compiler.h"
#include "typechecker.h"
#include "values.h"

#ifdef COMPILER_DEBUG
#include "debug.h"
#endif

static void error(Compiler* compiler, const char* message, int line);

static void emit(Compiler* compiler, uint8_t bytecode, int line);

static void init_compiler(Compiler* compiler, const char* source, Chunk* output);

static void compile_literal(void* ctx, LiteralExpr* literal);
static void compile_binary(void* ctx, BinaryExpr* binary);
static void compile_unary(void* ctx, UnaryExpr* unary);

ExprVisitor compiler_visitor = (ExprVisitor){
    .visit_literal = compile_literal,
    .visit_binary = compile_binary,
    .visit_unary = compile_unary,
};

#define ACCEPT(compiler, expr) expr_dispatch(&compiler_visitor, compiler, expr)

static void error(Compiler* compiler, const char* message, int line) {
    fprintf(stderr, "[Line %d] Compile error: %s\n", line, message);
    compiler->has_error = true;
}

void init_compiler(Compiler* compiler, const char* source, Chunk* output) {
    init_parser(&compiler->parser, source);
    compiler->chunk = output;
    compiler->has_error = false;
}

bool compile(const char* source, Chunk* output_chunk) {
    Compiler compiler;
    init_compiler(&compiler, source, output_chunk);
    Expr* ast = parse(&compiler.parser);
    if (compiler.parser.has_error || !typecheck(ast)) {
        free_expr(ast);
        return false;
    }
    ACCEPT(&compiler, ast);
    emit(&compiler, OP_RETURN, -1);
#ifdef COMPILER_DEBUG
    valuearray_print(&compiler.chunk->constants);
    chunk_print(compiler.chunk);
#endif
    free_expr(ast);
    return !compiler.has_error;
}

static void emit(Compiler* compiler, uint8_t bytecode, int line) {
    chunk_write(compiler->chunk, bytecode, line);
}

static void compile_literal(void* ctx, LiteralExpr* literal) {
    Compiler* compiler = (Compiler*) ctx;
    Value value;
    switch(literal->literal.type) {
    case TOKEN_INTEGER: {
        int i = (int) strtol(literal->literal.start, NULL, 10);
        value = INTEGER_VALUE(i);
        break;
    }
    case TOKEN_FLOAT: {
        double d = (double) strtod(literal->literal.start, NULL);
        value = FLOAT_VALUE(d);
        break;
    }
    case TOKEN_TRUE: {
        value = BOOL_VALUE(true);
        break;
    }
    case TOKEN_FALSE: {
        value = BOOL_VALUE(false);
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
    OpCode op;
    switch(binary->op.type) {
    case TOKEN_PLUS:
        op = OP_ADD;
        break;
    case TOKEN_MINUS:
        op = OP_SUB;
        break;
    case TOKEN_STAR:
        op = OP_MUL;
        break;
    case TOKEN_SLASH:
        op = OP_DIV;
        break;
    case TOKEN_AND:
        op = OP_AND;
        break;
    case TOKEN_OR:
        op = OP_OR;
        break;
    default:
        error(compiler, "Unkown binary operator in expression", binary->op.line);
        return;
    }
    ACCEPT(compiler, binary->left); // Compile left argument
    ACCEPT(compiler, binary->right); // Compile right argument
    emit(compiler, op, binary->op.line);
}

static void compile_unary(void* ctx, UnaryExpr* unary) {
    Compiler* compiler = (Compiler*) ctx;
    OpCode op;
    switch(unary->op.type) {
    case TOKEN_BANG:
        op = OP_NOT;
        break;
    default:
        error(compiler, "Unkown unary operator in expression", unary->op.line);
        return;
    }
    ACCEPT(compiler, unary->expr);
    emit(compiler, op, unary->op.line);
}
