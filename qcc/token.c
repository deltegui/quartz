#include <string.h>

#include "token.h"

static const char* token_type_str(TokenType type) {
    switch (type) {
    case TOKEN_END: return "End";
    case TOKEN_ERROR: return "Error";
    case TOKEN_PLUS: return "Plus";
    case TOKEN_MINUS: return "Minus";
    case TOKEN_STAR: return "Star";
    case TOKEN_SLASH: return "Slash";
    case TOKEN_PERCENT: return "Percent";
    case TOKEN_LEFT_PAREN: return "LeftParen";
    case TOKEN_RIGHT_PAREN: return "RightParen";
    case TOKEN_DOT: return "Dot";
    case TOKEN_INTEGER: return "Integer";
    case TOKEN_FLOAT: return "Float";
    default: return "Unknown";
    }
}

void print_token(Token token) {
    printf(
        "Token{ Type: '%s', Line: '%d', Value: '%.*s', Length: '%d' }\n",
        token_type_str(token.type),
        token.line,
        token.length,
        token.start,
        token.length);
}