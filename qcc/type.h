#ifndef QUARTZ_TYPES_H
#define QUARTZ_TYPES_H

#include "lexer.h"

typedef enum {
    TYPE_NUMBER,
    TYPE_BOOL,
    TYPE_NIL,
    TYPE_STRING,
    TYPE_FUNCTION,
    TYPE_VOID,
    TYPE_UNKNOWN,
} Type;

Type type_from_token_kind(TokenKind kind);
void type_print(Type type);

#endif
