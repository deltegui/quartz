#include "lexer.h"

Lexer* create_lexer(const char* buffer) {
    Lexer* lexer = (Lexer*) malloc(sizeof(Lexer));
    lexer->current = buffer;
    lexer->start = buffer;
    lexer->line = 1;
    return lexer;
}

void free_lexer(Lexer* lexer) {
    free(lexer);
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

static Token create_token(Lexer* lexer, TokenType type) {
    Token token;
    token.length = (int) (lexer->current - lexer->start);
    token.line = lexer->line;
    token.start = lexer->start;
    token.type = type;
#ifdef LEXER_DEBUG
    printf("[LEXER DEBUG]: Readed ");
    print_token(token);
#endif
    return token;
}

static Token create_error(Lexer* lexer, const char* message) {
    int len = (int) (lexer->current - lexer->start);
    fprintf(stderr, "[Line %d] %s here '%.*s'\n", lexer->line, message, len, lexer->start);
    return create_token(lexer, TOKEN_ERROR);
}

static void skip_whitespaces(Lexer* lexer) {
#define CONSUME_UNTIL(character)\
while(peek(lexer) != character && !is_at_end(lexer))\
advance(lexer)

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
                CONSUME_UNTIL('\n');
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }

#undef CONSUME_UNITL
}

static bool is_numeric(Lexer* lexer) {
#define ASCII_ZERO 48
#define ASCII_NINE 57
    return *lexer->current >= ASCII_ZERO && *lexer->current <= ASCII_NINE;
#undef ASCII_ZERO
#undef ASCII_NINE
}

static Token scan_number(Lexer* lexer) {
    while (is_numeric(lexer)) {
        lexer->current++;
    }
    if (!match(lexer, '.')) {
        return create_token(lexer, TOKEN_INTEGER);
    }
    advance(lexer); // consume dot
    if (!is_numeric(lexer)) {
        return create_error(lexer, "Malformed float: Expected to have numbers after dot");
    }
    while (is_numeric(lexer)) {
        lexer->current++;
    }
    return create_token(lexer, TOKEN_FLOAT);
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
    switch (*lexer->current++) {
    case '+': return create_token(lexer, TOKEN_PLUS);
    case '-': return create_token(lexer, TOKEN_MINUS);
    case '*': return create_token(lexer, TOKEN_STAR);
    case '/': return create_token(lexer, TOKEN_SLASH);
    case '%': return create_token(lexer, TOKEN_PERCENT);
    case '(': return create_token(lexer, TOKEN_LEFT_PAREN);
    case ')': return create_token(lexer, TOKEN_RIGHT_PAREN);
    case '.': return create_token(lexer, TOKEN_DOT);
    default: return create_token(lexer, TOKEN_ERROR);
    }
}
