#include "parser.h"
#include <stdarg.h>
#include "common.h"
#include "vector.h"
#include "symbol.h"
#include "type.h"
#include "error.h"
#include "import.h"
#include "array.h"
#include "string.h"

#ifdef PARSER_DEBUG
#include "debug.h"
#endif

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_CAST,        // cast
    PREC_PRIMARY
} Precedence;

typedef Expr* (*PrefixParse)(Parser* const parser, bool can_assign);
typedef Expr* (*SuffixParse)(Parser* const parser, bool can_assign, Expr* left);

typedef struct {
    PrefixParse prefix;
    SuffixParse infix;
    Precedence precedence;
} ParseRule;

static ParseRule* get_rule(TokenKind kind);
static Expr* parse_precendence(Parser* const parser, Precedence precedence);

static void error(Parser* const parser, const char* message, ...);
static void error_prev(Parser* const parser, const char* message, ...);
static void error_at(Parser* const parser, Token* token, const char* message, va_list* params);
static void syncronize(Parser* const parser);

#define ERROR_AT(parser, token, msg) (error_at(parser, token, msg, NULL))

static void create_scope(Parser* const parser);
static void create_class_scope(Parser* const parser);
static void end_scope(Parser* const parser);
static Symbol* current_scope_lookup(Parser* const parser, SymbolName* name);
static Symbol* lookup_str(Parser* const parser, const char* name, int length);
static Symbol* lookup_with_class_str(Parser* const parser, const char* name, int length);
static void insert(Parser* const parser, Symbol entry);
static bool register_symbol(Parser* const parser, Symbol symbol);
static Symbol create_symbol_calc_global(Parser* const parser, Token* token, Type* type);
static void symbol_update_class_body(Parser* const parser, Symbol* obj);

static void advance(Parser* const parser);
static bool consume(Parser* const parser, TokenKind expected, const char* msg);

static Symbol* get_identifier_symbol(Parser* const parser, Token identifier);

static Stmt* parse_global(Parser* parser);
static Stmt* declaration_block(Parser* const parser, TokenKind limit_token);
static void write_declaration_block(Parser* const parser, TokenKind limit, ListStmt* write);

static Stmt* native_class(Parser* const parser, NativeClassStmt (*register_fn)(ScopedSymbolTable* const table));
static Stmt* declaration(Parser* const parser);
static Stmt* parse_variable(Parser* const parser);
static Stmt* variable_decl(Parser* const parser);
static Stmt* function_decl(Parser* const parser);
static Stmt* typealias_decl(Parser* const parser);
static Stmt* import_decl(Parser* const parser);
static Stmt* class_decl(Parser* const parser);
static Stmt* parse_class_body(Parser* const parser, Symbol* klass_sym);
static SymbolVisibility parse_property_visibility(Parser* const parser);
static Stmt* native_import(Parser* const parser, NativeImport import, int line, int column);
static Stmt* file_import(Parser* const parser, FileImport import);
static void parse_function_body(Parser* const parser, FunctionStmt* fn, Symbol* fn_sym);
static void parse_function_params_declaration(Parser* const parser, Symbol* symbol);
static void add_params_to_body(Parser* const parser, Symbol* fn_sym);
static Type* parse_type(Parser* const parser);
static Type* parse_array_type(Parser* const parser);
static Type* parse_function_type(Parser* const parser);

static Stmt* statement(Parser* const parser);
static Stmt* block_stmt(Parser* const parser);
static Stmt* return_stmt(Parser* const parser);
static Stmt* if_stmt(Parser* const parser);
static Stmt* for_stmt(Parser* const parser);
static Stmt* while_stmt(Parser* const parser);
static Stmt* loopg_stmt(Parser* const parser);
static Stmt* expr_stmt(Parser* const parser);

static void parse_for_init(Parser* const parser, ForStmt* for_stmt);
static void parse_for_condition(Parser* const parser, ForStmt* for_stmt);
static void parse_for_mod(Parser* const parser, ForStmt* for_stmt);

static Expr* expression(Parser* const parser);
static Expr* grouping(Parser* const parser, bool can_assign);
static Expr* primary(Parser* const parser, bool can_assign);
static Expr* identifier(Parser* const parser, bool can_assign);
static Expr* self(Parser* const parser, bool can_assign);
static Expr* unary(Parser* const parser, bool can_assign);
static Expr* new_(Parser* const parser, bool can_assign);
static Expr* arr(Parser* const parser, bool can_assign);
static Expr* cast(Parser* const parser, bool can_assign);
static Expr* binary(Parser* const parser, bool can_assign, Expr* left);
static Expr* call(Parser* const parser, bool can_assign, Expr* left);
static Expr* prop(Parser* const parser, bool can_assign, Expr* left);

static void parse_call_params(Parser* const parser, Vector* params);
static void parse_expression_list(Parser* const parser, Vector* params, TokenKind end, const char* error_end_missing);

