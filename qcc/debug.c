#include "debug.h"
#include <stdarg.h>
#include "expr.h"
#include "type.h"
#include "object.h"

static void symbol_table_print(SymbolTable* table) {
    printf("--------[ SYMBOL TABLE ]--------\n\n");
    printf("| Name\t| Line\t| Type\n");
    printf("|-------|-------|------------\n");
    for (int i = 0; i < table->capacity; i++) {
        Symbol* current = &table->entries[i];
        if (current->name.length == 0) {
            continue;
        }
        printf(
            "| %.*s\t| %d\t| ",
            current->name.length,
            current->name.str,
            current->declaration_line);
        type_print(current->type);
        printf("\n");
    }
    printf("\n\n");
}

static void symbol_node_print(SymbolNode* node) {
    symbol_table_print(&node->symbols);
    for (int i = 0; i < node->size; i++) {
        symbol_node_print(&node->childs[i]);
    }
}

void scoped_symbol_table_print(ScopedSymbolTable* table) {
    symbol_node_print(&table->global);
}

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
    "OP_EQUAL",
    "OP_GREATER",
    "OP_LOWER",
    "OP_TRUE",
    "OP_FALSE",
    "OP_NIL",
    "OP_NOP",
    "OP_PRINT",
    "OP_RETURN",
    "OP_POP",
    "OP_CALL",
    "OP_END",
    "OP_CONSTANT",
    "OP_CONSTANT_LONG",
    "OP_DEFINE_GLOBAL",
    "OP_GET_GLOBAL",
    "OP_SET_GLOBAL",
    "OP_DEFINE_GLOBAL_LONG",
    "OP_GET_GLOBAL_LONG",
    "OP_SET_GLOBAL_LONG",
    "OP_GET_LOCAL",
    "OP_SET_LOCAL",
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

static void chunk_format_print(Chunk* chunk, int i, const char* format, ...);
static void chunk_value_print(Chunk* chunk, int index);
static int chunk_opcode_print(Chunk* chunk, int i);
static int chunk_short_print(Chunk* chunk, int i);
static int chunk_long_print(Chunk* chunk, int i);
static void standalone_chunk_print(Chunk* chunk);

static void chunk_format_print(Chunk* chunk, int i, const char* format, ...) {
    printf("[%02d;%02d]\t", i, chunk->lines[i]);
    va_list params;
    va_start(params, format);
    vprintf(format, params);
    va_end(params);
}

static void chunk_value_print(Chunk* chunk, int index) {
    Value val = chunk->constants.values[index];
    value_print(val);
    printf("\n");
}

static int chunk_opcode_print(Chunk* chunk, int i) {
    chunk_format_print(chunk, i, "%s\n", OpCodeStrings[chunk->code[i]]);
    return ++i;
}

static int chunk_short_print(Chunk* chunk, int i) {
    i = chunk_opcode_print(chunk, i);
    chunk_format_print(chunk, i, "%04x\n", chunk->code[i]);
    // chunk_value_print(chunk, chunk->code[i]);
    return ++i;
}

static int chunk_long_print(Chunk* chunk, int i) {
    i = chunk_opcode_print(chunk, i);
    uint8_t* pc = &chunk->code[i];
    uint16_t num = read_long(&pc);
    i++;
    chunk_format_print(chunk, i, "%04x\t", num);
    chunk_value_print(chunk, num);
    return ++i;
}

static void standalone_chunk_print(Chunk* chunk) {
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
        case OP_OR:
        case OP_TRUE:
        case OP_FALSE:
        case OP_NIL:
        case OP_EQUAL:
        case OP_LOWER:
        case OP_POP:
        case OP_PRINT:
        case OP_GREATER:
        case OP_END: {
            i = chunk_opcode_print(chunk, i);
            break;
        }
        case OP_DEFINE_GLOBAL:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
        case OP_CONSTANT:
        case OP_CALL: {
            i = chunk_short_print(chunk, i);
            break;
        }
        case OP_GET_GLOBAL_LONG:
        case OP_SET_GLOBAL_LONG:
        case OP_DEFINE_GLOBAL_LONG:
        case OP_CONSTANT_LONG: {
            i = chunk_long_print(chunk, i);
            break;
        }
        }
    }
    printf("\n");
}

