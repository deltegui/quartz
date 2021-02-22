#include "compiler.h"
#include "chunk.h"
#include "expr.h"
#include "values.h"

static void emit(uint8_t bytecode, int line);

static void compile_literal(LiteralExpr* literal);
static void compile_binary(BinaryExpr* binary);

typedef struct {
    Chunk chunk;
} Compiler;

Compiler compiler;

ExprVisitor compiler_visitor = (ExprVisitor){
    .visit_literal = compile_literal,
    .visit_binary = compile_binary,
};

#define ACCEPT(expr) expr_dispatch(&compiler_visitor, expr)

void init_compiler() {
    chunk_init(&compiler.chunk);
}

void free_compiler() {
    chunk_free(&compiler.chunk);
}

Chunk* compile(Expr* ast) {
    ACCEPT(ast);
    emit(OP_RETURN, -1);
    valuearray_print(&compiler.chunk.constants);
    return &compiler.chunk;
}

static void emit(uint8_t bytecode, int line) {
    chunk_write(&compiler.chunk, bytecode, line);
}

static void compile_literal(LiteralExpr* literal) {
    Value value;
    // @todo Change literal prop in LiteralExpr to token.
    /*
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
    default:
        // @todo THIS IS SHIT. REWRITE THIS GENERIC PART OF
        // THE COMPILATION.
        fprintf(stderr, "Compile error: expected integer or float\n");
        exit(1);
    }
    */
    double d = (double) strtod(literal->literal.start, NULL);
    value = FLOAT_VALUE(d);
    emit(OP_CONSTANT, literal->literal.line);
    uint8_t value_pos = valuearray_write(&compiler.chunk.constants, value);
    emit(value_pos, literal->literal.line);
}

static void compile_binary(BinaryExpr* binary) {
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
    default:
        // @todo THIS IS SHIT. REWRITE THIS GENERIC PART OF
        // THE COMPILATION.
        fprintf(stderr, "Compile error: expected binary operation\n");
        exit(1);
    }
    ACCEPT(binary->left); // Compile left argument
    ACCEPT(binary->right); // Compile right argument
    emit(op, binary->op.line);
}
