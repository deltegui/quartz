#ifndef QUARTZ_TOKEN_H
#define QUARTZ_TOKEN_H

#include "common.h"

namespace Quartz {

enum class TokenType {
    // Special tokens
    End,
    Error,

    // Single character tokens
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    LeftParen,
    RightParen,
    Dot,

    // Multi-character tokens
    Integer,
    Float,
};

typedef struct {
    TokenType type;
    const char* start;
    uint8_t length;
    uint32_t line;
} Token;

void print_token(Token token);

}

#endif