void chunk_print(Chunk* chunk) {
    printf("--------[ CHUNK DUMP: <GLOBAL> ]--------\n\n");
    standalone_chunk_print(chunk);
    for (int i = 0; i < chunk->constants.size; i++) {
        if (VALUE_IS_OBJ(chunk->constants.values[i])) {
            Obj* obj = VALUE_AS_OBJ(chunk->constants.values[i]);
            if (OBJ_IS_FUNCTION(obj)) {
                ObjFunction* fn = OBJ_AS_FUNCTION(obj);
                printf("--------[ CHUNK DUMP: '%s' ]--------\n\n", OBJ_AS_CSTRING(fn->name));
                standalone_chunk_print(&fn->chunk);
            }
        }
    }
}

static const char* token_type_print(TokenKind kind) {
    switch (kind) {
    case TOKEN_END: return "TokenEnd";
    case TOKEN_ERROR: return "TokenError";
    case TOKEN_PLUS: return "TokenPlus";
    case TOKEN_MINUS: return "TokenMinus";
    case TOKEN_STAR: return "TokenStar";
    case TOKEN_SLASH: return "TokenSlash";
    case TOKEN_PERCENT: return "TokenPercent";
    case TOKEN_LEFT_PAREN: return "TokenLeftParen";
    case TOKEN_RIGHT_PAREN: return "TokenRightParen";
    case TOKEN_LEFT_BRACE: return "TokenLeftBrace";
    case TOKEN_RIGHT_BRACE: return "TokenRightBrace";
    case TOKEN_DOT: return "TokenDot";
    case TOKEN_EQUAL: return "TokenEqual";
    case TOKEN_BANG: return "TokenBang";
    case TOKEN_AND: return "TokenAnd";
    case TOKEN_OR: return "TokenOr";
    case TOKEN_FUNCTION: return "TokenFunction";
    case TOKEN_VAR: return "TokenVar";
    case TOKEN_NUMBER: return "TokenNumber";
    case TOKEN_TRUE: return "TokenTrue";
    case TOKEN_FALSE: return "TokenFalse";
    case TOKEN_STRING: return "TokenString";
    case TOKEN_NIL: return "TokenNil";
    case TOKEN_EQUAL_EQUAL: return "TokenEqualEqual";
    case TOKEN_BANG_EQUAL: return "TokenBangEqual";
    case TOKEN_LOWER: return "TokenLower";
    case TOKEN_GREATER: return "TokenGreater";
    case TOKEN_LOWER_EQUAL: return "TokenLowerEqual";
    case TOKEN_GREATER_EQUAL: return "TokenGreaterEqual";
    case TOKEN_IDENTIFIER: return "TokenIdentifier";
    case TOKEN_PRINT: return "TokenPrint";
    case TOKEN_SEMICOLON: return "TokenSemicolon";
    case TOKEN_COLON: return "TokenColon";
    case TOKEN_TYPE_NUMBER: return "TokenNumberType";
    case TOKEN_TYPE_STRING: return "TokenStringType";
    case TOKEN_TYPE_BOOL: return "TokenBoolType";
    case TOKEN_TYPE_NIL: return "TokenNilType";
    case TOKEN_COMMA: return "TokenComma";
    case TOKEN_RETURN: return "TokenReturn";
    default: return "Unknown";
    }
}

void token_print(Token token) {
    printf(
        "Token{ Type: '%s', Line: '%d', Value: '%.*s', Length: '%d' }\n",
        token_type_print(token.kind),
        token.line,
        token.length,
        token.start,
        token.length);
}

static void print_offset();
static void pretty_print(const char *msg, ...);

static void print_binary(void* ctx, BinaryExpr* binary);
static void print_unary(void* ctx, UnaryExpr* unary);
static void print_literal(void* ctx, LiteralExpr* literal);
static void print_identifier(void* ctx, IdentifierExpr* identifier);
static void print_assignment(void* ctx, AssignmentExpr* assignment);
static void print_call(void* ctx, CallExpr* call);

ExprVisitor printer_expr_visitor = (ExprVisitor){
    .visit_literal = print_literal,
    .visit_binary = print_binary,
    .visit_unary = print_unary,
    .visit_identifier = print_identifier,
    .visit_assignment = print_assignment,
    .visit_call = print_call,
};

