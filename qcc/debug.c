#include "debug.h"
#include <stdarg.h>
#include "expr.h"
#include "type.h"
#include "object.h"

void table_print(const Table* table) {
    printf("\t| Key\t\t| Value\n");
    printf("\t|---------------|-----------------\n");
    for (int i = 0; i < table->capacity; i++) {
        if (! IS_ENTRY_EMPTY(table, i)) {
            printf("\t|%s\t\t|", OBJ_AS_CSTRING(table->entries[i].key));
            value_print(table->entries[i].value);
            printf("\n");
        }
    }
    printf("\n\n");
}

static void upvalues_print(const SymbolSet* set) {
    if (SYMBOL_SET_SIZE(set) == 0) {
        printf("Empty\t");
        return;
    }
    SYMBOL_SET_FOREACH(set, {
        Symbol* current = elements[i];
        printf(
            "'%.*s'(%d), ",
            SYMBOL_NAME_LENGTH(current->name),
            SYMBOL_NAME_START(current->name),
            current->declaration_line);
    });
}

static void symbol_upvalues_print(const Symbol* symbol) {
    upvalues_print(symbol->upvalue_fn_refs);
}

static void fn_upvalues_print(const Symbol* symbol) {
    if (symbol->kind != SYMBOL_FUNCTION) {
        printf("Not a function");
        return;
    }
    upvalues_print(symbol->function.upvalues);
}

static void symbol_table_print(const SymbolTable* table) {
    printf("--------[ SYMBOL TABLE ]--------\n\n");
    printf("| Name\t| Line \t| Type  \t| Global? \t| Symbol upvalues \t | Function upvalues\n");
    printf("|-------|-------|---------------|---------------|------------------------|------------\n");
    SYMBOL_TABLE_FOREACH(table, {
        Symbol* current = &elements[i];
        printf(
            "| %.*s\t| %d\t| ",
            SYMBOL_NAME_LENGTH(current->name),
            SYMBOL_NAME_START(current->name),
            current->declaration_line);
        TYPE_PRINT(current->type);

        printf("\t| %s\t", (current->global ? "Yes" : "No"));

        printf("\t| ");
        symbol_upvalues_print(current);

        printf("\t| ");
        fn_upvalues_print(current);

        printf("\n");
    });
    printf("\n\n");
}

static void symbol_node_print(const SymbolNode* node) {
    symbol_table_print(&node->symbols);
    printf("SCOPE CHILDS: %d\n", node->childs.size);
    SymbolNode* childs = VECTOR_AS_SYMBOL_NODE(&node->childs);
    for (uint32_t i = 0; i < node->childs.size; i++) {
        symbol_node_print(&childs[i]);
    }
}

void scoped_symbol_table_print(const ScopedSymbolTable* table) {
    symbol_node_print(&table->global);
}

void valuearray_print(const ValueArray* values) {
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
    "OP_GET_UPVALUE",
    "OP_SET_UPVALUE",
    "OP_BIND_UPVALUE",
    "OP_CLOSE",
    "OP_BIND_CLOSED",
    "OP_JUMP",
    "OP_JUMP_IF_FALSE",
    "OP_NEW",
    "OP_GET_PROP",
};

void opcode_print(uint8_t op) {
    printf("%s\n", OpCodeStrings[op]);
}

void stack_print(const Value* stack_top, Value* stack) {
    Value* current = stack;
    while (current < stack_top) {
        printf("[ ");
        value_print(*current);
        printf(" ] ");
        current = current + 1;
    }
}

static void chunk_format_print(const Chunk* chunk, int i, const char* format, ...);
static void chunk_value_print(const Chunk* chunk, int index);
static int chunk_opcode_print(const Chunk* chunk, int i);
static int chunk_short_print(const Chunk* chunk, int i);
static int chunk_long_print(const Chunk* chunk, int i);
static void standalone_chunk_print(const Chunk* chunk);
static void chunk_print_value(Value value);

