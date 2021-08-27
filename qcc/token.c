#include "token.h"
#include <string.h>

// TODO token equals should not be necessary

bool token_equals(const Token* const first, const Token* const second) {
    if (
        first->kind != second->kind ||
        first->length != second->length ||
        first->line != second->line
    ) {
        return false;
    }
    return memcmp(first->start, second->start, sizeof(char) * first->length) == 0;
}
