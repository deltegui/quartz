#ifndef QUARTZ_LEXER_H_
#define QUARTZ_LEXER_H_

#include "common.h"
#include "token.h"

typedef struct {
    const char* start;
    const char* current;
    uint32_t line;
} Lexer;

void init_lexer(Lexer* lexer, const char* buffer);
Token next_token(Lexer* lexer);

#endif
