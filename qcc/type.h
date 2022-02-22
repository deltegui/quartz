#ifndef QUARTZ_TYPES_H_
#define QUARTZ_TYPES_H_

#include "lexer.h"
#include "obj_kind.h"
#include "vector.h"

typedef enum {
    TYPE_CLASS,
    TYPE_OBJECT,
    TYPE_ALIAS,
    TYPE_NUMBER,
    TYPE_BOOL,
    TYPE_NIL,
    TYPE_STRING,
    TYPE_FUNCTION,
    TYPE_VOID,
    TYPE_UNKNOWN,
} TypeKind;

struct s_type;

typedef struct {
    Vector param_types;
    struct s_type* return_type;
} FunctionType;

typedef struct {
    struct s_type* def;
    char* identifier;
} AliasType;

typedef struct {
    int length;
    char* identifier;
} ClassType;

typedef struct {
    struct s_type* klass;
} ObjectType;

typedef struct s_type {
    TypeKind kind;
    union {
        FunctionType function;
        AliasType alias;
        ClassType klass;
        ObjectType object;
    };
} Type;

#define TYPE_ALIAS_RESOLVE(type_alias) ((type_alias)->alias.def)
#define RESOLVE_IF_TYPEALIAS(type) ((type)->kind == TYPE_ALIAS) ? TYPE_ALIAS_RESOLVE(type) : type

#define TYPE_FN_RETURN(type_fn) ((type_fn)->function.return_type)
#define TYPE_FN_PARAMS(type_fn) ((type_fn)->function.param_types)
#define TYPE_FN_ADD_PARAM(type_fn, param_type) (VECTOR_ADD_TYPE(&TYPE_FN_PARAMS(type_fn), param_type))

#define TYPE_OBJECT_CLASS_NAME(type_obj) ((type_obj)->object.klass->klass.identifier)
#define TYPE_OBJECT_CLASS_LENGTH(type_obj) ((type_obj)->object.klass->klass.length)

#define TYPE_IS_OBJECT(type) ((type)->kind == TYPE_OBJECT)
#define TYPE_IS_CLASS(type) ((type)->kind == TYPE_CLASS)
#define TYPE_IS_ALIAS(type) ((type)->kind == TYPE_ALIAS)
#define TYPE_IS_NUMBER(type) ((type)->kind == TYPE_NUMBER)
#define TYPE_IS_BOOL(type) ((type)->kind == TYPE_BOOL)
#define TYPE_IS_NIL(type) ((type)->kind == TYPE_NIL)
#define TYPE_IS_STRING(type) ((type)->kind == TYPE_STRING)
#define TYPE_IS_FUNCTION(type) ((type)->kind == TYPE_FUNCTION)
#define TYPE_IS_VOID(type) ((type)->kind == TYPE_VOID)
#define TYPE_IS_UNKNOWN(type) ((type)->kind == TYPE_UNKNOWN)

void init_type_pool();
void free_type_pool();

Type* create_type_simple(TypeKind kind);
Type* create_type_function();
Type* create_type_alias(const char* identifier, int length, Type* original);
Type* create_type_class(const char* identifier, int length);
Type* create_type_object(Type* klass);

#define CREATE_TYPE_NUMBER() create_type_simple(TYPE_NUMBER)
#define CREATE_TYPE_BOOL() create_type_simple(TYPE_BOOL)
#define CREATE_TYPE_NIL() create_type_simple(TYPE_NIL)
#define CREATE_TYPE_STRING() create_type_simple(TYPE_STRING)
#define CREATE_TYPE_VOID() create_type_simple(TYPE_VOID)
#define CREATE_TYPE_UNKNOWN() create_type_simple(TYPE_UNKNOWN)

#define TYPE_PRINT(typ) type_fprint(stdout, typ)
#define ERR_TYPE_PRINT(typ) type_fprint(stderr, typ)

#define TYPE_IS_ASSIGNABLE(var_type, expr_type) ((TYPE_IS_NIL(expr_type) && TYPE_IS_OBJECT(var_type)) || type_equals(var_type, expr_type))

Type* simple_type_from_token_kind(TokenKind kind);
void type_fprint(FILE* out, const Type* const type);
bool type_equals(Type* first, Type* second);

#define VECTOR_AS_TYPES(vect) VECTOR_AS(vect, Type*)
#define VECTOR_ADD_TYPE(vect, type) VECTOR_ADD(vect, type, Type*)

#endif
