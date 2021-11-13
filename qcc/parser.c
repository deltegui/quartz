#include "parser.h"
#include <stdarg.h>
#include "common.h"
#include "vector.h"
#include "symbol.h"
#include "type.h"
#include "error.h"
#include "import.h"

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
static void error_at(Parser* const parser, Token* token, const char* message, va_list params);
static void syncronize(Parser* const parser);

static void create_scope(Parser* const parser);
static void end_scope(Parser* const parser);
static Symbol* current_scope_lookup(Parser* const parser, SymbolName* name);
static Symbol* lookup_str(Parser* const parser, const char* name, int length);
static void insert(Parser* const parser, Symbol entry);
static bool register_symbol(Parser* const parser, Symbol symbol);
static Symbol create_symbol_calc_global(Parser* const parser, Token* token, Type* type);

static void advance(Parser* const parser);
static bool consume(Parser* const parser, TokenKind expected, const char* msg);

static Symbol* get_identifier_symbol(Parser* const parser, Token identifier);

static Stmt* declaration_block(Parser* const parser, TokenKind limit_token);

static Stmt* declaration(Parser* const parser);
static Stmt* parse_variable(Parser* const parser);
static Stmt* variable_decl(Parser* const parser);
static Stmt* function_decl(Parser* const parser);
static Stmt* typealias_decl(Parser* const parser);
static Stmt* import_decl(Parser* const parser);
static Stmt* native_import(Parser* const parser, NativeImport import);
static Stmt* file_import(Parser* const parser, FileImport import);
static void parse_function_body(Parser* const parser, FunctionStmt* fn, Symbol* fn_sym);
static void parse_function_params_declaration(Parser* const parser, Symbol* symbol);
static void add_params_to_body(Parser* const parser, Symbol* fn_sym);
static Type* parse_type(Parser* const parser);
static Type* parse_function_type(Parser* const parser);

static Stmt* statement(Parser* const parser);
static Stmt* block_stmt(Parser* const parser);
static Stmt* print_stmt(Parser* const parser);
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
static Expr* unary(Parser* const parser, bool can_assign);
static Expr* binary(Parser* const parser, bool can_assign, Expr* left);
static Expr* call(Parser* const parser, bool can_assign, Expr* left);

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
    [TOKEN_DOT]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_BANG]          = {unary,       NULL,   PREC_UNARY},
    [TOKEN_EQUAL]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_LOWER]         = {NULL,        binary, PREC_COMPARISON},
    [TOKEN_GREATER]       = {NULL,        binary, PREC_COMPARISON},
    [TOKEN_SEMICOLON]     = {NULL,        NULL,   PREC_NONE},
    [TOKEN_COLON]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,        NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,        NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,        NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,        NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,        NULL,   PREC_NONE},

    [TOKEN_AND]           = {NULL,        binary, PREC_AND},
    [TOKEN_OR]            = {NULL,        binary, PREC_OR},
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
    [TOKEN_NIL]           = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_STRING]        = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_PRINT]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_IDENTIFIER]    = {identifier,  NULL,   PREC_NONE},

    [TOKEN_TYPE_NUMBER]   = {NULL,        NULL,   PREC_NONE},
    [TOKEN_TYPE_STRING]   = {NULL,        NULL,   PREC_NONE},
    [TOKEN_TYPE_BOOL]     = {NULL,        NULL,   PREC_NONE},
    [TOKEN_TYPE_NIL]      = {NULL,        NULL,   PREC_NONE},
};

#define IN_LOOP(parser, ...)\
    do {\
        bool prev = parser->is_in_loop;\
        parser->is_in_loop = true;\
        __VA_ARGS__\
        parser->is_in_loop = prev;\
    } while(false)

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

void init_parser(Parser* const parser, const char* source, ScopedSymbolTable* symbols) {
    parser->symbols = symbols;
    parser->current.kind = -1;
    parser->prev.kind = -1;
    init_lexer(&parser->lexer, source);
    parser->panic_mode = false;
    parser->has_error = false;
    parser->function_deep_count = 0;
    parser->scope_depth = 0;
    parser->is_in_loop = false;
}

