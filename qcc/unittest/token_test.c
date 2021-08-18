#include <stdarg.h>
#include "./common.h"
#include "../token.h"

void should_compare_to_tokens_successfully() {
    Token first = (Token){
        .kind = TOKEN_NIL,
        .start = "nil",
        .length = 3,
        .line = 34,
    };
    Token second = (Token){
        .kind = TOKEN_NIL,
        .start = "nil",
        .length = 3,
        .line = 34,
    };
    assert_true(token_equals(&first, &second));
}

void should_compare_to_tokens_error() {
    Token first = (Token){
        .kind = TOKEN_IDENTIFIER,
        .start = "adios",
        .length = 5,
        .line = 34,
    };
    Token second = (Token){
        .kind = TOKEN_IDENTIFIER,
        .start = "hola",
        .length = 4,
        .line = 34,
    };
    assert_false(token_equals(&first, &second));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_compare_to_tokens_successfully),
        cmocka_unit_test(should_compare_to_tokens_error)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
