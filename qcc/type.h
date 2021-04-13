#ifndef QUARTZ_TYPES_H
#define QUARTZ_TYPES_H

#include "lexer.h"

typedef enum {
    NUMBER_TYPE,
    BOOL_TYPE,
    NIL_TYPE,
    STRING_TYPE,
    FUNCTION_TYPE,
    UNKNOWN_TYPE,
} Type;

Type type_from_token_kind(TokenKind kind);
void type_print(Type type);

#endif
