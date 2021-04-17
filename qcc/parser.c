#include "parser.h"
#include <stdarg.h>
#include "common.h"
#include "fnparams.h"
#include "symbol.h"
#include "type.h"

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

typedef Expr* (*PrefixParse)(Parser* parser, bool can_assign);
typedef Expr* (*SuffixParse)(Parser* parser, bool can_assign, Expr* left);

typedef struct {
    PrefixParse prefix;
    SuffixParse infix;
    Precedence precedence;
} ParseRule;

static ParseRule* get_rule(TokenKind kind);
static Expr* parse_precendence(Parser* parser, Precedence precedence);

static void error(Parser* parser, const char* message, ...);
static void error_prev(Parser* parser, const char* message, ...);
static void error_at(Parser* parser, Token* token, const char* message, va_list params);
static void syncronize(Parser* parser);

static void create_scope(Parser* parser);
static void open_prev_scope(Parser* parser);
static void end_scope(Parser* parser);
static Symbol* current_scope_lookup(Parser* parser, SymbolName* name);
static Symbol* lookup_str(Parser* parser, const char* name, int length);
static void insert(Parser* parser, Symbol entry);

static void advance(Parser* parser);
static bool consume(Parser* parser, TokenKind expected, const char* msg);

static Symbol* get_identifier_symbol(Parser* parser, Token identifier);

static Stmt* declaration_block(Parser* parser);

static Stmt* declaration(Parser* parser);
static Stmt* variable_decl(Parser* parser);
static Stmt* function_decl(Parser* parser);
static void parse_function_params_declaration(Parser* parser, FunctionSymbol* fn_sym);
static void add_params_to_body(Parser* parser, FunctionSymbol* fn_sym);
static void register_symbol(Parser* parser, Symbol symbol);

static Stmt* statement(Parser* parser);
static Stmt* block_stmt(Parser* parser);
static Stmt* print_stmt(Parser* parser);
static Stmt* return_stmt(Parser* parser);
static Stmt* expr_stmt(Parser* parser);

static Expr* expression(Parser* parser);
static Expr* grouping(Parser* parser, bool can_assign);
static Expr* primary(Parser* parser, bool can_assign);
static Expr* identifier(Parser* parser, bool can_assign);
static Expr* unary(Parser* parser, bool can_assign);
static Expr* binary(Parser* parser, bool can_assign, Expr* left);
static Expr* call(Parser* parser, bool can_assign, Expr* left);

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

static ParseRule* get_rule(TokenKind kind) {
    return &rules[kind];
}

