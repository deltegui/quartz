#ifndef QUARTZ_TYPES_H_
#define QUARTZ_TYPES_H_

#include "lexer.h"
#include "obj_kind.h"
#include "vector.h"

typedef enum {
    TYPE_NUMBER,
    TYPE_BOOL,
    TYPE_NIL,
    TYPE_STRING,
    TYPE_FUNCTION,
    TYPE_VOID,
    TYPE_UNKNOWN,
} TypeKind;

struct s_func_type;

typedef struct {
    TypeKind kind;
    union {
        struct s_func_type* function;
    };
} Type;

typedef struct s_func_type {
    Vector* param_types;
    Type return_type;
} FunctionType;

#define SIMPLE_TYPE(knd) ((Type){ .kind = knd })
#define TYPE_IS_KIND(type, knd) (type.kind == knd)
#define TYPE_EQUALS(first, second) (first.kind == second.kind)

Type type_from_obj_kind(ObjKind kind);
Type type_from_token_kind(TokenKind kind);
void type_print(Type type);

#define VECTOR_AS_TYPES(vect) VECTOR_AS(vect, Type)
#define VECTOR_ADD_TYPE(vect, type) VECTOR_ADD(vect, type, Type)

#endif
