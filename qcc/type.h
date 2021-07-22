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

typedef struct s_type {
    TypeKind kind;
    struct s_type* next;
    union {
        struct s_func_type* function;
    };
} Type;

typedef struct s_func_type {
    Vector param_types;
    Type* return_type;
} FunctionType;

// TODO What we do with this interface?
// #define TYPE_FUNCTION_RETURN_IS_KIND(type_fn, knd) (type_fn->function->return_type->kind == knd)

#define TYPE_IS_KIND(type, knd) (type->kind == knd)
#define TYPE_EQUALS(first, second) (first->kind == second->kind)

// TODO delete this shit if we choose to use a linked list.
void init_type_pool();
void free_type_pool();

Type* create_type_simple(TypeKind kind);
Type* create_type_function();

#define CREATE_TYPE_NUMBER() create_type_simple(TYPE_NUMBER)
#define CREATE_TYPE_BOOL() create_type_simple(TYPE_BOOL)
#define CREATE_TYPE_NIL() create_type_simple(TYPE_NIL)
#define CREATE_TYPE_STRING() create_type_simple(TYPE_STRING)
#define CREATE_TYPE_VOID() create_type_simple(TYPE_VOID)
#define CREATE_TYPE_UNKNOWN() create_type_simple(TYPE_UNKNOWN)

// Type* type_from_obj_kind(ObjKind kind);
Type* type_from_token_kind(TokenKind kind);
void type_print(Type* type);

#define VECTOR_AS_TYPES(vect) VECTOR_AS(vect, Type*)
#define VECTOR_ADD_TYPE(vect, type) VECTOR_ADD(vect, type, Type*)

#endif