ParseRule rules[] = {
    [TOKEN_END]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,        NULL,   PREC_NONE},

    [TOKEN_PLUS]          = {unary,       binary, PREC_TERM},
    [TOKEN_MINUS]         = {unary,       binary, PREC_TERM},
    [TOKEN_STAR]          = {NULL,        binary, PREC_FACTOR},
    [TOKEN_SLASH]         = {NULL,        binary, PREC_FACTOR},
    [TOKEN_PERCENT]       = {NULL,        binary, PREC_FACTOR},
    [TOKEN_LEFT_PAREN]    = {grouping,    call,   PREC_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,        NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,        prop,   PREC_CALL},
    [TOKEN_BANG]          = {unary,       NULL,   PREC_UNARY},
    [TOKEN_EQUAL]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_LOWER]         = {NULL,        binary, PREC_COMPARISON},
    [TOKEN_GREATER]       = {NULL,        binary, PREC_COMPARISON},
    [TOKEN_SEMICOLON]     = {NULL,        NULL,   PREC_NONE},
    [TOKEN_COLON]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,        NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,        NULL,   PREC_NONE},
    [TOKEN_LEFT_BRAKET]   = {arr,         NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRAKET]  = {NULL,        NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,        NULL,   PREC_NONE},

    [TOKEN_EQUAL_EQUAL]   = {NULL,        binary, PREC_EQUALITY},
    [TOKEN_BANG_EQUAL]    = {NULL,        binary, PREC_EQUALITY},
    [TOKEN_LOWER_EQUAL]   = {NULL,        binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,        binary, PREC_COMPARISON},

    [TOKEN_RETURN]        = {NULL,        NULL,   PREC_NONE},
    [TOKEN_FUNCTION]      = {NULL,        NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_TRUE]          = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_FALSE]         = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_AND]           = {NULL,        binary, PREC_AND},
    [TOKEN_OR]            = {NULL,        binary, PREC_OR},
    [TOKEN_NIL]           = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_STRING]        = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_IDENTIFIER]    = {identifier,  NULL,   PREC_NONE},
    [TOKEN_CONTINUE]      = {NULL,        NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,        NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,        NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_NEW]           = {new_,        NULL,   PREC_CALL},
    [TOKEN_TYPEDEF]       = {NULL,        NULL,   PREC_NONE},
    [TOKEN_IMPORT]        = {NULL,        NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_PUBLIC]        = {NULL,        NULL,   PREC_NONE},
    [TOKEN_SELF]          = {self,        NULL,   PREC_NONE},
    [TOKEN_CAST]          = {cast,        NULL,   PREC_CAST},

    [TOKEN_TYPE_ANY]      = {NULL,        NULL,   PREC_NONE},
    [TOKEN_TYPE_NUMBER]   = {NULL,        NULL,   PREC_NONE},
    [TOKEN_TYPE_STRING]   = {NULL,        NULL,   PREC_NONE},
    [TOKEN_TYPE_BOOL]     = {NULL,        NULL,   PREC_NONE},
    [TOKEN_TYPE_VOID]     = {NULL,        NULL,   PREC_NONE},
    [TOKEN_TYPE_NIL]      = {NULL,        NULL,   PREC_NONE},
};

#define IN_LOOP(parser, ...)\
    do {\
        bool prev = parser->is_in_loop;\
        parser->is_in_loop = true;\
        __VA_ARGS__\
        parser->is_in_loop = prev;\
    } while (false)

#define IS_IN_CLASS(parser) (parser->current_class_type != NULL)

#define IN_CLASS(parser, type, ...)\
    do {\
        Type* prev = parser->current_class_type;\
        parser->current_class_type = type;\
        __VA_ARGS__\
        parser->current_class_type = prev;\
    } while (false)

#define TRY_REGISTER_SYMBOL(parser, symbol, err)\
    do {\
        if (! register_symbol(parser, symbol)) {\
            free_symbol(&symbol);\
            if (err != NULL) {\
                error(parser, err);\
            }\
        }\
    } while (false)


static ParseRule* get_rule(TokenKind kind) {
    return &rules[kind];
}

static Expr* parse_precendence(Parser* const parser, Precedence precedence) {
    advance(parser);
    PrefixParse prefix_parser = get_rule(parser->prev.kind)->prefix;
    if (prefix_parser == NULL) {
        error_prev(parser, "Expected expression");
        return NULL;
    }
    bool can_assign = precedence <= PREC_ASSIGNMENT;
    Expr* left = prefix_parser(parser, can_assign);
    while (precedence <= get_rule(parser->current.kind)->precedence) {
        if (left == NULL) {
            break;
        }
        advance(parser);
        SuffixParse infix_parser = get_rule(parser->prev.kind)->infix;
        if (infix_parser == NULL) {
            break;
        }
        left = infix_parser(parser, can_assign, left);
    }
    return left;
}

void init_parser(Parser* const parser, FileImport ctx, ScopedSymbolTable* symbols) {
    parser->symbols = symbols;
    parser->current.kind = -1;
    parser->prev.kind = -1;
    init_lexer(&parser->lexer, ctx);
    parser->panic_mode = false;
    parser->has_error = false;
    parser->function_deep_count = 0;
    parser->scope_depth = 0;
    parser->is_in_loop = false;
    parser->current_class_type = NULL;
}

static void error(Parser* const parser, const char* message, ...) {
    va_list params;
    va_start(params, message);
    error_at(parser, &parser->current, message, &params);
    va_end(params);
}

static void error_prev(Parser* const parser, const char* message, ...) {
    va_list params;
    va_start(params, message);
    error_at(parser, &parser->prev, message, &params);
    va_end(params);
}

static void error_at(Parser* const parser, Token* token, const char* format, va_list* params) {
    if (parser->panic_mode) {
        return;
    }
    parser->panic_mode = true;
    fprintf(
        stderr,
        "[File: %.*s, Line %d] Error",
        parser->lexer.ctx.path_length,
        parser->lexer.ctx.path,
        token->line);
    switch(token->kind) {
    case TOKEN_ERROR: break;
    case TOKEN_END:
        fprintf(stderr, " at end: ");
        break;
    default:
        fprintf(stderr, " at '%.*s': ", token->length, token->start);
    }
    if (params != NULL) {
        vfprintf(stderr, format, *params);
    } else {
        fprintf(stderr, "%s", format);
    }
    fprintf(stderr, "\n");
    print_error_context(token);
    parser->has_error = true;
}

