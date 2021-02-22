#ifndef QUARTZ_LEXER_H
#define QUARTZ_LEXER_H

#include "common.h"
#include "token.h"

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
