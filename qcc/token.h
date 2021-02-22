#ifndef QUARTZ_TOKEN_H
#define QUARTZ_TOKEN_H

#include "common.h"

typedef enum {
    // Special tokens
    TOKEN_END,
    TOKEN_ERROR,

    // Single character tokens
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_DOT,

    // Multi-character tokens
    TOKEN_INTEGER,
    TOKEN_FLOAT,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    uint8_t length;
    uint32_t line;
} Token;

void print_token(Token token);

#endif