static void syncronize(Parser* const parser) {
    parser->panic_mode = false;
    for (;;) {
        switch (parser->current.kind) {
        case TOKEN_SEMICOLON:
            advance(parser); // consume semicolon
        case TOKEN_VAR:
        case TOKEN_FUNCTION:
        case TOKEN_CONTINUE:
        case TOKEN_BREAK:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_FOR:
        case TOKEN_RETURN:
        case TOKEN_END:
            return;
        default:
            advance(parser);
        }
    }
}

static void create_scope(Parser* const parser){
    parser->scope_depth++;
    symbol_create_scope(parser->symbols);
}

static void create_class_scope(Parser* const parser) {
    parser->scope_depth++;
    symbol_create_class_scope(parser->symbols);
}

static void end_scope(Parser* const parser){
    symbol_end_scope(parser->symbols);
    parser->scope_depth--;
}

static Symbol* current_scope_lookup(Parser* const parser, SymbolName* name){
    return symbol_lookup(&parser->symbols->current->symbols, name);
}

static Symbol* lookup_str(Parser* const parser, const char* name, int length){
    return scoped_symbol_lookup_str(parser->symbols, name, length);
}

static Symbol* lookup_with_class_str(Parser* const parser, const char* name, int length) {
    return scoped_symbol_lookup_with_class_str(parser->symbols, name, length);
}

static void insert(Parser* const parser, Symbol entry){
    scoped_symbol_insert(parser->symbols, entry);
}

static bool register_symbol(Parser* const parser, Symbol symbol) {
    Symbol* exsting = current_scope_lookup(parser, &symbol.name);
    if (exsting) {
        error_prev(parser, "Variable already declared in line %d", exsting->line);
        return false;
    }
    insert(parser, symbol);
    return true;
}

static Symbol create_symbol_calc_global(Parser* const parser, Token* token, Type* type) {
    Symbol symbol = create_symbol_from_token(token, type);
    symbol.global = parser->scope_depth == 0;
    return symbol;
}

static void symbol_update_class_body(Parser* const parser, Symbol* obj) {
    scoped_symbol_update_class_body(parser->symbols, obj);
}

static void advance(Parser* const parser) {
    if (parser->current.kind == TOKEN_END) {
        return;
    }
    parser->prev = parser->current;
    parser->current = next_token(&parser->lexer);
}

static bool consume(Parser* const parser, TokenKind expected, const char* message) {
    if (parser->current.kind != expected) {
        error(parser, message);
        return false;
    }
    advance(parser);
    return true;
}

static Symbol* get_identifier_symbol(Parser* const parser, Token identifier) {
    Symbol* existing = lookup_str(parser, identifier.start, identifier.length);
    if (!existing) {
        error_prev(parser, "Use of undeclared variable", identifier.length, identifier.start);
        return NULL;
    }
    if (existing->line > identifier.line) {
        error_prev(parser, "Use of variable '%.*s' before declaration", identifier.length, identifier.start);
        return NULL;
    }
    return existing;
}

Stmt* parse(Parser* const parser) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: Parser start\n");
#endif

    advance(parser);
    if (parser->current.kind == TOKEN_ERROR) {
        parser->has_error = true;
        return NULL;
    }
    if (parser->current.kind == TOKEN_END) {
        return NULL;
    }
    Stmt* ast = parse_global(parser);
#ifdef PARSER_DEBUG
    ast_print(ast);
#endif
    return ast;
}

static Stmt* parse_global(Parser* const parser) {
    ListStmt* list = create_stmt_list();

    stmt_list_add(list, native_class(parser, array_register));
    stmt_list_add(list, native_class(parser, string_register));

    write_declaration_block(parser, TOKEN_END, list);

    return CREATE_STMT_LIST(list);
}

static Stmt* declaration_block(Parser* const parser, TokenKind limit_token) {
    ListStmt* list = create_stmt_list();
    write_declaration_block(parser, limit_token, list);
    return CREATE_STMT_LIST(list);
}

static void write_declaration_block(Parser* const parser, TokenKind limit, ListStmt* write) {
    while (parser->current.kind != limit && parser->current.kind != TOKEN_END) {
        Stmt* stmt = declaration(parser);
        assert(stmt != NULL);
        if (parser->panic_mode) {
            // That stmt is not inside the abstract syntax tree. You need to free it.
            free_stmt(stmt);
            syncronize(parser);
        } else {
            stmt_list_add(write, stmt);
        }
    }
}

static Stmt* native_class(Parser* const parser, NativeClassStmt (*register_fn)(ScopedSymbolTable* const table)) {
    NativeClassStmt native = register_fn(parser->symbols);
    return CREATE_STMT_NATIVE_CLASS(native);
}

static Stmt* declaration(Parser* const parser) {
    switch (parser->current.kind) {
    case TOKEN_VAR:
        return variable_decl(parser);
    case TOKEN_FUNCTION:
        return function_decl(parser);
    case TOKEN_TYPEDEF:
        return typealias_decl(parser);
    case TOKEN_IMPORT:
        return import_decl(parser);
    case TOKEN_CLASS:
        return class_decl(parser);
    default:
        return statement(parser);
    }
}

static Stmt* statement(Parser* const parser) {
    switch (parser->current.kind) {
    case TOKEN_LEFT_BRACE:
        return block_stmt(parser);
    case TOKEN_RETURN:
        return return_stmt(parser);
    case TOKEN_IF:
        return if_stmt(parser);
    case TOKEN_FOR:
        return for_stmt(parser);
    case TOKEN_WHILE:
        return while_stmt(parser);
    case TOKEN_CONTINUE:
    case TOKEN_BREAK:
        return loopg_stmt(parser);
    default:
        return expr_stmt(parser);
    }
}