static void chunk_format_print(const Chunk* chunk, int i, const char* format, ...) {
    printf("[%02d;%02d]\t", i, chunk->lines[i]);
    va_list params;
    va_start(params, format);
    vprintf(format, params);
    va_end(params);
}

static void chunk_value_print(const Chunk* chunk, int index) {
    Value val = chunk->constants.values[index];
    value_print(val);
    printf("\n");
}

static int chunk_opcode_print(const Chunk* chunk, int i) {
    chunk_format_print(chunk, i, "%s\n", OpCodeStrings[chunk->code[i]]);
    return ++i;
}

static int chunk_short_print(const Chunk* chunk, int i) {
    chunk_format_print(chunk, i, "%04x\t", chunk->code[i]);
    if (chunk->code[i] < chunk->constants.size) {
        chunk_value_print(chunk, chunk->code[i]);
    } else {
        printf("\n");
    }
    return ++i;
}

static int chunk_long_print(const Chunk* chunk, int i) {
    uint8_t* pc = &chunk->code[i];
    uint16_t num = read_long(&pc);
    i++;
    chunk_format_print(chunk, i, "%04x\t", num);
    if (num < chunk->constants.size) {
        chunk_value_print(chunk, num);
    } else {
        printf("\n");
    }
    return ++i;
}

static void standalone_chunk_print(const Chunk* chunk) {
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
        case OP_GREATER:
        case OP_CLOSE:
        case OP_NEW:
        case OP_END: {
            i = chunk_opcode_print(chunk, i);
            break;
        }
        case OP_DEFINE_GLOBAL:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
        case OP_GET_UPVALUE:
        case OP_SET_UPVALUE:
        case OP_CONSTANT:
        case OP_GET_PROP:
        case OP_CALL: {
            i = chunk_opcode_print(chunk, i);
            i = chunk_short_print(chunk, i);
            break;
        }
        case OP_GET_GLOBAL_LONG:
        case OP_SET_GLOBAL_LONG:
        case OP_DEFINE_GLOBAL_LONG:
        case OP_CONSTANT_LONG:
        case OP_JUMP:
        case OP_JUMP_IF_FALSE: {
            i = chunk_opcode_print(chunk, i);
            i = chunk_long_print(chunk, i);
            break;
        }
        case OP_BIND_UPVALUE: {
            i = chunk_opcode_print(chunk, i);
            i = chunk_short_print(chunk, i);
            i = chunk_short_print(chunk, i);
            break;
        }
        case OP_BIND_CLOSED: {
            i = chunk_opcode_print(chunk, i);
            i = chunk_short_print(chunk, i);
            break;
        }
        }
    }
    printf("\n");
}

static void chunk_print_with_name(const Chunk* chunk, const char* name) {
    printf("--------[ CHUNK DUMP: %s ]--------\n\n", name);
    valuearray_print(&chunk->constants);
    standalone_chunk_print(chunk);
    for (int i = 0; i < chunk->constants.size; i++) {
        chunk_print_value(chunk->constants.values[i]);
    }
}

static void chunk_print_value(Value value) {
    if (! VALUE_IS_OBJ(value)) {
        return;
    }
    Obj* obj = VALUE_AS_OBJ(value);
    if (OBJ_IS_FUNCTION(obj)) {
        ObjFunction* fn = OBJ_AS_FUNCTION(obj);
        char* name = OBJ_AS_CSTRING(fn->name);
        chunk_print_with_name(&fn->chunk, name);
    }
    if (OBJ_IS_CLASS(obj)) {
        ObjClass* klass = OBJ_AS_CLASS(obj);
        for (int i = 0; i < klass->instance.size ; i++) {
            chunk_print_value(klass->instance.values[i]);
        }
    }
}

