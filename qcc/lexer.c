#include "lexer.h"
#include <string.h>

#ifdef LEXER_DEBUG
#include "debug.h"
#endif

static bool is_at_end(Lexer* const lexer);
static char peek(Lexer* const lexer);
static bool match(Lexer* const lexer, char c);
static bool match_next(Lexer* const lexer, char next);
static char advance(Lexer* const lexer);
static bool consume(Lexer* const lexer, char expected);

static Token create_token(Lexer* const lexer, TokenKind kind);
static Token create_error(Lexer* const lexer, const char* message);

static bool skip_whitespaces(Lexer* const lexer);
static bool consume_multiline_comment(Lexer* const lexer);
static bool is_numeric(Lexer* const lexer);
static bool is_alpha(Lexer* const lexer);
static bool is_string_quote(Lexer* const lexer);
static Token scan_number(Lexer* const lexer);
static Token scan_string(Lexer* const lexer);
static bool match_subtoken(Lexer* const lexer, const char* subpart, int start, int len);
static bool match_token(Lexer* const lexer, const char* subpart, int start, int len);
static Token scan_identifier(Lexer* const lexer);
static inline Token scan_token(Lexer* const lexer);
Token next_token(Lexer* const lexer);

#define CONSUME_UNTIL(lexer, character)\
    while(peek(lexer) != character && !is_at_end(lexer))\
        advance(lexer)

#define NEW_LINE(lexer)\
    lexer->line++;\
    lexer->column = 0

void init_lexer(Lexer* const lexer, const char* const buffer) {
    lexer->source = buffer;
    lexer->current = buffer;
    lexer->start = buffer;
    lexer->line = 1;
    lexer->column = 0;
}

static bool is_at_end(Lexer* const lexer) {
    return *lexer->current == '\0';
}

static char peek(Lexer* const lexer) {
    return *lexer->current;
}

static bool match(Lexer* const lexer, char c) {
    if (is_at_end(lexer)) {
        return false;
    }
    return (*lexer->current) == c;
}

static bool match_next(Lexer* const lexer, char next) {
    if (is_at_end(lexer)) {
        return false;
    }
    return *(lexer->current + 1) == next;
}

static char advance(Lexer* const lexer) {
    if (is_at_end(lexer)) {
        return '\0';
    }
    lexer->column++;
    return *lexer->current++;
}

static bool consume(Lexer* const lexer, char expected) {
    if (match(lexer, expected)) {
        advance(lexer);
        return true;
    }
    return false;
}

static Token create_token(Lexer* const lexer, TokenKind kind) {
    Token token;
    token.length = (int) (lexer->current - lexer->start);
    token.line = lexer->line;
    token.column = lexer->column - 1;
    token.start = lexer->start;
    token.kind = kind;
#ifdef LEXER_DEBUG
    printf("[LEXER DEBUG]: Read ");
    token_print(token);
#endif
    return token;
}

static Token create_error(Lexer* const lexer, const char* message) {
    int len = (int) (lexer->current - lexer->start);
    fprintf(stderr, "[Line %d] %s here '%.*s'\n", lexer->line, message, len, lexer->start);
    return create_token(lexer, TOKEN_ERROR);
}

static bool skip_whitespaces(Lexer* const lexer) {
    bool ok = true;
    for (;;) {
        switch (*lexer->current) {
        case '\n':
            NEW_LINE(lexer);
        case ' ':
        case '\t':
        case '\r':
            advance(lexer);
            break;
        case '/':
            if (match_next(lexer, '/')) {
                CONSUME_UNTIL(lexer, '\n');
            } else if (match_next(lexer, '*')) {
                ok = ok && consume_multiline_comment(lexer);
            } else {
                return ok;
            }
            break;
        default:
            return ok;
        }
    }
}

