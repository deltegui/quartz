#include <stdarg.h>
#include "./common.h"

#include "../lexer.h"

#define ASSERT_TOKEN_TYPE(tkn, t) assert_true(tkn.type == t)

static void assert_types(const char* source, int size, ...) {
    Lexer lexer;
    init_lexer(&lexer, source);
    va_list tokens;
    va_start(tokens, size);
    for (int i = 0; i < size; i++) {
        Token current = next_token(&lexer);
        ASSERT_TOKEN_TYPE(current, va_arg(tokens, TokenType));
    }
    Token end = next_token(&lexer);
    ASSERT_TOKEN_TYPE(end, TOKEN_END);
    va_end(tokens);
}

static void assert_tokens(const char* source, int size, ...) {
    Lexer lexer;
    init_lexer(&lexer, source);
    va_list tokens;
    va_start(tokens, size);
    for (int i = 0; i < size; i++) {
        Token current = next_token(&lexer);
        Token expected = va_arg(tokens, Token);
        assert_true(current.type == expected.type);
        assert_true(current.line == expected.line);
        assert_true(current.length == expected.length);

        char actual[current.length + 1];
        sprintf(actual, "%.*s", current.length, current.start);
        assert_string_equal(actual, expected.start);
    }
    Token end = next_token(&lexer);
    ASSERT_TOKEN_TYPE(end, TOKEN_END);
    va_end(tokens);
}

static void should_scan_empty_text() {
    assert_types(
        "",
        1,
        TOKEN_END
    );
}

static void should_omit_spaces() {
    assert_types(
        "        \n  \t \n\r      ",
        1,
        TOKEN_END
    );
}

static void should_scan_numbers() {
    assert_types(
        "1 +    3.2",
        3,
        TOKEN_INTEGER,
        TOKEN_PLUS,
        TOKEN_FLOAT
    );
}

static void should_scan_arithmetic_operators() {
    assert_types(
        "   +   -   /    *  %  )  (      \n  . ",
        8,
        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_SLASH,
        TOKEN_STAR,
        TOKEN_PERCENT,
        TOKEN_RIGHT_PAREN,
        TOKEN_LEFT_PAREN,
        TOKEN_DOT
    );
}

static void should_scan_boolean_operators() {
    assert_types(
        "  !   && ! || &&",
        5,
        TOKEN_BANG,
        TOKEN_AND,
        TOKEN_BANG,
        TOKEN_OR,
        TOKEN_AND
    );
}

static void should_scan_reserved_words() {
    assert_tokens(
        "true false",
        2,
        (Token){
            .length = 4,
            .line = 1,
            .start = "true",
            .type = TOKEN_TRUE
        },
        (Token){
            .length = 5,
            .line = 1,
            .start = "false",
            .type = TOKEN_FALSE
        }
    );
}

static void should_scan_reserved_words_correctly() {
    assert_tokens(
        "  true\n  (false)",
        4,
        (Token){
            .length = 4,
            .line = 1,
            .start = "true",
            .type = TOKEN_TRUE
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = "(",
            .type = TOKEN_LEFT_PAREN
        },
        (Token){
            .length = 5,
            .line = 1,
            .start = "false",
            .type = TOKEN_FALSE
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = ")",
            .type = TOKEN_RIGHT_PAREN
        }
    );
}

static void should_create_number_tokens_correctly() {
    assert_tokens(
        "13.2323    9043  ",
        2,
        (Token){
            .length = 7,
            .line = 1,
            .start = "13.2323",
            .type = TOKEN_FLOAT
        },
        (Token){
            .length = 4,
            .line = 1,
            .start = "9043",
            .type = TOKEN_INTEGER
        }
    );
}

static void should_fail_if_float_is_malformed() {
    assert_types(
        "  3.   ",
        1,
        TOKEN_ERROR
    );
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_scan_empty_text),
        cmocka_unit_test(should_omit_spaces),
        cmocka_unit_test(should_scan_arithmetic_operators),
        cmocka_unit_test(should_create_number_tokens_correctly),
        cmocka_unit_test(should_fail_if_float_is_malformed),
        cmocka_unit_test(should_scan_numbers),
        cmocka_unit_test(should_scan_reserved_words),
        cmocka_unit_test(should_scan_boolean_operators)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