static Stmt* block_stmt(Parser* const parser) {
    consume(parser, TOKEN_LEFT_BRACE, "Expected block to start with '{'");
    BlockStmt block;
    create_scope(parser);
    block.stmts = declaration_block(parser, TOKEN_RIGHT_BRACE);
    consume(parser, TOKEN_RIGHT_BRACE, "Expected block to end with '}'");
    end_scope(parser);

    return CREATE_STMT_BLOCK(block);
}

static Stmt* parse_variable(Parser* const parser) {
    consume(parser, TOKEN_VAR, "Expected variable declaration to start with 'var'");
    if (parser->current.kind != TOKEN_IDENTIFIER) {
        error(parser, "Expected identifier to be var name");
    }
    VarStmt var;
    var.identifier = parser->current;
    advance(parser); // consume identifier

    Type* var_type = CREATE_TYPE_UNKNOWN();
    if (parser->current.kind == TOKEN_COLON) {
        advance(parser); // consume :
        var_type = parse_type(parser);
        if (TYPE_IS_UNKNOWN(var_type)) {
            error(parser, "Unkown type in variable declaration");
        }
        advance(parser); // consume type
    }

    var.definition = NULL;
    if (parser->current.kind == TOKEN_EQUAL) {
        advance(parser); // consume =
        var.definition = expression(parser);
    }

    Symbol symbol = create_symbol_calc_global(parser, &var.identifier, var_type);
    symbol.assigned = var.definition != NULL;
    symbol.global = parser->scope_depth == 0;
    TRY_REGISTER_SYMBOL(parser, symbol, NULL);

    return CREATE_STMT_VAR(var);
}

