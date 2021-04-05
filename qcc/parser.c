#include "parser.h"
#include <stdarg.h>
#include "common.h"
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

static void advance(Parser* parser);
static bool consume(Parser* parser, TokenKind expected, const char* msg);

static Stmt* main_block(Parser* parser);

static Stmt* declaration(Parser* parser);
static Stmt* variable_decl(Parser* parser);
static void register_symbol(Parser* parser, Token* tkn_symbol, Type type);

static Stmt* statement(Parser* parser);
static Stmt* print_stmt(Parser* parser);
static Stmt* expr_stmt(Parser* parser);

static Expr* expression(Parser* parser);
static Expr* grouping(Parser* parser, bool can_assign);
static Expr* primary(Parser* parser, bool can_assign);
static Expr* identifier(Parser* parser, bool can_assign);
static Expr* unary(Parser* parser, bool can_assign);
static Expr* binary(Parser* parser, bool can_assign, Expr* left);

ParseRule rules[] = {
    [TOKEN_END]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,        NULL,   PREC_NONE},

    [TOKEN_PLUS]          = {unary,       binary, PREC_TERM},
    [TOKEN_MINUS]         = {unary,       binary, PREC_TERM},
    [TOKEN_STAR]          = {NULL,        binary, PREC_FACTOR},
    [TOKEN_SLASH]         = {NULL,        binary, PREC_FACTOR},
    [TOKEN_PERCENT]       = {NULL,        binary, PREC_FACTOR},
    [TOKEN_LEFT_PAREN]    = {grouping,    NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,        NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_BANG]          = {unary,       NULL,   PREC_UNARY},
    [TOKEN_EQUAL]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_LOWER]         = {NULL,        binary, PREC_COMPARISON},
    [TOKEN_GREATER]       = {NULL,        binary, PREC_COMPARISON},

    [TOKEN_AND]           = {NULL,        binary, PREC_AND},
    [TOKEN_OR]            = {NULL,        binary, PREC_OR},
    [TOKEN_EQUAL_EQUAL]   = {NULL,        binary, PREC_EQUALITY},
    [TOKEN_BANG_EQUAL]    = {NULL,        binary, PREC_EQUALITY},
    [TOKEN_LOWER_EQUAL]   = {NULL,        binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,        binary, PREC_COMPARISON},

    [TOKEN_VAR]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_TRUE]          = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_FALSE]         = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_NIL]           = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_STRING]        = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_PRINT]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_IDENTIFIER]    = {identifier,  NULL,   PREC_NONE},
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

void init_parser(Parser* parser, const char* source) {
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
    while(parser->current.kind != TOKEN_SEMICOLON && parser->current.kind != TOKEN_END) {
        advance(parser);
    }
    if (parser->current.kind == TOKEN_SEMICOLON) {
        advance(parser); // consume semicolon
    }
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
    Stmt* ast = main_block(parser);
#ifdef PARSER_DEBUG
    ast_print(ast);
#endif
    return ast;
}

static Stmt* main_block(Parser* parser) {
    ListStmt* list = create_list_stmt();
    while (parser->current.kind != TOKEN_END) {
        Stmt* stmt = declaration(parser);
        list_stmt_add(list, stmt);
        if (parser->panic_mode) {
            syncronize(parser);
        }
    }
    return CREATE_LIST_STMT(list);
}

static Stmt* declaration(Parser* parser) {
    switch (parser->current.kind) {
    case TOKEN_VAR:
        return variable_decl(parser);
    default:
        return statement(parser);
    }
}

static Stmt* statement(Parser* parser) {
    switch (parser->current.kind) {
    case TOKEN_PRINT:
        return print_stmt(parser);
    default:
        return expr_stmt(parser);
    }
}

static Stmt* variable_decl(Parser* parser) {
    advance(parser); // consume var
    VarStmt var;
    var.identifier = parser->current;
    advance(parser); // consume identifier

    Type var_type = UNKNOWN_TYPE;
    if (parser->current.kind == TOKEN_COLON) {
        advance(parser); // consume :
        var_type = type_from_token_kind(parser->current.kind);
        if (var_type == UNKNOWN_TYPE) {
            error(parser, "Unkown type in variable declaration");
        }
        advance(parser); // consume type
    }

    register_symbol(parser, &var.identifier, var_type);

    var.definition = NULL;
    if (parser->current.kind == TOKEN_EQUAL) {
        advance(parser); // consume =
        var.definition = expression(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expected global declaration to end with ';'");
    return CREATE_VAR_STMT(var);
}

static void register_symbol(Parser* parser, Token* tkn_symbol, Type type) {
    Symbol var_symbol = (Symbol){
        .name = create_symbol_name(tkn_symbol->start, tkn_symbol->length),
        .declaration_line = tkn_symbol->line,
        .type = type,
        .constant_index = UINT8_MAX,
    };
    Symbol* exsting = CSYMBOL_LOOKUP(&var_symbol.name);
    if (exsting && exsting->declaration_line < var_symbol.declaration_line) {
        error_prev(parser, "Variable already declared in line %d", exsting->declaration_line);
        return;
    }
    CSYMBOL_INSERT(var_symbol);
}

static Stmt* print_stmt(Parser* parser) {
    advance(parser); // consume print
    Expr* expr = expression(parser);
    PrintStmt print_stmt = (PrintStmt){
        .inner = expr,
    };
    consume(parser, TOKEN_SEMICOLON, "Expected print to end with ';'");
    return CREATE_PRINT_STMT(print_stmt);
}

static Stmt* expr_stmt(Parser* parser) {
    Expr* expr = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected statement to end with ';'");
    ExprStmt expr_stmt = (ExprStmt){
        .inner = expr,
    };
    return CREATE_EXPR_STMT(expr_stmt);
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
    Symbol* existing = CSYMBOL_LOOKUP_STR(identifier.start, identifier.length);
    if (!existing) {
        error_prev(parser, "Use of undeclared variable", identifier.length, identifier.start);
        return NULL;
    }
    if (existing->declaration_line > identifier.line) {
        error_prev(parser, "Use of variable '%.*s' before declaration", identifier.length, identifier.start);
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