void chunk_print(const Chunk* chunk) {
    chunk_print_with_name(chunk, "<GLOBAL>");
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
    case TOKEN_SEMICOLON: return "TokenSemicolon";
    case TOKEN_COLON: return "TokenColon";
    case TOKEN_TYPE_NUMBER: return "TokenNumberType";
    case TOKEN_TYPE_STRING: return "TokenStringType";
    case TOKEN_TYPE_BOOL: return "TokenBoolType";
    case TOKEN_TYPE_NIL: return "TokenNilType";
    case TOKEN_COMMA: return "TokenComma";
    case TOKEN_RETURN: return "TokenReturn";
    case TOKEN_IF: return "TokenIf";
    case TOKEN_ELSE: return "TokenElse";
    case TOKEN_FOR: return "TokenFor";
    case TOKEN_WHILE: return "TokenWhile";
    case TOKEN_BREAK: return "TokenBreak";
    case TOKEN_CONTINUE: return "TokenContinue";
    case TOKEN_IMPORT: return "TokenImport";
    case TOKEN_CLASS: return "TokenClass";
    case TOKEN_PUBLIC: return "TokenPublic";
    case TOKEN_NEW: return "TokenNew";
    default: return "Unknown";
    }
}

void token_print(Token token) {
    printf(
        "Token{ Type: '%s', Line: '%d', Col: '%d', Value: '%.*s', Length: '%d' }\n",
        token_type_print(token.kind),
        token.line,
        token.column,
        token.length,
        token.start,
        token.length);
}

static void print_offset();
static void pretty_print(const char *msg);

static void print_binary(void* ctx, BinaryExpr* binary);
static void print_unary(void* ctx, UnaryExpr* unary);
static void print_literal(void* ctx, LiteralExpr* literal);
static void print_identifier(void* ctx, IdentifierExpr* identifier);
static void print_assignment(void* ctx, AssignmentExpr* assignment);
static void print_call(void* ctx, CallExpr* call);
static void print_new(void* ctx, NewExpr* new_);

ExprVisitor printer_expr_visitor = (ExprVisitor){
    .visit_literal = print_literal,
    .visit_binary = print_binary,
    .visit_unary = print_unary,
    .visit_identifier = print_identifier,
    .visit_assignment = print_assignment,
    .visit_call = print_call,
    .visit_new = print_new,
};

static void print_expr(void* ctx, ExprStmt* expr);
static void print_var(void* ctx, VarStmt* var);
static void print_block(void* ctx, BlockStmt* block);
static void print_function(void* ctx, FunctionStmt* function);
static void print_return(void* ctx, ReturnStmt* return_);
static void print_if(void* ctx, IfStmt* if_);
static void print_for(void* ctx, ForStmt* for_);
static void print_while(void* ctx, WhileStmt* while_);
static void print_loopg(void* ctx, LoopGotoStmt* loopg);
static void print_typealias(void* ctx, TypealiasStmt* alias);
static void print_import(void* ctx, ImportStmt* import);
static void print_native(void* ctx, NativeFunctionStmt* native);
static void print_class(void* ctx, ClassStmt* klass);

