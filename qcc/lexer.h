#ifndef QUARTZ_LEXER_H_
#define QUARTZ_LEXER_H_

#include "common.h"
#include "token.h"

typedef struct {
    const char* start;
    const char* current;
    uint32_t line;
} Lexer;

void init_lexer(Lexer* const lexer, const char* const buffer);
Token next_token(Lexer* const lexer);

#endif
