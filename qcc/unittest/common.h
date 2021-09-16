#ifndef QUARTZ_TESTS
#define QUARTZ_TESTS

#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <cmocka.h>

#include "../lexer.h"

bool t_token_equals(Token* first, Token* second) {
#define ERROR(message) fprintf(\
    stderr,\
    "Token %.*s does not equal %.*s: %s\n",\
    first->length,\
    first->start,\
    second->length,\
    second->start,\
    message)

    if (first->length != second->length) {
        ERROR("Length differs");
        return false;
    }
    int str_equal = memcmp(
        first->start,
        second->start,
        first->length);
    if (str_equal != 0) {
        ERROR("Text differs");
        return false;
    }
    if (first->kind != second->kind) {
        ERROR("Kind differs");
        return false;
    }
    if (first->line != second->line) {
        ERROR("Line differs");
        return false;
    }
    return true;

#undef ERROR
}

#endif