static Stmt* variable_decl(Parser* const parser) {
    Stmt* var = parse_variable(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected variable declaration to end with ';'");
    return var;
}

static Stmt* typealias_decl(Parser* const parser) {
    consume(parser, TOKEN_TYPEDEF, "Expected type alias to start with 'typedef'");

    TypealiasStmt stmt = (TypealiasStmt) {
        .identifier = parser->current,
    };
    advance(parser); // consume alias

    consume(parser, TOKEN_EQUAL, "Expected '=' after type alias name");
    Type* def = parse_type(parser);
    advance(parser); // consume type
    consume(parser, TOKEN_SEMICOLON, "Expected semicolon at the end of type alias");

    Type* alias = create_type_alias(
        stmt.identifier.start,
        stmt.identifier.length,
        def);
    Symbol symbol = create_symbol_calc_global(parser, &stmt.identifier, alias);
    symbol.kind = SYMBOL_TYPEALIAS;
    TRY_REGISTER_SYMBOL(parser, symbol, "Type alias already defined");

    return CREATE_STMT_TYPEALIAS(stmt);
}

static Stmt* import_decl(Parser* const parser) {
    consume(parser, TOKEN_IMPORT, "Expected import to start with 'import'");
    ImportStmt import_stmt = (ImportStmt) {
        .filename = parser->current,
        .ast = NULL,
    };
    advance(parser); // consume filename
    consume(parser, TOKEN_SEMICOLON, "Expected semicolon at end of import statment");
    Import imp = import(
        import_stmt.filename.start,
        import_stmt.filename.length);
    if (! imp.is_already_loaded) {
        import_stmt.ast = (imp.is_native) ?
            native_import(parser, imp.native, import_stmt.filename.line, import_stmt.filename.column) :
            file_import(parser, imp.file);
    }
    return CREATE_STMT_IMPORT(import_stmt);
}

static Stmt* native_import(Parser* const parser, NativeImport import, int line, int column) {
    ListStmt* list = create_stmt_list();
    for (int i = 0; i < import.functions_length; i++) {
        NativeFunction fn = import.functions[i];

        NativeFunctionStmt stmt = (NativeFunctionStmt) {
            .name = fn.name,
            .length = fn.length,
            .function = fn.function,
        };
        stmt_list_add(list, CREATE_STMT_NATIVE(stmt));

        // This symbol is not a function. Lets say its a variable
        // that holds an OBJ_NATIVE and acts like a function. We
        // must override the kind because it infers from the type
        // that is a function. create_symbol also reserves memory
        // in case you are a function, so we must lie him saying
        // we are TYPE_UNKNOWN.
        Symbol native_symbol = create_symbol(
            create_symbol_name(fn.name, fn.length),
            line,
            column,
            CREATE_TYPE_UNKNOWN());
        native_symbol.kind = SYMBOL_VAR;
        native_symbol.type = fn.type;
        native_symbol.global = parser->scope_depth == 0;
        native_symbol.native = true;
        register_symbol(parser, native_symbol);
    }
    return CREATE_STMT_LIST(list);
}

static Stmt* file_import(Parser* const parser, FileImport import) {
    if (import.source == NULL) {
        parser->has_error = true;
        return NULL;
    }
    Parser subparser;
    init_parser(&subparser, import, parser->symbols);
    Stmt* subast = parse(&subparser);
    if (subparser.has_error) {
        parser->has_error = true;
    }
    return subast;
}

static Stmt* class_decl(Parser* const parser) {
    consume(parser, TOKEN_CLASS, "Expected class declaration to start with 'class'");

    if (parser->current.kind != TOKEN_IDENTIFIER) {
        error(parser, "Expected identifier to be class name");
    }
    ClassStmt klass;
    klass.identifier = parser->current;

    Symbol symbol = create_symbol_calc_global(
        parser,
        &klass.identifier,
        create_type_class(klass.identifier.start, klass.identifier.length));
    TRY_REGISTER_SYMBOL(parser, symbol, "Class already defined");

    advance(parser); // Consume identifier

    Symbol* inserted = lookup_str(parser, klass.identifier.start, klass.identifier.length);
    assert(inserted != NULL);

    consume(parser, TOKEN_LEFT_BRACE, "Expected '{' after class name in class declaration");
    create_class_scope(parser);
    symbol_update_class_body(parser, inserted);
    IN_CLASS(parser, inserted->type, {
        klass.body = parse_class_body(parser, inserted);
    });
    end_scope(parser);
    consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' before class body");

    return CREATE_STMT_CLASS(klass);
}

static Stmt* parse_class_body(Parser* const parser, Symbol* klass_sym) {
    ListStmt* list = create_stmt_list();
    while (parser->current.kind != TOKEN_RIGHT_BRACE) {
        SymbolVisibility visibility = parse_property_visibility(parser);
        Stmt* stmt = NULL;
        Token identifier;

        switch (parser->current.kind) {
        case TOKEN_VAR:
            stmt = variable_decl(parser);
            if (stmt->var.definition != NULL) {
                ERROR_AT(
                    parser,
                    &stmt->var.identifier,
                    "Class variable properties cannot be initialized!");
            }
            identifier = stmt->var.identifier;
            break;
        case TOKEN_FUNCTION:
            stmt = function_decl(parser);
            identifier = stmt->function.identifier;
            break;
        default:
            error(parser, "Unexpected token inside class body");
            return CREATE_STMT_LIST(list);
        }

        Symbol* sym = lookup_with_class_str(parser, identifier.start, identifier.length);
        assert(sym != NULL);
        sym->visibility = visibility;
        stmt_list_add(list, stmt);
    }
    return CREATE_STMT_LIST(list);
}

static SymbolVisibility parse_property_visibility(Parser* const parser) {
    if (parser->current.kind == TOKEN_PUBLIC) {
        advance(parser); // consume pub token
        return SYMBOL_VISIBILITY_PUBLIC;
    }
    return SYMBOL_VISIBILITY_PRIVATE;
}

static Stmt* function_decl(Parser* const parser) {
    consume(parser, TOKEN_FUNCTION, "Expected function declaration to start with 'fn'");

    if (parser->current.kind != TOKEN_IDENTIFIER) {
        error(parser, "Expected identifier to be function name");
    }
    FunctionStmt fn = (FunctionStmt){
        .identifier = parser->current,
    };
    Symbol symbol = create_symbol_calc_global(parser, &fn.identifier, create_type_function());

    advance(parser); // consume identifier
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after function name in function declaration");
    if (parser->current.kind != TOKEN_RIGHT_PAREN) {
        parse_function_params_declaration(parser, &symbol);
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after function params in declaration");

    if (parser->current.kind == TOKEN_COLON) {
        advance(parser); // consume colon
        Type* return_type = parse_type(parser);
        if (TYPE_IS_UNKNOWN(return_type)) {
            error(
                parser,
                "Unknown return type in function '%.*s'",
                fn.identifier.length,
                fn.identifier.start);
        }
        TYPE_FN_RETURN(symbol.type) = return_type;
        advance(parser); // consume type
    }

    // Insert symbol before parsing the function body
    // so you can call a function inside a function
    TRY_REGISTER_SYMBOL(parser, symbol, NULL);
    Symbol* registered = lookup_with_class_str(parser, fn.identifier.start, fn.identifier.length);
    assert(registered != NULL);

    parse_function_body(parser, &fn, registered);

    return CREATE_STMT_FUNCTION(fn);
}

static void parse_function_body(Parser* const parser, FunctionStmt* fn, Symbol* fn_sym) {
    create_scope(parser);
    add_params_to_body(parser, fn_sym);
    parser->function_deep_count++;
    fn->body = block_stmt(parser);
    parser->function_deep_count--;
    end_scope(parser);
}

static void parse_function_params_declaration(Parser* const parser, Symbol* fn_sym) {
    for (;;) {
        if (parser->current.kind != TOKEN_IDENTIFIER) {
            error(parser, "Expected to have an identifier in parameter in function declaration");
        }
        VECTOR_ADD_TOKEN(&fn_sym->function.param_names, parser->current);
        advance(parser); // cosume param identifier
        consume(parser, TOKEN_COLON, "Expected function parameter to have a type in function declaration");
        Type* type = parse_type(parser);
        if (TYPE_IS_UNKNOWN(type)) {
            error(parser, "Unknown type in function param in function declaration");
        }
        VECTOR_ADD_TYPE(&TYPE_FN_PARAMS(fn_sym->type), type);
        advance(parser); // consume type
        if (parser->current.kind != TOKEN_COMMA) {
            break;
        }
        advance(parser); // consume comma
    }
}

static void add_params_to_body(Parser* const parser, Symbol* fn_sym) {
    if (IS_IN_CLASS(parser)) {
        Symbol self = create_symbol(
            create_symbol_name(
                CLASS_SELF_NAME,
                CLASS_SELF_LENGTH),
            fn_sym->line,
            fn_sym->column,
            create_type_object(parser->current_class_type));
        TRY_REGISTER_SYMBOL(parser, self, NULL);
    }

    Token* param_names = VECTOR_AS_TOKENS(&fn_sym->function.param_names);
    Type** param_types = VECTOR_AS_TYPES(&TYPE_FN_PARAMS(fn_sym->type));
    for (uint32_t i = 0; i < fn_sym->function.param_names.size; i++) {
        Symbol param = create_symbol_from_token(
            &param_names[i],
            param_types[i]);
        TRY_REGISTER_SYMBOL(parser, param, NULL);
    }
}

static Type* parse_type(Parser* const parser) {
    Type* simple_type = simple_type_from_token_kind(parser->current.kind);
    if (! TYPE_IS_UNKNOWN(simple_type)) {
        return simple_type;
    }
    if (parser->current.kind == TOKEN_LEFT_PAREN) {
        return parse_function_type(parser);
    }
    if (parser->current.kind == TOKEN_LEFT_BRAKET) {
        return parse_array_type(parser);
    }
    if (parser->current.kind != TOKEN_IDENTIFIER) {
        return CREATE_TYPE_UNKNOWN();
    }
    // We don't create new types here. If the type is an identifier
    // it should have been declared before.
    Symbol* symbol = lookup_str(parser, parser->current.start, parser->current.length);
    if (symbol == NULL) {
        error(
            parser,
            "The type '%.*s' is not defined",
            parser->current.length,
            parser->current.start);
        return CREATE_TYPE_UNKNOWN();
    }
    if (TYPE_IS_CLASS(symbol->type)) {
        return create_type_object(symbol->type);
    }
    return symbol->type;
}

static Type* parse_array_type(Parser* const parser) {
    consume(parser, TOKEN_LEFT_BRAKET, "Expected left braket in array type");
    consume(parser, TOKEN_RIGHT_BRAKET, "Expected right braket in array type");
    Type* inner = parse_type(parser);
    // advance(parser); // consume type
    return create_type_array(inner);
}

static Type* parse_function_type(Parser* const parser) {
    Type* fn_type = create_type_function();
    consume(parser, TOKEN_LEFT_PAREN, "Expected left paren in function type");
    if (parser->current.kind != TOKEN_RIGHT_PAREN) {
        for (;;) {
            Type* param = parse_type(parser);
            advance(parser); // consume simple type
            if (TYPE_IS_UNKNOWN(param)) {
                error_prev(parser, "Unkown type in param in function type declaration");
            }
            if (TYPE_IS_VOID(param)) {
                error_prev(parser, "You can't use Void type in params of function type declaration");
            }
            VECTOR_ADD_TYPE(& TYPE_FN_PARAMS(fn_type), param);
            if (parser->current.kind != TOKEN_COMMA) {
                break;
            }
            advance(parser); // consume comma
        }
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ) at end of function param types declaration");
    consume(parser, TOKEN_COLON, "Expected return type in function type declaration");
    Type* return_type = parse_type(parser);
    if (TYPE_IS_UNKNOWN(return_type)) {
        error(parser, "Unkown type in return in function type declaration");
    }
    TYPE_FN_RETURN(fn_type) = return_type;
    return fn_type;
}

static Stmt* return_stmt(Parser* const parser) {
    if (parser->function_deep_count == 0) {
        error(parser, "Cannot use return outside a function!");
    }
    consume(parser, TOKEN_RETURN, "Expected return statement to start with 'return'");
    ReturnStmt return_stmt;
    if (parser->current.kind == TOKEN_SEMICOLON) {
        advance(parser); // consume semicolon
        return_stmt.inner = NULL;
    } else {
        return_stmt.inner = expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expected return statment to end with ';'");
    }
    return CREATE_STMT_RETURN(return_stmt);
}

static Stmt* if_stmt(Parser* const parser) {
    Token token = parser->current;
    consume(parser, TOKEN_IF, "Expected if statement to start with 'if'");
    consume(parser, TOKEN_LEFT_PAREN, "expected left paren in if condition");
    Expr* condition = expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "expected right paren in if condition");
    Stmt* then = statement(parser);
    Stmt* else_ = NULL;
    if (parser->current.kind == TOKEN_ELSE) {
        advance(parser); // consume else
        else_ = statement(parser);
    }
    IfStmt if_stmt = (IfStmt){
        .token = token,
        .condition = condition,
        .then = then,
        .else_= else_,
    };
    return CREATE_STMT_IF(if_stmt);
}

static Stmt* while_stmt(Parser* const parser) {
    WhileStmt while_stmt;
    while_stmt.token = parser->current;
    consume(parser, TOKEN_WHILE, "Expected while statement to start with 'while'");
    consume(parser, TOKEN_LEFT_PAREN, "expected left paren before while condition");
    while_stmt.condition = expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "expected right paren after while condition");
    IN_LOOP(parser, {
        while_stmt.body = statement(parser);
    });
    return CREATE_STMT_WHILE(while_stmt);
}