static void error(Parser* const parser, const char* message, ...) {
    va_list params;
    va_start(params, message);
    error_at(parser, &parser->current, message, params);
    va_end(params);
}

static void error_prev(Parser* const parser, const char* message, ...) {
    va_list params;
    va_start(params, message);
    error_at(parser, &parser->prev, message, params);
    va_end(params);
}

static void error_at(Parser* const parser, Token* token, const char* format, va_list params) {
    if (parser->panic_mode) {
        return;
    }
    parser->panic_mode = true;
    fprintf(stderr, "[Line %d] Error", token->line);
    switch(token->kind) {
    case TOKEN_ERROR: break;
    case TOKEN_END:
        fprintf(stderr, " at end");
        break;
    default:
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": ");
    vfprintf(stderr, format, params);
    fprintf(stderr, "\n");
    print_error_context(parser->lexer.source, token);
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
        case TOKEN_PRINT:
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

static void insert(Parser* const parser, Symbol entry){
    scoped_symbol_insert(parser->symbols, entry);
}

static bool register_symbol(Parser* const parser, Symbol symbol) {
    Symbol* exsting = current_scope_lookup(parser, &symbol.name);
    if (exsting) {
        error_prev(parser, "Variable already declared in line %d", exsting->declaration_line);
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
    if (existing->declaration_line > identifier.line) {
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
    Stmt* ast = declaration_block(parser, TOKEN_END);
#ifdef PARSER_DEBUG
    ast_print(ast);
#endif
    return ast;
}

static Stmt* declaration_block(Parser* const parser, TokenKind limit_token) {
    ListStmt* list = create_stmt_list();
    while (parser->current.kind != limit_token && parser->current.kind != TOKEN_END) {
        Stmt* stmt = declaration(parser);
        assert(stmt != NULL);
        if (parser->panic_mode) {
            // That stmt is not inside the abstract syntax tree. You need to free it.
            free_stmt(stmt);
            syncronize(parser);
        } else {
            stmt_list_add(list, stmt);
        }
    }
    return CREATE_STMT_LIST(list);
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
    default:
        return statement(parser);
    }
}

static Stmt* statement(Parser* const parser) {
    switch (parser->current.kind) {
    case TOKEN_LEFT_BRACE:
        return block_stmt(parser);
    case TOKEN_PRINT:
        return print_stmt(parser);
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
    advance(parser); // consume {
    BlockStmt block;
    create_scope(parser);
    block.stmts = declaration_block(parser, TOKEN_RIGHT_BRACE);
    consume(parser, TOKEN_RIGHT_BRACE, "Expected block to end with '}'");
    end_scope(parser);

    return CREATE_STMT_BLOCK(block);
}

static Stmt* parse_variable(Parser* const parser) {
    advance(parser); // consume var
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
    if (! register_symbol(parser, symbol)) {
        free_symbol(&symbol);
    }

    return CREATE_STMT_VAR(var);
}

static Stmt* variable_decl(Parser* const parser) {
    Stmt* var = parse_variable(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected variable declaration to end with ';'");
    return var;
}

static Stmt* typealias_decl(Parser* const parser) {
    advance(parser); // consume typedef

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
    if (! register_symbol(parser, symbol)) {
        free_symbol(&symbol);
        error(parser, "Type alias already defined");
    }

    return CREATE_STMT_TYPEALIAS(stmt);
}

static Stmt* import_decl(Parser* const parser) {
    advance(parser); // consume import
    ImportStmt import_stmt = (ImportStmt) {
        .filename = parser->current,
        .ast = NULL,
    };
    advance(parser); // consume filename
    consume(parser, TOKEN_SEMICOLON, "Expected semicolon at end of import statment");
    Import imp = import(
        import_stmt.filename.start,
        import_stmt.filename.length);
    import_stmt.ast = (imp.is_native) ?
        native_import(parser, imp.native) :
        file_import(parser, imp.file);
    return CREATE_STMT_IMPORT(imp);
}

static Stmt* native_import(Parser* const parser, NativeImport import) {
    ListStmt* list = create_stmt_list();
    for (int i = 0; i < import.functions_length; i++) {
        NativeFunction fn = import.functions[i];
        NativeFunctionStmt stmt = (NativeFunctionStmt) {
            .name = fn.name,
            .length = fn.length,
            .function = fn.function,
        };
        stmt_list_add(list, CREATE_STMT_NATIVE(stmt));
    }
    return CREATE_STMT_LIST(list);
}

static Stmt* file_import(Parser* const parser, FileImport import) {
    if (import.is_already_loaded) {
        return NULL;
    }
    if (import.source == NULL) {
        parser->has_error = true;
        return NULL;
    }
    Parser subparser;
    init_parser(&subparser, import.source, parser->symbols);
    Stmt* subast = parse(&subparser);
    if (subparser.has_error) {
        parser->has_error = true;
    }
    return subast;
}

static Stmt* function_decl(Parser* const parser) {
    advance(parser); // consume fn

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
    if (! register_symbol(parser, symbol)) {
        free_symbol(&symbol);
    }
    Symbol* registered = lookup_str(parser, fn.identifier.start, fn.identifier.length);
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
            error (parser, "Unknown type in function param in function declaration");
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
    Token* param_names = VECTOR_AS_TOKENS(&fn_sym->function.param_names);
    Type** param_types = VECTOR_AS_TYPES(&TYPE_FN_PARAMS(fn_sym->type));
    for (uint32_t i = 0; i < fn_sym->function.param_names.size; i++) {
        Symbol param = create_symbol_from_token(
            &param_names[i],
            param_types[i]);
        if (! register_symbol(parser, param)) {
            free_symbol(&param);
        }
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
    switch (symbol->kind) {
    case SYMBOL_TYPEALIAS:
        return symbol->type;
    default:
        error(
            parser,
            "The identifier '%.*s' is not a type",
            parser->current.length,
            parser->current.start);
        return CREATE_TYPE_UNKNOWN();
    }
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

static Stmt* print_stmt(Parser* const parser) {
    advance(parser); // consume print
    Expr* expr = expression(parser);
    PrintStmt print_stmt = (PrintStmt){
        .inner = expr,
    };
    consume(parser, TOKEN_SEMICOLON, "Expected print statment to end with ';'");
    return CREATE_STMT_PRINT(print_stmt);
}

static Stmt* return_stmt(Parser* const parser) {
    if (parser->function_deep_count == 0) {
        error(parser, "Cannot use return outside a function!");
    }
    advance(parser); // consume return
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
    advance(parser); // consume if
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
    advance(parser); // consume while
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

    advance(parser); // consume for
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

    // TODO this block of code is similar than (parse_for_mod)
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
    if (! EXPR_IS_IDENTIFIER(*left)) {
        error(parser, "You can only call functions");
    }
    IdentifierExpr fn_name = left->identifier;

    CallExpr call = (CallExpr){
        .identifier = fn_name.name,
    };
    init_vector(&call.params, sizeof(Expr*));

    if (parser->current.kind != TOKEN_RIGHT_PAREN) {
        for (;;) {
            Expr* param = expression(parser);
            VECTOR_ADD_EXPR(&call.params, param);
            if (parser->current.kind != TOKEN_COMMA) {
                break;
            }
            advance(parser); // consume ,
        }
    }
    consume(
        parser,
        TOKEN_RIGHT_PAREN,
        "Expected ')' to enclose '(' in function call");

    free_expr(left); // We only need left identifier. We must free it.
    return CREATE_CALL_EXPR(call);
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
    printf("[PARSER DEBUG]: end PRIMARY Expression\n");
#endif
    return expr;
}
