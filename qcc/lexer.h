#ifndef QUARTZ_LEXER_H_
#define QUARTZ_LEXER_H_

#include "common.h"
#include "token.h"

typedef struct {
    const char* source;
    const char* start;
    const char* current;
    uint32_t line;
    uint32_t column;
} Lexer;

void init_lexer(Lexer* const lexer, const char* const buffer);
Token next_token(Lexer* const lexer);

#endif