StmtVisitor printer_stmt_visitor = (StmtVisitor){
    .visit_expr = print_expr,
    .visit_var = print_var,
    .visit_block = print_block,
    .visit_function = print_function,
    .visit_return = print_return,
    .visit_if = print_if,
    .visit_for = print_for,
    .visit_while = print_while,
    .visit_loopg = print_loopg,
    .visit_typealias = print_typealias,
    .visit_import = print_import,
    .visit_native = print_native,
    .visit_class = print_class,
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

static void pretty_print(const char *msg) {
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
    Expr** exprs = VECTOR_AS_EXPRS(&call->params);
    pretty_print("Call Expr: [\n");
    OFFSET({
        pretty_print("Function name: ");
        token_print(call->identifier);
        pretty_print("Params: [\n");
        OFFSET({
            for (uint32_t i = 0; i < call->params.size; i++) {
                ACCEPT_EXPR(exprs[i]);
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

static void print_new(void* ctx, NewExpr* new_) {
    Expr** exprs = VECTOR_AS_EXPRS(&new_->params);
    pretty_print("New: [\n");
    OFFSET({
        pretty_print("Class: ");
        token_print(new_->klass);
        pretty_print("Params: [\n");
        OFFSET({
            for (uint32_t i = 0; i < new_->params.size; i++) {
                ACCEPT_EXPR(exprs[i]);
            }
        });
        pretty_print("]\n");
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

static void print_if(void* ctx, IfStmt* if_) {
    pretty_print("If: [\n");
    OFFSET({
        pretty_print("Token: ");
        token_print(if_->token);
        pretty_print("Condition: \n");
        OFFSET({
            ACCEPT_EXPR(if_->condition);
        });
        pretty_print("Then: [\n");
        OFFSET({
            ACCEPT_STMT(if_->then);
        });
        pretty_print("]\n");
        pretty_print("Else: [\n");
        OFFSET({
            if (if_->else_ != NULL) {
                ACCEPT_STMT(if_->else_);
            }
        });
        pretty_print("]\n");
    });
    pretty_print("]\n");
}

static void print_for(void* ctx, ForStmt* for_) {
    pretty_print("for: [\n");
    OFFSET({
        pretty_print("Token: ");
        token_print(for_->token);
        pretty_print("Init: [\n");
        OFFSET({
            if (for_->init != NULL) {
                ACCEPT_STMT(for_->init);
            }
        });
        pretty_print("]\n");
        pretty_print("Condition: \n");
        OFFSET({
            if (for_->condition != NULL) {
                ACCEPT_EXPR(for_->condition);
            }
        });
        pretty_print("Mod: [\n");
        OFFSET({
            if (for_->mod != NULL) {
                ACCEPT_STMT(for_->mod);
            }
        });
        pretty_print("]\n");
        pretty_print("Body: [\n");
        OFFSET({
            ACCEPT_STMT(for_->body);
        });
        pretty_print("]\n");
    });
    pretty_print("]\n");
}

static void print_while(void* ctx, WhileStmt* while_) {
    pretty_print("while: [\n");
    OFFSET({
        pretty_print("Token: ");
        token_print(while_->token);
        pretty_print("Condition: \n");
        OFFSET({
            if (while_->condition != NULL) {
                ACCEPT_EXPR(while_->condition);
            }
        });
        pretty_print("Body: [\n");
        OFFSET({
            ACCEPT_STMT(while_->body);
        });
        pretty_print("]\n");
    });
    pretty_print("]\n");
}

static void print_loopg(void* ctx, LoopGotoStmt* loopg) {
    pretty_print("Loop Goto: [\n");
    OFFSET({
        pretty_print("Token: ");
        token_print(loopg->token);
        pretty_print("Kind: ");
        printf("%s\n", (loopg->kind == LOOP_BREAK) ? "BREAK" : "CONTINUE");
    });
    pretty_print("]\n");
}

static void print_typealias(void* ctx, TypealiasStmt* alias) {
    pretty_print("Type Alias: [\n");
    OFFSET({
        pretty_print("Identifier: ");
        token_print(alias->identifier);
    });
    pretty_print("]\n");
}

static void print_import(void* ctx, ImportStmt* import) {
    pretty_print("Import: [\n");
    OFFSET({
        pretty_print("File: ");
        token_print(import->filename);
        pretty_print("AST: [\n");
        OFFSET({
            if (import->ast == NULL) {
                pretty_print("<Already imported>\n");
            } else {
                ACCEPT_STMT(import->ast);
            }
        });
        pretty_print("]\n");
    });
    pretty_print("]\n");
}

static void print_native(void* ctx, NativeFunctionStmt* native) {
    pretty_print("Native Function: [\n");
    OFFSET({
        pretty_print("Name: ");
        printf("'%.*s'\n", native->length, native->name);
    });
    pretty_print("]\n");
}

static void print_class(void* ctx, ClassStmt* klass) {
    pretty_print("Class: [\n");
    OFFSET({
        pretty_print("Identifier: ");
        token_print(klass->identifier);
        pretty_print("Body: [\n");
        OFFSET({
            ACCEPT_STMT(klass->body);
        });
        pretty_print("]\n");
    });
    pretty_print("]\n");
}