static Stmt* loopg_stmt(Parser* const parser) {
    if (!parser->is_in_loop) {
        error(parser, "Expected break/continue statement to be inside a loop");
    }
    LoopGotoStmt loopg;
    loopg.token = parser->current;
    loopg.kind = (parser->current.kind == TOKEN_BREAK)
        ? LOOP_BREAK
        : LOOP_CONTINUE;
    advance(parser); // consume break or continue
    consume(parser, TOKEN_SEMICOLON, "expected break/continue statement to end with semicolon");
    return CREATE_STMT_LOOPG(loopg);
}

static Stmt* for_stmt(Parser* const parser) {
    ForStmt for_stmt;
    for_stmt.token = parser->current;

    // We need a additional scope here because
    // a for stmt can delcare variables in its init
    // part, and those variables should be local
    // to the for body.
    create_scope(parser);

    consume(parser, TOKEN_FOR, "Expected for statement to start with 'for'");
    consume(parser, TOKEN_LEFT_PAREN, "expected left paren in for condition");

    parse_for_init(parser, &for_stmt);
    parse_for_condition(parser, &for_stmt);
    parse_for_mod(parser, &for_stmt);

    consume(parser, TOKEN_RIGHT_PAREN, "expected right paren in for condition");
    IN_LOOP(parser, {
        for_stmt.body = statement(parser);
    });

    end_scope(parser);

    return CREATE_STMT_FOR(for_stmt);
}