static void print_expr(void* ctx, ExprStmt* expr);
static void print_var(void* ctx, VarStmt* var);
static void print_print(void* ctx, PrintStmt* var);
static void print_block(void* ctx, BlockStmt* block);
static void print_function(void* ctx, FunctionStmt* function);
static void print_return(void* ctx, ReturnStmt* return_);

StmtVisitor printer_stmt_visitor = (StmtVisitor){
    .visit_expr = print_expr,
    .visit_var = print_var,
    .visit_print = print_print,
    .visit_block = print_block,
    .visit_function = print_function,
    .visit_return = print_return,
};

#define ACCEPT_STMT(stmt) stmt_dispatch(&printer_stmt_visitor, NULL, stmt)
#define ACCEPT_EXPR(expr) expr_dispatch(&printer_expr_visitor, NULL, expr)

int offset = 0;

#define OFFSET(...) do { offset++; __VA_ARGS__ offset--; } while(false)

void ast_print(Stmt* root) {
    ACCEPT_STMT(root);
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

static void print_block(void* ctx, BlockStmt* block) {
    pretty_print("Block: {\n");
    OFFSET({
        ACCEPT_STMT(block->stmts);
    });
    pretty_print("}\n");
}

static void print_print(void* ctx, PrintStmt* print) {
    pretty_print("Print: [\n");
    OFFSET({
        ACCEPT_EXPR(print->inner);
    });
    pretty_print("]\n");
}

static void print_expr(void* ctx, ExprStmt* expr) {
    pretty_print("Expr Stmt: [\n");
    OFFSET({
        ACCEPT_EXPR(expr->inner);
    });
    pretty_print("]\n");
}

static void print_return(void* ctx, ReturnStmt* return_) {
    pretty_print("Return Stmt: [\n");
    OFFSET({
        ACCEPT_EXPR(return_->inner);
    });
    pretty_print("]\n");
}

static void print_var(void* ctx, VarStmt* var) {
    pretty_print("Var Stmt: [\n");
    OFFSET({
        pretty_print("Identifier: ");
        token_print(var->identifier);
        pretty_print("Definition: \n");
        OFFSET({
            ACCEPT_EXPR(var->definition);
        });
    });
    pretty_print("]\n");
}

static void print_binary(void* ctx, BinaryExpr* binary) {
    pretty_print("Binary: [\n");
    OFFSET({
        pretty_print("Left:\n");
        OFFSET({
            ACCEPT_EXPR(binary->left);
        });

        pretty_print("Operator: ");
        token_print(binary->op);

        pretty_print("Right: \n");
        OFFSET({
            ACCEPT_EXPR(binary->right);
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

static void print_identifier(void* ctx, IdentifierExpr *identifier) {
    pretty_print("Identifier Expr: [\n");
    OFFSET({
        pretty_print("Value: ");
        token_print(identifier->name);
    });
    pretty_print("]\n");
}

static void print_assignment(void* ctx, AssignmentExpr* assignment) {
    pretty_print("Assignment Expr: [\n");
    OFFSET({
        pretty_print("Variable: ");
        token_print(assignment->name);
        pretty_print("Value:\n");
        OFFSET({
            ACCEPT_EXPR(assignment->value);
        });
    });
    pretty_print("]\n");
}

static void print_call(void* ctx, CallExpr* call) {
    pretty_print("Call Expr: [\n");
    OFFSET({
        pretty_print("Function name: ");
        token_print(call->identifier);
        pretty_print("Params: [\n");
        OFFSET({
            for (int i = 0; i < call->params.size; i++) {
                ACCEPT_EXPR(call->params.params[i].expr);
            }
        });
        pretty_print("]\n");
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
            ACCEPT_EXPR(unary->expr);
        });
    });
    pretty_print("]\n");
}

static void print_function(void* ctx, FunctionStmt* function) {
    pretty_print("Function '");
    printf("%.*s' [\n", function->identifier.length, function->identifier.start);
    OFFSET({
        pretty_print("Body: \n");
        OFFSET({
            ACCEPT_STMT(function->body);
        });
    });
    pretty_print("]\n");
}