static Expr* parse_precendence(Parser* parser, Precedence precedence) {
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

void init_parser(Parser* parser, const char* source, ScopedSymbolTable* symbols) {
    parser->symbols = symbols;
    parser->current.kind = -1;
    parser->prev.kind = -1;
    init_lexer(&parser->lexer, source);
    parser->panic_mode = false;
    parser->has_error = false;
}

static void error(Parser* parser, const char* message, ...) {
    va_list params;
    va_start(params, message);
    error_at(parser, &parser->current, message, params);
    va_end(params);
}

static void error_prev(Parser* parser, const char* message, ...) {
    va_list params;
    va_start(params, message);
    error_at(parser, &parser->prev, message, params);
    va_end(params);
}

static void error_at(Parser* parser, Token* token, const char* format, va_list params) {
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
    parser->has_error = true;
}

static void syncronize(Parser* parser) {
    parser->panic_mode = false;
    for (;;) {
        switch (parser->current.kind) {
        case TOKEN_SEMICOLON:
            advance(parser); // consume semicolon
        case TOKEN_VAR:
        case TOKEN_PRINT:
        case TOKEN_END:
            return;
        default:
            advance(parser);
        }
    }
}

static void create_scope(Parser* parser){
    symbol_create_scope(parser->symbols);
}

static void open_prev_scope(Parser* parser) {
    symbol_open_prev_scope(parser->symbols);
}

static void end_scope(Parser* parser){
    symbol_end_scope(parser->symbols);
}

static Symbol* current_scope_lookup(Parser* parser, SymbolName* name){
    return symbol_lookup(&parser->symbols->current->symbols, name);
}

static Symbol* lookup_str(Parser* parser, const char* name, int length){
    return scoped_symbol_lookup_str(parser->symbols, name, length);
}

static void insert(Parser* parser, Symbol entry){
    scoped_symbol_insert(parser->symbols, entry);
}

static void advance(Parser* parser) {
    if (parser->current.kind == TOKEN_END) {
        return;
    }
    parser->prev = parser->current;
    parser->current = next_token(&parser->lexer);
}

static bool consume(Parser* parser, TokenKind expected, const char* message) {
    if (parser->current.kind != expected) {
        error(parser, message);
        return false;
    }
    advance(parser);
    return true;
}

static Symbol* get_identifier_symbol(Parser* parser, Token identifier) {
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

Stmt* parse(Parser* parser) {
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
    Stmt* ast = declaration_block(parser);
#ifdef PARSER_DEBUG
    ast_print(ast);
#endif
    return ast;
}

static Stmt* declaration_block(Parser* parser) {
    ListStmt* list = create_stmt_list();
    while (parser->current.kind != TOKEN_RIGHT_BRACE && parser->current.kind != TOKEN_END) {
        Stmt* stmt = declaration(parser);
        stmt_list_add(list, stmt);
        if (parser->panic_mode) {
            syncronize(parser);
        }
    }
    return CREATE_STMT_LIST(list);
}

static Stmt* declaration(Parser* parser) {
    switch (parser->current.kind) {
    case TOKEN_VAR:
        return variable_decl(parser);
    case TOKEN_FUNCTION:
        return function_decl(parser);
    default:
        return statement(parser);
    }
}

static Stmt* statement(Parser* parser) {
    switch (parser->current.kind) {
    case TOKEN_LEFT_BRACE:
        return block_stmt(parser);
    case TOKEN_PRINT:
        return print_stmt(parser);
    case TOKEN_RETURN:
        return return_stmt(parser);
    default:
        return expr_stmt(parser);
    }
}

static Stmt* block_stmt(Parser* parser) {
    advance(parser); // consume {
    BlockStmt block;
    create_scope(parser);
    block.stmts = declaration_block(parser);
    consume(parser, TOKEN_RIGHT_BRACE, "Expected block to end with '}'");
    end_scope(parser);
    return CREATE_STMT_BLOCK(block);
}

static Stmt* variable_decl(Parser* parser) {
    advance(parser); // consume var
    VarStmt var;
    var.identifier = parser->current;
    advance(parser); // consume identifier

    Type var_type = TYPE_UNKNOWN;
    if (parser->current.kind == TOKEN_COLON) {
        advance(parser); // consume :
        var_type = type_from_token_kind(parser->current.kind);
        if (var_type == TYPE_UNKNOWN) {
            error(parser, "Unkown type in variable declaration");
        }
        advance(parser); // consume type
    }

    register_symbol(parser, create_symbol_from_token(&var.identifier, var_type));

    var.definition = NULL;
    if (parser->current.kind == TOKEN_EQUAL) {
        advance(parser); // consume =
        var.definition = expression(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expected global declaration to end with ';'");
    return CREATE_STMT_VAR(var);
}

static Stmt* function_decl(Parser* parser) {
    advance(parser); // consume fn

    FunctionStmt fn = (FunctionStmt){
        .identifier = parser->current,
    };
    Symbol symbol = create_symbol_from_token(&fn.identifier, TYPE_FUNCTION);

    advance(parser); // consume identifier
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after function name in function declaration");
    if (parser->current.kind != TOKEN_RIGHT_PAREN) {
        parse_function_params_declaration(parser, &symbol.function);
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after function params in declaration");

    if (parser->current.kind == TOKEN_COLON) {
        advance(parser); // consume colon
        Type return_type = type_from_token_kind(parser->current.kind);
        if (return_type == TYPE_UNKNOWN) {
            error(
                parser,
                "Unknown return type in function '%.*s'",
                fn.identifier.length,
                fn.identifier.start);
        }
        symbol.function.return_type = return_type;
        advance(parser); // consume type
    }

    fn.body = block_stmt(parser);
    add_params_to_body(parser, &symbol.function);

    register_symbol(parser, symbol);
    return CREATE_STMT_FUNCTION(fn);
}

static void parse_function_params_declaration(Parser* parser, FunctionSymbol* fn_sym) {
    for (;;) {
        if (parser->current.kind != TOKEN_IDENTIFIER) {
            error(parser, "Expected to have an identifier in parameter in function declaration");
            break;
        }
        PARAM_ARRAY_ADD_TOKEN(&fn_sym->params, parser->current);
        advance(parser); // cosume param identifier
        if (parser->current.kind != TOKEN_COLON) {
            error(parser, "Expected function parameter to have a type in function declaration");
            break;
        }
        advance(parser); // cosume colon
        Type type = type_from_token_kind(parser->current.kind);
        if (type == TYPE_UNKNOWN) {
            error (parser, "Unknown type in function param in function declaration");
            break;
        }
        PARAM_ARRAY_ADD_TYPE(&fn_sym->param_types, type);
        advance(parser); // consume type
        if (parser->current.kind != TOKEN_COMMA) {
            break;
        }
        advance(parser); // consume comma
    }
}

static void add_params_to_body(Parser* parser, FunctionSymbol* fn_sym) {
    open_prev_scope(parser);
    for (int i = 0; i < fn_sym->params.size; i++) {
        Symbol param = create_symbol_from_token(
            &fn_sym->params.params[i].identifier,
            fn_sym->param_types.params[i].type);
        register_symbol(parser, param);
    }
    end_scope(parser);
}

static void register_symbol(Parser* parser, Symbol symbol) {
    Symbol* exsting = current_scope_lookup(parser, &symbol.name);
    if (exsting) {
        error_prev(parser, "Variable already declared in line %d", exsting->declaration_line);
        return;
    }
    insert(parser, symbol);
}

static Stmt* print_stmt(Parser* parser) {
    advance(parser); // consume print
    Expr* expr = expression(parser);
    PrintStmt print_stmt = (PrintStmt){
        .inner = expr,
    };
    consume(parser, TOKEN_SEMICOLON, "Expected print statment to end with ';'");
    return CREATE_STMT_PRINT(print_stmt);
}

static Stmt* return_stmt(Parser* parser) {
    advance(parser); // consume return
    Expr* expr = expression(parser);
    ReturnStmt return_stmt = (ReturnStmt){
        .inner = expr,
    };
    consume(parser, TOKEN_SEMICOLON, "Expected return statment to end with ';'");
    return CREATE_STMT_RETURN(return_stmt);
}

static Stmt* expr_stmt(Parser* parser) {
    Expr* expr = expression(parser);
    ExprStmt expr_stmt = (ExprStmt){
        .inner = expr,
    };
    consume(parser, TOKEN_SEMICOLON, "Expected expression to end with ';'");
    return CREATE_STMT_EXPR(expr_stmt);
}

static Expr* expression(Parser* parser) {
    return parse_precendence(parser, PREC_ASSIGNMENT);
}

static Expr* binary(Parser* parser, bool can_assign, Expr* left) {
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

static Expr* call(Parser* parser, bool can_assign, Expr* left) {
    if (! EXPR_IS_IDENTIFIER(*left)) {
        error(parser, "You can only call functions");
    }
    IdentifierExpr fn_name = left->identifier;

    CallExpr call = (CallExpr){
        .identifier = fn_name.name,
    };
    init_param_array(&call.params);

    Symbol* fn_sym = lookup_str(parser, fn_name.name.start, fn_name.name.length);
    assert(fn_sym != NULL);

    if (parser->current.kind != TOKEN_RIGHT_PAREN) {
        for (;;) {
            Expr* param = expression(parser);
            PARAM_ARRAY_ADD_EXPR(&call.params, param);
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
    
    if (fn_sym->function.params.size != call.params.size) {
        error(
            parser,
            "Function '%.*s' expects %d params, but was called with %d params",
            call.identifier.length,
            call.identifier.start,
            fn_sym->function.params.size,
            call.params.size);
    }

    free_expr(left); // We only need left identifier. We must free it.
    return CREATE_CALL_EXPR(call);
}

static Expr* grouping(Parser* parser, bool can_assign) {
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

static Expr* primary(Parser* parser, bool can_assign) {
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

static Expr* unary(Parser* parser, bool can_assign) {
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

static Expr* identifier(Parser* parser, bool can_assign) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: IDENTIFIER Expression\n");
#endif

    Token identifier = parser->prev;
    Symbol* existing = get_identifier_symbol(parser, identifier);
    if (!existing) {
        return NULL;
    }

    Expr* expr = NULL;
    if (can_assign && parser->current.kind == TOKEN_EQUAL) {
        advance(parser); //consume =
        Expr* value = parse_precendence(parser, PREC_ASSIGNMENT);
        AssignmentExpr node = (AssignmentExpr){
            .name = identifier,
            .value = value,
        };
        expr = CREATE_ASSIGNMENT_EXPR(node);
    } else {
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