static void parse_for_init(Parser* const parser, ForStmt* for_stmt) {
    for_stmt->init = NULL;
    if (parser->current.kind == TOKEN_RIGHT_PAREN) {
        error(parser, "expected ';' after init in for");
        return;
    }
    if (parser->current.kind == TOKEN_SEMICOLON) {
        advance(parser); // consume semicolon
        return;
    }

    ListStmt* vars = create_stmt_list();
    for (;;) {
        Stmt* var = parse_variable(parser);
        stmt_list_add(vars, var);
        if (parser->current.kind == TOKEN_SEMICOLON) {
            break;
        }
        consume(parser, TOKEN_COMMA, "expected ',' between var initialization in for");
        if (parser->has_error) {
            break;
        }
    }
    consume(parser, TOKEN_SEMICOLON, "expected ';' at end of var initialization in for");
    for_stmt->init = CREATE_STMT_LIST(vars);
}

static void parse_for_condition(Parser* const parser, ForStmt* for_stmt) {
    for_stmt->condition = NULL;
    if (parser->current.kind == TOKEN_RIGHT_PAREN) {
        error(parser, "expected ';' after condition in for");
        return;
    }
    if (parser->current.kind != TOKEN_SEMICOLON) {
        for_stmt->condition = expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON, "expected ';' at end of condition in for");
}

static void parse_for_mod(Parser* const parser, ForStmt* for_stmt) {
    if (parser->current.kind == TOKEN_RIGHT_PAREN) {
        for_stmt->mod = NULL;
        return;
    }

    ListStmt* mods = create_stmt_list();
    for (;;) {
        Expr* expr = expression(parser);
        stmt_list_add(mods, CREATE_STMT_EXPR(expr));
        if (parser->current.kind == TOKEN_RIGHT_PAREN) {
            break;
        }
        consume(parser, TOKEN_COMMA, "expected ',' between var initialization in for");
        if (parser->has_error) {
            break;
        }
    }
    for_stmt->mod = CREATE_STMT_LIST(mods);
}

static Stmt* expr_stmt(Parser* const parser) {
    Expr* expr = expression(parser);
    ExprStmt expr_stmt = (ExprStmt){
        .inner = expr,
    };
    consume(parser, TOKEN_SEMICOLON, "Expected expression to end with ';'");
    return CREATE_STMT_EXPR(expr_stmt);
}

static Expr* expression(Parser* const parser) {
    return parse_precendence(parser, PREC_ASSIGNMENT);
}

static Expr* binary(Parser* const parser, bool can_assign, Expr* left) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: BINARY Expression\n");
#endif

    Token op = parser->prev;
    switch (op.kind) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_PERCENT:
    case TOKEN_AND:
    case TOKEN_OR:
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL:
    case TOKEN_LOWER:
    case TOKEN_LOWER_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
        break;
    default:
        error_prev(parser, "Expected arithmetic operation");
        return NULL;
    }
    ParseRule* rule = get_rule(op.kind);
    Expr* right = parse_precendence(parser, (Precedence)(rule->precedence + 1));
    BinaryExpr binary = (BinaryExpr){
        .left = left,
        .op = op,
        .right = right,
    };

#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: end BINARY expression\n");
#endif
    return CREATE_BINARY_EXPR(binary);
}

static Expr* call(Parser* const parser, bool can_assign, Expr* left) {
    CallExpr call;
    call.callee = left;
    init_vector(&call.params, sizeof(Expr*));
    parse_call_params(parser, &call.params);
    return CREATE_CALL_EXPR(call);
}

static void parse_call_params(Parser* const parser, Vector* params) {
    parse_expression_list(
        parser,
        params,
        TOKEN_RIGHT_PAREN,
        "Expected ')' to enclose '(' in function call");
}

static void parse_expression_list(Parser* const parser, Vector* params, TokenKind end, const char* error_end_missing) {
    if (parser->current.kind != end) {
        for (;;) {
            Expr* param = expression(parser);
            VECTOR_ADD_EXPR(params, param);
            if (parser->current.kind != TOKEN_COMMA) {
                break;
            }
            advance(parser); // consume ,
        }
    }
    consume(
        parser,
        end,
        error_end_missing);
}

static Expr* grouping(Parser* const parser, bool can_assign) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: GROUP Expression\n");
#endif

    Expr* inner = expression(parser);
    consume(
        parser,
        TOKEN_RIGHT_PAREN,
        "Expected ')' to enclose '(' in group expression");

#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: en [GROUP]\n");
#endif
    return inner;
}

static Expr* primary(Parser* const parser, bool can_assign) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: PRIMARY Expression\n");
#endif

    LiteralExpr literal = (LiteralExpr){
        .literal = parser->prev,
    };
    Expr* expr = CREATE_LITERAL_EXPR(literal);

#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: PRIMARY value ");
    token_print(literal.literal);
    printf("[PARSER DEBUG]: end PRIMARY Expression\n");
#endif
    return expr;
}

static Expr* unary(Parser* const parser, bool can_assign) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: UNARY Expression\n");
#endif

    Token op = parser->prev;
    ParseRule* rule = get_rule(op.kind);
    Expr* inner = parse_precendence(parser, (Precedence)(rule->precedence + 1));
    UnaryExpr unary = (UnaryExpr){
        .op = op,
        .expr = inner,
    };
    Expr* expr = CREATE_UNARY_EXPR(unary);

