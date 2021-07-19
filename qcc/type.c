#include "type.h"
#include <stdio.h>

Type type_from_token_kind(TokenKind kind) {
    switch (kind) {
    case TOKEN_TYPE_NUMBER: return TYPE_NUMBER;
    case TOKEN_TYPE_STRING: return TYPE_STRING;
    case TOKEN_TYPE_BOOL: return TYPE_BOOL;
    case TOKEN_TYPE_NIL: return TYPE_NIL;
    case TOKEN_TYPE_VOID: return TYPE_VOID;
    default: return TYPE_UNKNOWN;
    }
}

void type_print(Type type) {
    switch (type) {
    case TYPE_NUMBER: printf("Number"); break;
    case TYPE_BOOL: printf("Bool"); break;
    case TYPE_NIL: printf("Nil"); break;
    case TYPE_STRING: printf("String"); break;
    case TYPE_FUNCTION: printf("Function"); break;
    case TYPE_UNKNOWN: printf("Unknown"); break;
    case TYPE_VOID: printf("Void"); break;
    }
}
