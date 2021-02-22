#include "common.h"
#include "parser.h"

Parser* create_parser(Lexer* lexer);
void free_parser(Parser* parser);

static void error(Parser* parser, const char* message);
static void error_next(Parser* parser, const char* message);
static void error_at(Parser* parser, Token* token, const char* message);
static void advance(Parser* parser);
static bool consume(Parser* parser, TokenType expected, const char* msg);

Expr* parse(Parser* parser);

static Expr* expression(Parser* parser);
static Expr* binary(Parser* parser, Expr* left);
static Expr* grouping(Parser* parser);
static Expr* primary(Parser* parser);

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

ParseRule rules[] = {
    [TOKEN_END]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_ERROR]       = {NULL,        NULL,   PREC_NONE},

    [TOKEN_PLUS]        = {NULL,        binary, PREC_TERM},
    [TOKEN_MINUS]       = {NULL,        binary, PREC_TERM},
    [TOKEN_STAR]        = {NULL,        binary, PREC_FACTOR},
    [TOKEN_SLASH]       = {NULL,        binary, PREC_FACTOR},
    [TOKEN_LEFT_PAREN]  = {grouping,    NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL,        NULL,   PREC_NONE},

    [TOKEN_INTEGER]     = {primary,     NULL,   PREC_PRIMARY},
    [TOKEN_FLOAT]       = {primary,     NULL,   PREC_PRIMARY},
};

static ParseRule* get_rule(TokenType type) {
    return &rules[type];
}

static Expr* parse_precendence(Parser* parser, Precedence precedence) {
    advance(parser);
    PrefixParse prefix_parser = get_rule(parser->current.type)->prefix;
    if (prefix_parser == NULL) {
        error(parser, "Expected expression");
        return NULL;
    }
    Expr* left = prefix_parser(parser);
    while (precedence <= get_rule(parser->next.type)->precedence) {
        advance(parser);
        SuffixParse infix_parser = get_rule(parser->current.type)->infix;
        left = infix_parser(parser, left);
    }
    return left;
}

Parser* create_parser(Lexer* lexer) {
    Parser* parser = (Parser*) malloc(sizeof(Parser));
    parser->current.type = -1;
    parser->next. type = -1;
    parser->lexer = lexer;
    parser->has_error = false;
    return parser;
}

void free_parser(Parser* parser) {
    free(parser);
}

static void error(Parser* parser, const char* message) {
    error_at(parser, &parser->current, message);
}

static void error_next(Parser* parser, const char* message) {
    error_at(parser, &parser->next, message);
}

static void error_at(Parser* parser, Token* token, const char* message) {
    fprintf(stderr, "[Line %d] Error", token->line);
    switch(token->type) {
    case TOKEN_ERROR: break;
    case TOKEN_END:
        fprintf(stderr, " at end");
        break;
    default:
        fprintf(stderr, "at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser->has_error = true;
}

static void advance(Parser* parser) {
    if (parser->current.type == TOKEN_END) {
        return;
    }
    parser->current = parser->next;
    parser->next = next_token(parser->lexer);
}

static bool consume(Parser* parser, TokenType expected, const char* message) {
    advance(parser);
    if (parser->current.type != expected) {
        error(parser, message);
        return false;
    }
    return true;
}

Expr* parse(Parser* parser) {
    printf("\tPARSE!!!!!\n");
    advance(parser);
    if (parser->next.type == TOKEN_END || parser->next.type == TOKEN_ERROR) {
        return NULL;
    }
    return expression(parser);
}

static Expr* expression(Parser* parser) {
    return parse_precendence(parser, PREC_ASSIGNMENT);
}

static Expr* binary(Parser* parser, Expr* left) {
    Token op = parser->current;
    switch (op.type) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
        break;
    default:
        error_next(parser, "Expected arithmetic operation");
        return NULL;
    }
    ParseRule* rule = get_rule(op.type);
    Expr* right = parse_precendence(parser, (Precedence)(rule->precedence + 1));
    BinaryExpr binary = (BinaryExpr){
        .left = left,
        .op = op,
        .right = right,
    };
    return CREATE_BINARY_EXPR(binary);
}

static Expr* grouping(Parser* parser) {
    printf("GROUP Expression\n");
    Expr* inner = expression(parser);
    consume(
        parser,
        TOKEN_RIGHT_PAREN,
        "Expected ')' to enclose '(' in group expression");
    printf("en [GROUP]\n");
    return inner;
}

static Expr* primary(Parser* parser) {
    printf("PRIMARY Expression\n");
    LiteralExpr literal = (LiteralExpr){
        .literal = parser->current,
    };
    Expr* expr = CREATE_LITERAL_EXPR(literal);
    print_token(parser->current);
    printf("end PRIMARY Expression\n");
    return expr;
}
