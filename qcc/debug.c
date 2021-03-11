#include <stdarg.h>
#include "debug.h"

void valuearray_print(ValueArray* values) {
    printf("--------[ VALUE ARRAY ]--------\n\n");
    printf("| Index\t| Value\n");
    printf("|-------|------------\n");
    for (int i = 0; i < values->size; i++) {
        printf("| %04d\t| ", i);
        value_print(values->values[i]);
        printf("\n");
    }
    printf("\n\n");
}

static const char* OpCodeStrings[] = {
    "OP_ADD",
    "OP_SUB",
    "OP_MUL",
    "OP_DIV",
    "OP_MOD",
    "OP_NEGATE",
    "OP_NOT",
    "OP_AND",
    "OP_OR",
    "OP_CONSTANT",
    "OP_NOP",
    "OP_RETURN",
};

void opcode_print(uint8_t op) {
    // @warning this be out of range.
    printf("%s\n", OpCodeStrings[op]);
}

void stack_print(Value* stack_top, Value* stack) {
    Value* current = stack;
    while (current < stack_top) {
        printf("[ ");
        value_print(*current);
        printf(" ] ");
        current = current + 1;
    }
}

static void chunk_format_print(Chunk* chunk, int i, const char* format, ...) {
    printf("[%02d;%02d]\t", i, chunk->lines[i]);
    va_list params;
    va_start(params, format);
    vprintf(format, params);
    va_end(params);
}

static void chunk_opcode_print(Chunk* chunk, int i) {
    chunk_format_print(chunk, i, "%s\n", OpCodeStrings[chunk->code[i]]);
}

void chunk_print(Chunk* chunk) {
    printf("--------[ CHUNK DUMP ]--------\n\n");
    for (int i = 0; i < chunk->size; i++) {
        printf("[%d] %04x\n", i, chunk->code[i]);
    }
    printf("\n\n");
    int i = 0;
    while (i < chunk->size) {
        switch(chunk->code[i]) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_NEGATE:
        case OP_RETURN:
        case OP_NOT:
        case OP_NOP:
        case OP_AND:
        case OP_OR: {
            chunk_opcode_print(chunk, i++);
            break;
        }
        case OP_CONSTANT: {
            chunk_opcode_print(chunk, i++);
            Value val = chunk->constants.values[chunk->code[i]];
            chunk_format_print(chunk, i, "%04x\t", chunk->code[i]);
            value_print(val);
            printf("\n");
            i++;
            break;
        }
        }
    }
}

static const char* token_type_print(TokenType type) {
    switch (type) {
    case TOKEN_END: return "End";
    case TOKEN_ERROR: return "Error";
    case TOKEN_PLUS: return "Plus";
    case TOKEN_MINUS: return "Minus";
    case TOKEN_STAR: return "Star";
    case TOKEN_SLASH: return "Slash";
    case TOKEN_PERCENT: return "Percent";
    case TOKEN_LEFT_PAREN: return "LeftParen";
    case TOKEN_RIGHT_PAREN: return "RightParen";
    case TOKEN_DOT: return "Dot";
    case TOKEN_BANG: return "Bang";
    case TOKEN_AND: return "And";
    case TOKEN_OR: return "Or";
    case TOKEN_NUMBER: return "Number";
    case TOKEN_TRUE: return "True";
    case TOKEN_FALSE: return "False";
    case TOKEN_STRING: return "String";
    default: return "Unknown";
    }
}

void token_print(Token token) {
    printf(
        "Token{ Type: '%s', Line: '%d', Value: '%.*s', Length: '%d' }\n",
        token_type_print(token.type),
        token.line,
        token.length,
        token.start,
        token.length);
}

static void print_offset();
static void pretty_print(const char *msg, ...);
static void print_binary(void* ctx, BinaryExpr* binary);
static void print_unary(void* ctx, UnaryExpr* unary);
static void print_literal(void* ctx, LiteralExpr *literal);

ExprVisitor printer_visitor = (ExprVisitor){
    .visit_literal = print_literal,
    .visit_binary = print_binary,
    .visit_unary = print_unary,
};

#define ACCEPT(expr) expr_dispatch(&printer_visitor, NULL, expr)

int offset = 0;

#define OFFSET(...) do { offset++; __VA_ARGS__ offset--; } while(false)

void ast_print(Expr* root) {
    ACCEPT(root);
}

static void print_offset() {
    for (int i = 0; i < offset; i++) {
        printf("  ");
    }
}

static void pretty_print(const char *msg, ...) {
    printf("[PARSER DEBUG]: ");
    print_offset();
    printf("%s", msg);
}

static void print_binary(void* ctx, BinaryExpr* binary) {
    pretty_print("Bianary: [\n");
    OFFSET({
        pretty_print("Left:\n");
        OFFSET({
            ACCEPT(binary->left);
        });

        pretty_print("Operator: ");
        token_print(binary->op);

        pretty_print("Right: \n");
        OFFSET({
            ACCEPT(binary->right);
        });
    });
    pretty_print("]\n");
}

static void print_literal(void* ctx, LiteralExpr *literal) {
    pretty_print("Literal: [\n");
    OFFSET({
        pretty_print("Value: ");
        token_print(literal->literal);
    });
    pretty_print("]\n");
}

static void print_unary(void* ctx, UnaryExpr* unary) {
    pretty_print("Unary: [\n");
    OFFSET({
        pretty_print("Op: ");
        token_print(unary->op);
        pretty_print("Expr: \n");
        OFFSET({
            ACCEPT(unary->expr);
        });
    });
    pretty_print("]\n");
}
