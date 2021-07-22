#include "type.h"
#include <stdio.h>

static void free_type(Type* type);
static Type* type_pool_add(Type type);
static Type* alloc_type(Type type);

Type* type_pool = NULL;

Type number_type;
Type bool_type;
Type nil_type;
Type string_type;
Type void_type;
Type unknown_type;

void init_type_pool() {
    number_type.kind = TYPE_NUMBER;
    bool_type.kind = TYPE_BOOL;
    nil_type.kind = TYPE_NIL;
    string_type.kind = TYPE_STRING;
    void_type.kind = TYPE_VOID;
    unknown_type.kind = TYPE_UNKNOWN;
}

void free_type_pool() {
    Type* current = type_pool;
    while (current != NULL) {
        Type* next = current->next;
        free_type(current);
        current = next;
    }
}

static void free_type(Type* type) {
    switch (type->kind) {
    case TYPE_FUNCTION: {
        free_vector(&type->function->param_types);
        free(type->function);
        break;
    }
    default:
        // Simple types do not have anything to free
        break;
    }
    free(type);
}

static Type* type_pool_add(Type type) {
    if (type_pool == NULL) {
        type_pool = alloc_type(type);
        return type_pool;
    }
    Type* current = type_pool;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = alloc_type(type);
    return current->next;
}

static Type* alloc_type(Type type) {
    Type* ptr = (Type*) malloc(sizeof(Type));
    *ptr = type;
    return ptr;
}

Type* create_type_simple(TypeKind kind) {
    switch (kind) {
    case TYPE_NUMBER: return &number_type;
    case TYPE_BOOL: return &bool_type;
    case TYPE_NIL: return &nil_type;
    case TYPE_STRING: return &string_type;
    case TYPE_VOID: return &void_type;
    case TYPE_UNKNOWN: return &unknown_type;
    default:
        // If we reach this assert, you forget to put a case in the swith
        // or you are calling the wrong function.
        assert(false);
        return &unknown_type;
    }
}

Type* create_type_function() {
    FunctionType* fn_type = (FunctionType*) malloc(sizeof(FunctionType));
    init_vector(&fn_type->param_types, sizeof(Type*));
    fn_type->return_type = CREATE_TYPE_VOID();
    Type type = (Type) {
        .kind = TYPE_FUNCTION,
        .function = fn_type,
    };
    return type_pool_add(type);
}

// TODO this function may be dead code. Check references.
/*
Type* type_from_obj_kind(ObjKind kind) {
    switch (kind) {
    case OBJ_STRING: return CREATE_TYPE_STRING();
    case OBJ_FUNCTION: return SIMPLE_TYPE(TYPE_FUNCTION);
    default: return CREATE_TYPE_UNKNOWN();
    }
}
*/

Type* type_from_token_kind(TokenKind kind) {
    switch (kind) {
    case TOKEN_TYPE_NUMBER: return CREATE_TYPE_NUMBER();
    case TOKEN_TYPE_STRING: return CREATE_TYPE_STRING();
    case TOKEN_TYPE_BOOL: return CREATE_TYPE_BOOL();
    case TOKEN_TYPE_NIL: return CREATE_TYPE_NIL();
    case TOKEN_TYPE_VOID: return CREATE_TYPE_VOID();
    default: return CREATE_TYPE_UNKNOWN();
    }
}

void type_print(Type* type) {
    switch (type->kind) {
    case TYPE_NUMBER: printf("Number"); break;
    case TYPE_BOOL: printf("Bool"); break;
    case TYPE_NIL: printf("Nil"); break;
    case TYPE_STRING: printf("String"); break;
    case TYPE_FUNCTION: printf("Function"); break;
    case TYPE_UNKNOWN: printf("Unknown"); break;
    case TYPE_VOID: printf("Void"); break;
    }
}
