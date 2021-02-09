#ifndef QUARTZ_LEXER_H
#define QUARTZ_LEXER_H

#include <stdint.h>

#include "token.h"

namespace Quartz {

class Lexer {
    const char* start;
    const char* current;
    int line;

    void skip_whitespaces();
    void advance();
    bool match_next(char next);
    char peek();
    bool is_at_end();
    bool is_numeric();
    bool match(char current);
    Token scan_number();
    Token create_token(TokenType type);

public:
    Lexer(const char* buffer);
    Token next_token();
};

}

#endif
