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

typedef struct {
    const char* start;
    const char* current;
    int line;
} Lexer;

// Creates a new Lexer struct in the heap, using
// the source file buffer. Notice that Tokens
// returned by next_token function will use
// that buffer, so dont free it until all tokens
// are deleted too.
Lexer* create_lexer(const char* buffer);

// Free a lexer and its internal references.
void free_lexer(Lexer* lexer);

// Get next token from a lexer. If a TOKEN_ERROR is returned
// it means was an error. If a TOKEN_END is returned you have
// reached the end of the buffer.
Token next_token(Lexer* lexer);

#endif