static bool consume_multiline_comment(Lexer* const lexer) {
    int comment_start_line = lexer->line;
    while (! (match(lexer, '*') && match_next(lexer, '/')) ) {
        if (is_at_end(lexer)) {
            fprintf(
                stderr,
                "[Line %d] Expected comment that starts in line %d to end with '*/' at end of file.\n",
                lexer->line,
                comment_start_line);
            return false;
        }
        if (match(lexer, '\n')) {
            NEW_LINE(lexer);
        }
        advance(lexer);
    }
    consume(lexer, '*');
    consume(lexer, '/');
    return true;
}

static bool is_numeric(Lexer* const lexer) {
    return *lexer->current >= '0' && *lexer->current <= '9';
}

static bool is_alpha(Lexer* const lexer) {
    char c = *lexer->current;
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

static bool is_string_quote(Lexer* const lexer) {
    char c = *lexer->current;
    return (c == '\'' || c == '\"');
}

static Token scan_number(Lexer* const lexer) {
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

static Token scan_string(Lexer* const lexer) {
    advance(lexer); // Consume first quote
    lexer->start = lexer->current; // Omit first quote
    while (!is_string_quote(lexer) && !is_at_end(lexer)) {
        if (match(lexer, '\n')) {
            NEW_LINE(lexer);
        }
        advance(lexer);
    }
    if (!is_string_quote(lexer)) {
        return create_error(lexer, "Malformed string: expected string to end with '\"'");
    }
    Token str_token = create_token(lexer, TOKEN_STRING);
    advance(lexer); //Consume last quote
    return str_token;
}

static bool match_subtoken(Lexer* const lexer, const char* subpart, int start, int len) {
#define START_OVERFLOWS lexer->start + start > lexer->current
#define END_OVERFLOWS lexer->start + len > lexer->current
    if (START_OVERFLOWS || END_OVERFLOWS) {
        return false;
    }
    return memcmp(lexer->start + start, subpart, len - start) == 0;
#undef START_OVERFLOWS
#undef END_OVERFLOWS
}

static bool match_token(Lexer* const lexer, const char* subpart, int start, int len) {
    int readed_length = lexer->current - lexer->start;
    if (readed_length != len) {
        return false;
    }
    return match_subtoken(lexer, subpart, start, len);
}

static Token scan_identifier(Lexer* const lexer) {
    while (is_numeric(lexer) || is_alpha(lexer)) {
        advance(lexer);
    }
    switch (*lexer->start) {
    case 'b': {
        if (match_token(lexer, "reak", 1, 5)) {
            return create_token(lexer, TOKEN_BREAK);
        }
        break;
    }
    case 'c': {
        if (match_token(lexer, "ontinue", 1, 8)) {
            return create_token(lexer, TOKEN_CONTINUE);
        }
        if (match_token(lexer, "lass", 1, 5)) {
            return create_token(lexer, TOKEN_CLASS);
        }
        break;
    }
    case 'e': {
        if (match_token(lexer, "lse", 1, 4)) {
            return create_token(lexer, TOKEN_ELSE);
        }
        break;
    }
    case 'f': {
        if (match_token(lexer, "alse", 1, 5)) {
            return create_token(lexer, TOKEN_FALSE);
        }
        if (match_token(lexer, "n", 1, 2)) {
            return create_token(lexer, TOKEN_FUNCTION);
        }
        if (match_token(lexer, "or", 1, 3)) {
            return create_token(lexer, TOKEN_FOR);
        }
        break;
    }
    case 'i': {
        if (match_token(lexer, "f", 1, 2)) {
            return create_token(lexer, TOKEN_IF);
        }
        if (match_token(lexer, "mport", 1, 6)) {
            return create_token(lexer, TOKEN_IMPORT);
        }
        break;
    }
    case 'n': {
        if (match_token(lexer, "il", 1, 3)) {
            return create_token(lexer, TOKEN_NIL);
        }
        if (match_token(lexer, "ew", 1, 3)) {
            return create_token(lexer, TOKEN_NEW);
        }
        break;
    }
    case 'p': {
        if (match_token(lexer, "ub", 1, 3)) {
            return create_token(lexer, TOKEN_PUBLIC);
        }
        break;
    }
    case 'r': {
        if (match_token(lexer, "eturn", 1, 6)) {
            return create_token(lexer, TOKEN_RETURN);
        }
        break;
    }
    case 't': {
        if (match_token(lexer, "rue", 1, 4)) {
            return create_token(lexer, TOKEN_TRUE);
        }
        if (match_token(lexer, "ypedef", 1, 7)) {
            return create_token(lexer, TOKEN_TYPEDEF);
        }
        break;
    }
    case 'v': {
        if (match_token(lexer, "ar", 1, 3)) {
            return create_token(lexer, TOKEN_VAR);
        }
        break;
    }
    case 'w': {
        if (match_token(lexer, "hile", 1, 5)) {
            return create_token(lexer, TOKEN_WHILE);
        }
        break;
    }
    case 'N': {
        if (match_token(lexer, "umber", 1, 6)) {
            return create_token(lexer, TOKEN_TYPE_NUMBER);
        }
        if (match_token(lexer, "il", 1, 3)) {
            return create_token(lexer, TOKEN_TYPE_NIL);
        }
        break;
    }
    case 'S': {
        if (match_token(lexer, "tring", 1, 6)) {
            return create_token(lexer, TOKEN_TYPE_STRING);
        }
        break;
    }
    case 'B': {
        if (match_token(lexer, "ool", 1, 4)) {
            return create_token(lexer, TOKEN_TYPE_BOOL);
        }
        break;
    }
    case 'V': {
        if (match_token(lexer, "oid", 1, 4)) {
            return create_token(lexer, TOKEN_TYPE_VOID);
        }
        break;
    }
    }
    return create_token(lexer, TOKEN_IDENTIFIER);
}

static inline Token scan_token(Lexer* const lexer) {
    switch (advance(lexer)) {
    case '+': return create_token(lexer, TOKEN_PLUS);
    case '-': return create_token(lexer, TOKEN_MINUS);
    case '*': return create_token(lexer, TOKEN_STAR);
    case '/': return create_token(lexer, TOKEN_SLASH);
    case '%': return create_token(lexer, TOKEN_PERCENT);
    case '(': return create_token(lexer, TOKEN_LEFT_PAREN);
    case ')': return create_token(lexer, TOKEN_RIGHT_PAREN);
    case '{': return create_token(lexer, TOKEN_LEFT_BRACE);
    case '}': return create_token(lexer, TOKEN_RIGHT_BRACE);
    case '.': return create_token(lexer, TOKEN_DOT);
    case ';': return create_token(lexer, TOKEN_SEMICOLON);
    case ':': return create_token(lexer, TOKEN_COLON);
    case ',': return create_token(lexer, TOKEN_COMMA);
    case '<': {
        if (consume(lexer, '=')) {
            return create_token(lexer, TOKEN_LOWER_EQUAL);
        }
        return create_token(lexer, TOKEN_LOWER);
    }
    case '>': {
        if (consume(lexer, '=')) {
            return create_token(lexer, TOKEN_GREATER_EQUAL);
        }
        return create_token(lexer, TOKEN_GREATER);
    }
    case '&': {
        if (consume(lexer, '&')) {
            return create_token(lexer, TOKEN_AND);
        }
        return create_error(lexer, "Unkown '&' character");
    }
    case '|': {
        if (consume(lexer, '|')) {
            return create_token(lexer, TOKEN_OR);
        }
        return create_error(lexer, "Unkown '|' character");
    }
    case '=': {
        if (consume(lexer, '=')) {
            return create_token(lexer, TOKEN_EQUAL_EQUAL);
        }
        return create_token(lexer, TOKEN_EQUAL);
    }
    case '!': {
        if (consume(lexer, '=')) {
            return create_token(lexer, TOKEN_BANG_EQUAL);
        }
        return create_token(lexer, TOKEN_BANG);
    }
    default: return scan_identifier(lexer);
    }
}

Token next_token(Lexer* const lexer) {
    if (!skip_whitespaces(lexer)) {
        return create_token(lexer, TOKEN_ERROR);
    }
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
    return scan_token(lexer);
}