#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: UNARY operator: ");
    token_print(op);
    printf("[PARSER DEBUG]: end UNARY Expression\n");
#endif
    return expr;
}

static Expr* new_(Parser* const parser, bool can_assign) {
#ifdef PARSER_DEBUG
    printf("[PARSER_DEBUG]: NEW Expression\n");
#endif

    Token klass = parser->current;

    NewExpr new_expr;
    new_expr.klass = klass;
    init_vector(&new_expr.params, sizeof(Expr*));

    Symbol* klass_sym = lookup_str(parser, klass.start, klass.length);
    if (klass_sym == NULL) {
        error(parser, "Undeclared class");
        return CREATE_NEW_EXPR(new_expr);
    }
    if (klass_sym->kind != SYMBOL_CLASS) {
        error(parser, "Cannot use 'new' with something that is not a class");
    }
    advance(parser); // consume klass name
    consume(parser, TOKEN_LEFT_PAREN, "Expected a '(' after class name in new statement");

    parse_call_params(parser, &new_expr.params);

#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: NEW class: ");
    token_print(klass);
    printf("[PARSER DEBUG]: end NEW Expression\n");
#endif
    return CREATE_NEW_EXPR(new_expr);
}

static Expr* prop(Parser* const parser, bool can_assign, Expr* left) {
    Token property = parser->current;
    consume(parser, TOKEN_IDENTIFIER, "Expected ");
    if (parser->current.kind == TOKEN_EQUAL) {
        advance(parser); // consume =
        PropAssigmentExpr assignment;
        assignment.object = left;
        assignment.prop = property;
        assignment.value = parse_precendence(parser, PREC_ASSIGNMENT);
        assignment.object_type = NULL; // we dont know yet
        return CREATE_PROP_ASSIGMENT_EXPR(assignment);
    }
    PropExpr prop;
    prop.object = left;
    prop.prop = property;
    prop.object_type = NULL; // we dont know yet
    return CREATE_PROP_EXPR(prop);
}

static Expr* arr(Parser* const parser, bool can_assign) {
    consume(parser, TOKEN_RIGHT_BRAKET, "Expected ']' after '[' in array expression");

    ArrayExpr array;
    array.left_braket = parser->current;
    init_vector(&array.elements, sizeof(Expr*));
    array.inner = parse_type(parser);
    advance(parser); // Consume type

    consume(parser, TOKEN_LEFT_BRACE, "Expected '{' after type in array expression");

    parse_expression_list(
        parser,
        &array.elements,
        TOKEN_RIGHT_BRACE,
        "Expected array expression to end with '}'");

    return CREATE_ARRAY_EXPR(array);
}

static Expr* cast(Parser* const parser, bool can_assign) {
    CastExpr expr;
    expr.token = parser->prev;
    consume(parser, TOKEN_LOWER, "Expected '<' after keyword 'cast'");
    if (parser->current.kind == TOKEN_GREATER) {
        error(parser, "Expected type after '<' in cast");
    }
    expr.type = parse_type(parser);
    advance(parser); // consume type
    consume(parser, TOKEN_GREATER, "Expected '>' after type in cast");
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after cast<>");
    expr.inner = expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression in cast<>");
    return CREATE_CAST_EXPR(expr);
}

static Expr* identifier(Parser* const parser, bool can_assign) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: IDENTIFIER Expression\n");
#endif

    Token identifier = parser->prev;

    // Why you shouldn't change this line?
    // Well, you may think it's strange that a language that has
    // a complete compiler cant be smart enough to realize that
    // a function that is declared before its use is correct.
    // Well, there is a problem in the compiler's phases that prevents
    // eliminate these lines and checking that in the typechecker:
    // The AST order is relevant for any compiler phase, so it won't
    // realize that the function is declared before, generating bad
    // bytecode (chunk constant index).
    Symbol* existing = get_identifier_symbol(parser, identifier);
    if (!existing) {
        return NULL;
    }

    Expr* expr = NULL;
    if (can_assign && parser->current.kind == TOKEN_EQUAL) {
        // The variable might not be assigned before. We need to ensure
        // that the Symbol table knows that it already does.
        existing->assigned = true;

        advance(parser); //consume =
        Expr* value = parse_precendence(parser, PREC_ASSIGNMENT);
        AssignmentExpr node = (AssignmentExpr){
            .name = identifier,
            .value = value,
        };
        expr = CREATE_ASSIGNMENT_EXPR(node);
    } else {
        if (! existing->assigned) {
            error_prev(parser, "Use of unassigned variable");
        }

        IdentifierExpr node = (IdentifierExpr){
            .name = identifier,
        };
        expr = CREATE_INDENTIFIER_EXPR(node);
    }

#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: NAME value ");
    token_print(identifier);
    printf("[PARSER DEBUG]: end IDENTIFIER Expression\n");
#endif
    return expr;
}

static Expr* self(Parser* const parser, bool can_assign) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: SELF Expression\n");
#endif

    if (! IS_IN_CLASS(parser)) {
        error(parser, "You can only use self inside a class deifinition");
    }
    Token self = parser->prev;
    if (parser->current.kind == TOKEN_EQUAL) {
        error(parser, "Cannot assign to 'self'");
        return NULL;
    }
    Symbol* existing = get_identifier_symbol(parser, self);
    if (!existing) {
        return NULL;
    }
    IdentifierExpr node = (IdentifierExpr){
        .name = self,
    };
    Expr* expr = CREATE_INDENTIFIER_EXPR(node);

#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: end SELF Expression\n");
#endif
    return expr;
}
