#ifndef QUARTZ_LEXER_H
#define QUARTZ_LEXER_H

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
    TOKEN_BANG,

    // Two character tokens
    TOKEN_AND,
    TOKEN_OR,

    // Multi-character tokens
    TOKEN_NUMBER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NIL,
    TOKEN_STRING,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    uint8_t length;
    uint32_t line;
} Token;

typedef struct {
    const char* start;
    const char* current;
    int line;
} Lexer;

// Initialize a exiting lexer using
// the source file buffer. Notice that Tokens
// returned by next_token function will use
// that buffer, so dont free it until all tokens
// are deleted too.
void init_lexer(Lexer* lexer, const char* buffer);

// Get next token from a lexer. If a TOKEN_ERROR is returned
// it means was an error. If a TOKEN_END is returned you have
// reached the end of the buffer.
Token next_token(Lexer* lexer);

#endif
