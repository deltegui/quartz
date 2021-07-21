#ifndef QUARTZ_TYPES_H_
#define QUARTZ_TYPES_H_

#include "lexer.h"
#include "obj_kind.h"

typedef enum {
    TYPE_NUMBER,
    TYPE_BOOL,
    TYPE_NIL,
    TYPE_STRING,
    TYPE_FUNCTION,
    TYPE_VOID,
    TYPE_UNKNOWN,
} Type;

Type type_from_obj_kind(ObjKind kind);
Type type_from_token_kind(TokenKind kind);
void type_print(Type type);

#define VECTOR_AS_TYPES(vect) VECTOR_AS(vect, Type)
#define VECTOR_ADD_TYPE(vect, type) VECTOR_ADD(vect, type, Type)

#endif
