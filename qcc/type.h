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
    Vector param_types;
    Type* return_type;
} FunctionType;

#define TYPE_FN_RETURN(type_fn) (type_fn->function->return_type)
#define TYPE_FN_PARAMS(type_fn) (type_fn->function->param_types)

#define TYPE_IS_NUMBER(type) (type->kind == TYPE_NUMBER)
#define TYPE_IS_BOOL(type) (type->kind == TYPE_BOOL)
#define TYPE_IS_NIL(type) (type->kind == TYPE_NIL)
#define TYPE_IS_STRING(type) (type->kind == TYPE_STRING)
#define TYPE_IS_FUNCTION(type) (type->kind == TYPE_FUNCTION)
#define TYPE_IS_VOID(type) (type->kind == TYPE_VOID)
#define TYPE_IS_UNKNOWN(type) (type->kind == TYPE_UNKNOWN)

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

#define TYPE_PRINT(typ) type_fprint(stdout, typ)
#define ERR_TYPE_PRINT(typ) type_fprint(stderr, typ)

Type* simple_type_from_token_kind(TokenKind kind);
void type_fprint(FILE* out, const Type* const type);
bool type_equals(Type* first, Type* second);

#define VECTOR_AS_TYPES(vect) VECTOR_AS(vect, Type*)
#define VECTOR_ADD_TYPE(vect, type) VECTOR_ADD(vect, type, Type*)

#endif
