#include "type.h"
#include <stdio.h>

Type type_from_token_kind(TokenKind kind) {
    switch (kind) {
    case TOKEN_NUMBER_TYPE: return NUMBER_TYPE;
    case TOKEN_STRING_TYPE: return STRING_TYPE;
    case TOKEN_BOOL_TYPE: return BOOL_TYPE;
    case TOKEN_NIL_TYPE: return NIL_TYPE;
    default: return UNKNOWN_TYPE;
    }
}

void type_print(Type type) {
    switch (type) {
    case NUMBER_TYPE: printf("Number"); break;
    case BOOL_TYPE: printf("Bool"); break;
    case NIL_TYPE: printf("Nil"); break;
    case STRING_TYPE: printf("String"); break;
    case FUNCTION_TYPE: printf("Function"); break;
    case UNKNOWN_TYPE: printf("Unknown"); break;
    }
}
