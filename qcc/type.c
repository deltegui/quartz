#include "type.h"
#include <stdio.h>

Type type_from_obj_kind(ObjKind kind) {
    switch (kind) {
    case OBJ_STRING: return SIMPLE_TYPE(TYPE_STRING);
    case OBJ_FUNCTION: return SIMPLE_TYPE(TYPE_FUNCTION);
    default: return SIMPLE_TYPE(TYPE_UNKNOWN);
    }
}

Type type_from_token_kind(TokenKind kind) {
    switch (kind) {
    case TOKEN_TYPE_NUMBER: return SIMPLE_TYPE(TYPE_NUMBER);
    case TOKEN_TYPE_STRING: return SIMPLE_TYPE(TYPE_STRING);
    case TOKEN_TYPE_BOOL: return SIMPLE_TYPE(TYPE_BOOL);
    case TOKEN_TYPE_NIL: return SIMPLE_TYPE(TYPE_NIL);
    case TOKEN_TYPE_VOID: return SIMPLE_TYPE(TYPE_VOID);
    default: return SIMPLE_TYPE(TYPE_UNKNOWN);
    }
}

void type_print(Type type) {
    switch (type.kind) {
    case TYPE_NUMBER: printf("Number"); break;
    case TYPE_BOOL: printf("Bool"); break;
    case TYPE_NIL: printf("Nil"); break;
    case TYPE_STRING: printf("String"); break;
    case TYPE_FUNCTION: printf("Function"); break;
    case TYPE_UNKNOWN: printf("Unknown"); break;
    case TYPE_VOID: printf("Void"); break;
    }
}
