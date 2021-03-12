#include "string.h"
#include "lexer.h"

#ifdef LEXER_DEBUG
#include "debug.h"
#endif

static bool is_at_end(Lexer* lexer);
static char peek(Lexer* lexer);
static bool match(Lexer* lexer, char c);
static bool match_next(Lexer* lexer, char next);
static void advance(Lexer* lexer);
static bool consume(Lexer* lexer, char expected);

static Token create_token(Lexer* lexer, TokenType type);
static Token create_error(Lexer* lexer, const char* message);

static void skip_whitespaces(Lexer* lexer);
static bool is_numeric(Lexer* lexer);
static bool is_alpha(Lexer* lexer);
static bool is_string_quote(Lexer* lexer);
static Token scan_number(Lexer* lexer);
static Token scan_string(Lexer* lexer);
static bool match_subtoken(Lexer* lexer, const char* subpart, int start, int len);
static Token scan_identifier(Lexer* lexer);
Token next_token(Lexer* lexer);

#define CONSUME_UNTIL(lexer, character)\
    while(peek(lexer) != character && !is_at_end(lexer))\
        advance(lexer)

void init_lexer(Lexer* lexer, const char* buffer) {
    lexer->current = buffer;
    lexer->start = buffer;
    lexer->line = 1;
}

static bool is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char peek(Lexer* lexer) {
    return *lexer->current;
}

static bool match(Lexer* lexer, char c) {
    if (is_at_end(lexer)) {
        return false;
    }
    return (*lexer->current) == c;
}

static bool match_next(Lexer* lexer, char next) {
    if (is_at_end(lexer)) {
        return false;
    }
    return *(lexer->current + 1) == next;
}

static void advance(Lexer* lexer) {
    if (is_at_end(lexer)) {
        return;
    }
    lexer->current++;
}

static bool consume(Lexer* lexer, char expected) {
    if (match(lexer, expected)) {
        advance(lexer);
        return true;
    }
    return false;
}

static Token create_token(Lexer* lexer, TokenType type) {
    Token token;
    token.length = (int) (lexer->current - lexer->start);
    token.line = lexer->line;
    token.start = lexer->start;
    token.type = type;
#ifdef LEXER_DEBUG
    printf("[LEXER DEBUG]: Readed ");
    token_print(token);
#endif
    return token;
}

static Token create_error(Lexer* lexer, const char* message) {
    int len = (int) (lexer->current - lexer->start);
    fprintf(stderr, "[Line %d] %s here '%.*s'\n", lexer->line, message, len, lexer->start);
    return create_token(lexer, TOKEN_ERROR);
}

static void skip_whitespaces(Lexer* lexer) {
    for (;;) {
        switch (*lexer->current) {
        case '\n':
            lexer->line++;
        case ' ':
        case '\t':
        case '\r':
            advance(lexer);
            break;
        case '/':
            if (match_next(lexer, '/')) {
                CONSUME_UNTIL(lexer, '\n');
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

static bool is_numeric(Lexer* lexer) {
#define ASCII_ZERO 48
#define ASCII_NINE 57
    return *lexer->current >= ASCII_ZERO && *lexer->current <= ASCII_NINE;
#undef ASCII_ZERO
#undef ASCII_NINE
}

static bool is_alpha(Lexer* lexer) {
    char c = *lexer->current;
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

static bool is_string_quote(Lexer* lexer) {
    char c = *lexer->current;
    return (c == '\'' || c == '\"');
}

static Token scan_number(Lexer* lexer) {
    while (is_numeric(lexer)) {
        advance(lexer);
    }
    if (!match(lexer, '.')) {
        return create_token(lexer, TOKEN_NUMBER);
    }
    advance(lexer); // consume dot
    if (!is_numeric(lexer)) {
        return create_error(lexer, "Malformed float: Expected to have numbers after dot");
    }
    while (is_numeric(lexer)) {
        advance(lexer);
    }
    return create_token(lexer, TOKEN_NUMBER);
}

static Token scan_string(Lexer* lexer) {
    advance(lexer); // Consume first quote
    lexer->start = lexer->current; // Omit first quote
    while (!is_string_quote(lexer) && !is_at_end(lexer)) {
        advance(lexer);
    }
    if (!is_string_quote(lexer)) {
        return create_error(lexer, "Malformed string: expected string to end with '\"'");
    }
    Token str_token = create_token(lexer, TOKEN_STRING);
    advance(lexer); //Consume last quote
    return str_token;
}

static bool match_subtoken(Lexer* lexer, const char* subpart, int start, int len) {
#define START_OVERFLOWS lexer->start + start > lexer->current
#define END_OVERFLOWS lexer->start + len > lexer->current
    if (START_OVERFLOWS || END_OVERFLOWS) {
        return false;
    }
    return memcmp(lexer->start + start, subpart, len - start) == 0;
#undef START_OVERFLOWS
#undef END_OVERFLOWS
}

static Token scan_identifier(Lexer* lexer) {
    while (is_numeric(lexer) || is_alpha(lexer))
        advance(lexer);
    switch (*lexer->start) {
    case 't': {
        if (match_subtoken(lexer, "rue", 1, 4)) {
            return create_token(lexer, TOKEN_TRUE);
        }
    }
    case 'f': {
        if (match_subtoken(lexer, "alse", 1, 5)) {
            return create_token(lexer, TOKEN_FALSE);
        }
    }
    case 'n': {
        if (match_subtoken(lexer, "il", 1, 3)) {
            return create_token(lexer, TOKEN_NIL);
        }
    }
    }
    return create_error(lexer, "Unkown identifier");
}

Token next_token(Lexer* lexer) {
    skip_whitespaces(lexer);
    if (is_at_end(lexer)) {
        return create_token(lexer, TOKEN_END);
    }
    lexer->start = lexer->current;
    if (is_numeric(lexer)) {
        return scan_number(lexer);
    }
    if (is_string_quote(lexer)) {
        return scan_string(lexer);
    }
    switch (*lexer->current++) {
    case '+': return create_token(lexer, TOKEN_PLUS);
    case '-': return create_token(lexer, TOKEN_MINUS);
    case '*': return create_token(lexer, TOKEN_STAR);
    case '/': return create_token(lexer, TOKEN_SLASH);
    case '%': return create_token(lexer, TOKEN_PERCENT);
    case '(': return create_token(lexer, TOKEN_LEFT_PAREN);
    case ')': return create_token(lexer, TOKEN_RIGHT_PAREN);
    case '.': return create_token(lexer, TOKEN_DOT);
    case '!': return create_token(lexer, TOKEN_BANG);
    case '&': {
        if (consume(lexer, '&')) {
            return create_token(lexer, TOKEN_AND);
        }
    }
    case '|': {
        if (consume(lexer, '|')) {
            return create_token(lexer, TOKEN_OR);
        }
    }
    default: return scan_identifier(lexer);
    }
}
