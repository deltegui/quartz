#include "common.h"
#include "parser.h"

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

typedef Expr* (*PrefixParse)(Parser* parser);
typedef Expr* (*SuffixParse)(Parser* parser, Expr* left);

typedef struct {
    PrefixParse prefix;
    SuffixParse infix;
    Precedence precedence;
} ParseRule;

static ParseRule* get_rule(TokenType type);
static Expr* parse_precendence(Parser* parser, Precedence precedence);

static void error(Parser* parser, const char* message);
static void error_prev(Parser* parser, const char* message);
static void error_at(Parser* parser, Token* token, const char* message);
static void syncronize(Parser* parser);

static void advance(Parser* parser);
static bool consume(Parser* parser, TokenType expected, const char* msg);

static Stmt* global(Parser* parser);
static Stmt* statement(Parser* parser);
static Stmt* print(Parser* parser);
static Stmt* stmt_expr(Parser* parser);

static Expr* expression(Parser* parser);
static Expr* grouping(Parser* parser);
static Expr* primary(Parser* parser);
static Expr* unary(Parser* parser);
static Expr* binary(Parser* parser, Expr* left);

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
    [TOKEN_IDENTIFIER]    = {NULL,        NULL,   PREC_NONE},
};

static ParseRule* get_rule(TokenType type) {
    return &rules[type];
}

static Expr* parse_precendence(Parser* parser, Precedence precedence) {
    advance(parser);
    PrefixParse prefix_parser = get_rule(parser->prev.type)->prefix;
    if (prefix_parser == NULL) {
        error_prev(parser, "Expected expression");
        return NULL;
    }
    Expr* left = prefix_parser(parser);
    while (precedence <= get_rule(parser->current.type)->precedence) {
        if (left == NULL) {
            break;
        }
        advance(parser);
        SuffixParse infix_parser = get_rule(parser->prev.type)->infix;
        if (infix_parser == NULL) {
            break;
        }
        left = infix_parser(parser, left);
    }
    return left;
}

void init_parser(Parser* parser, const char* source) {
    parser->current.type = -1;
    parser->prev.type = -1;
    init_lexer(&parser->lexer, source);
    parser->panic_mode = false;
    parser->has_error = false;
}

static void error(Parser* parser, const char* message) {
    error_at(parser, &parser->current, message);
}

static void error_prev(Parser* parser, const char* message) {
    error_at(parser, &parser->prev, message);
}

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) {
        return;
    }
    parser->panic_mode = true;
    fprintf(stderr, "[Line %d] Error", token->line);
    switch(token->type) {
    case TOKEN_ERROR: break;
    case TOKEN_END:
        fprintf(stderr, " at end");
        break;
    default:
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser->has_error = true;
}

static void syncronize(Parser* parser) {
    parser->panic_mode = false;
    while(parser->current.type != TOKEN_SEMICOLON && parser->current.type != TOKEN_END) {
        advance(parser);
    }
    if (parser->current.type == TOKEN_SEMICOLON) {
        advance(parser); // consume semicolon
    }
}

static void advance(Parser* parser) {
    if (parser->current.type == TOKEN_END) {
        return;
    }
    parser->prev = parser->current;
    parser->current = next_token(&parser->lexer);
}

static bool consume(Parser* parser, TokenType expected, const char* message) {
    if (parser->current.type != expected) {
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
    if (parser->current.type == TOKEN_ERROR) {
        parser->has_error = true; // propagate lexer error to the consumer.
        return NULL;
    }
    if (parser->current.type == TOKEN_END) {
        return NULL;
    }
    Stmt* ast = global(parser);
#ifdef PARSER_DEBUG
    ast_print(ast);
#endif
    return ast;
}

static Stmt* global(Parser* parser) {
    ListStmt* list = create_list_stmt();
    while (parser->current.type != TOKEN_END) {
        Stmt* stmt = statement(parser);
        list_stmt_add(list, stmt);
        if (parser->panic_mode) {
            syncronize(parser);
        }
    }
    return CREATE_LIST_STMT(list);
}

static Stmt* statement(Parser* parser) {
    switch (parser->current.type) {
    case TOKEN_PRINT:
        return print(parser);
    default:
        return stmt_expr(parser);
    }
}

static Stmt* print(Parser* parser) {
    advance(parser); // consume print
    Expr* expr = expression(parser);
    PrintStmt print_stmt = (PrintStmt){
        .inner = expr,
    };
    consume(parser, TOKEN_SEMICOLON, "Expected print to end with ';'");
    return CREATE_PRINT_STMT(print_stmt);
}

static Stmt* stmt_expr(Parser* parser) {
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

static Expr* binary(Parser* parser, Expr* left) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: BINARY Expression\n");
#endif

    Token op = parser->prev;
    switch (op.type) {
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
    ParseRule* rule = get_rule(op.type);
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

static Expr* grouping(Parser* parser) {
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

static Expr* primary(Parser* parser) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: PRIMARY Expression\n");
#endif

    LiteralExpr literal = (LiteralExpr){
        .literal = parser->prev,
    };
    Expr* expr = CREATE_LITERAL_EXPR(literal);

#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: PRIMARY value ");
    token_print(parser->current);
    printf("[PARSER DEBUG]: end PRIMARY Expression\n");
#endif
    return expr;
}

static Expr* unary(Parser* parser) {
#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: UNARY Expression\n");
#endif

    Token op = parser->prev;
    ParseRule* rule = get_rule(op.type);
    Expr* inner = parse_precendence(parser, (Precedence)(rule->precedence + 1));
    UnaryExpr unary = (UnaryExpr){
        .op = op,
        .expr = inner,
    };
    Expr* expr = CREATE_UNARY_EXPR(unary);

#ifdef PARSER_DEBUG
    printf("[PARSER DEBUG]: UNARY operator: ");
    token_print(parser->current);
    printf("[PARSER DEBUG]: end UNARY Expression\n");
#endif
    return expr;
}
