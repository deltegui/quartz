#include <stdio.h>

#include "lexer.h"

namespace Quartz {

Lexer::Lexer(const char* buffer) {
    current = buffer;
    start = buffer;
    line = 1;
}

Token Lexer::next_token() {
    skip_whitespaces();
    if (is_at_end()) {
        return create_token(TokenType::End);
    }
    start = current;
    if (is_numeric()) {
        return this->scan_number();
    }
    switch (*current++) {
    case '+': return create_token(TokenType::Plus);
    case '-': return create_token(TokenType::Minus);
    case '*': return create_token(TokenType::Star);
    case '/': return create_token(TokenType::Slash);
    case '%': return create_token(TokenType::Percent);
    case '(': return create_token(TokenType::LeftParen);
    case ')': return create_token(TokenType::RightParen);
    case '.': return create_token(TokenType::Dot);
    default: return create_token(TokenType::Error);
    }
}

Token Lexer::create_token(TokenType type) {
    Token token;
    token.length = static_cast<int>(this->current - this->start);
    token.line = this->line;
    token.start = this->start;
    token.type = type;
    return token;
}

void Lexer::skip_whitespaces() {
#define CONSUME_UNTIL(character)\
while(this->peek() != character && !is_at_end())\
this->advance()

    for (;;) {
        switch (*this->current) {
        case '\n':
            this->line++;
        case ' ':
        case '\t':
        case '\r':
            this->advance();
            break;
        case '/':
            if (this->match_next('/')) {
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

Token Lexer::scan_number() {
    while (is_numeric()) {
        current++;
    }
    if (!match('.')) {
        return create_token(TokenType::Integer);
    }
    advance(); // consume dot
    while (is_numeric()) {
        current++;
    }
    return create_token(TokenType::Float);
}

void Lexer::advance() {
    if (is_at_end()) {
        return;
    }
    current++;
}

bool Lexer::match_next(char next) {
    if (is_at_end()) {
        return false;
    }
    return *(current + 1) == next;
}

bool Lexer::match(char c) {
    if (is_at_end()) {
        return false;
    }
    return (*current) == c;
}

bool Lexer::is_numeric() {
#define ASCII_ZERO 48
#define ASCII_NINE 57
    return *current >= ASCII_ZERO && *current <= ASCII_NINE;
#undef ASCII_ZERO
#undef ASCII_NINE
}

char Lexer::peek() {
    return *current;
}

bool Lexer::is_at_end() {
    return *current == '\0';
}

}
