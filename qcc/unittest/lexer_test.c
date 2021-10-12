#include <stdarg.h>
#include "./common.h"
#include "../lexer.h"

#define ASSERT_TOKEN_TYPE(tkn, t) assert_true(tkn.kind == t)

static void assert_types(const char* source, int size, ...) {
    Lexer lexer;
    init_lexer(&lexer, source);
    va_list tokens;
    va_start(tokens, size);
    for (int i = 0; i < size; i++) {
        Token current = next_token(&lexer);
        ASSERT_TOKEN_TYPE(current, va_arg(tokens, TokenKind));
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
        assert_true(current.kind == expected.kind);
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
        TOKEN_NUMBER,
        TOKEN_PLUS,
        TOKEN_NUMBER
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

static void should_scan_braces() {
    assert_types(
        " {   }   ",
        2,
        TOKEN_LEFT_BRACE,
        TOKEN_RIGHT_BRACE
    );
}

static void should_scan_boolean_operators() {
    assert_types(
        "  !   && ! || &&   ==   !=   =",
        8,
        TOKEN_BANG,
        TOKEN_AND,
        TOKEN_BANG,
        TOKEN_OR,
        TOKEN_AND,
        TOKEN_EQUAL_EQUAL,
        TOKEN_BANG_EQUAL,
        TOKEN_EQUAL
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
            .kind = TOKEN_TRUE
        },
        (Token){
            .length = 5,
            .line = 1,
            .start = "false",
            .kind = TOKEN_FALSE
        }
    );
}

static void should_scan_reserved_words_correctly() {
    assert_tokens(
        "  true\n  (false) \r\n nil",
        4,
        (Token){
            .length = 4,
            .line = 1,
            .start = "true",
            .kind = TOKEN_TRUE
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = "(",
            .kind = TOKEN_LEFT_PAREN
        },
        (Token){
            .length = 5,
            .line = 1,
            .start = "false",
            .kind = TOKEN_FALSE
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = ")",
            .kind = TOKEN_RIGHT_PAREN
        },
        (Token){
            .length = 3,
            .line = 1,
            .start = "nil",
            .kind = TOKEN_NIL
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
            .kind = TOKEN_NUMBER
        },
        (Token){
            .length = 4,
            .line = 1,
            .start = "9043",
            .kind = TOKEN_NUMBER
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

static void should_create_string_tokens_correctly() {
    assert_tokens(
        "\"\"    'Hola' \n    \"Hola Mundo!! ñ\"  ",
        3,
        (Token){
            .length = 0,
            .line = 1,
            .start = "",
            .kind = TOKEN_STRING
        },
        (Token){
            .length = 4,
            .line = 1,
            .start = "Hola",
            .kind = TOKEN_STRING
        },
        (Token){
            .length = 15,
            .line = 2,
            .start = "Hola Mundo!! ñ",
            .kind = TOKEN_STRING
        }
    );
}

static void should_fail_if_string_is_malformed() {
    assert_types(
        "    ' este string no se acaba  ",
        1,
        TOKEN_ERROR
    );
}

static void should_scan_global_declarations() {
    assert_tokens(
        "   var demo = 12;     ",
        5,
        (Token){
            .length = 3,
            .line = 1,
            .start = "var",
            .kind = TOKEN_VAR
        },
        (Token){
            .length = 4,
            .line = 1,
            .start = "demo",
            .kind = TOKEN_IDENTIFIER
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = "=",
            .kind = TOKEN_EQUAL
        },
        (Token){
            .length = 2,
            .line = 1,
            .start = "12",
            .kind = TOKEN_NUMBER
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = ";",
            .kind = TOKEN_SEMICOLON
        }
    );
}

static void should_scan_global_declarations_with_types() {
    assert_tokens(
        "   var demo: Number = 6;     ",
        7,
        (Token){
            .length = 3,
            .line = 1,
            .start = "var",
            .kind = TOKEN_VAR
        },
        (Token){
            .length = 4,
            .line = 1,
            .start = "demo",
            .kind = TOKEN_IDENTIFIER
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = ":",
            .kind = TOKEN_COLON
        },
        (Token){
            .length = 6,
            .line = 1,
            .start = "Number",
            .kind = TOKEN_TYPE_NUMBER
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = "=",
            .kind = TOKEN_EQUAL
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = "6",
            .kind = TOKEN_NUMBER
        },
        (Token){
            .length = 1,
            .line = 1,
            .start = ";",
            .kind = TOKEN_SEMICOLON
        }
    );
}

static void should_tokenize_type_names() {
    assert_types(
        "  Number String   Bool Nil   Void",
        5,
        TOKEN_TYPE_NUMBER,
        TOKEN_TYPE_STRING,
        TOKEN_TYPE_BOOL,
        TOKEN_TYPE_NIL,
        TOKEN_TYPE_VOID
    );
}

static void should_tokenize_function_declarations() {
    assert_types(
        " fn hello(a: String, b: Number) {} ",
        13,
        TOKEN_FUNCTION,
        TOKEN_IDENTIFIER,
        TOKEN_LEFT_PAREN,
        TOKEN_IDENTIFIER,
        TOKEN_COLON,
        TOKEN_TYPE_STRING,
        TOKEN_COMMA,
        TOKEN_IDENTIFIER,
        TOKEN_COLON,
        TOKEN_TYPE_NUMBER,
        TOKEN_RIGHT_PAREN,
        TOKEN_LEFT_BRACE,
        TOKEN_RIGHT_BRACE
    );
}

static void should_tokenize_returns() {
    assert_types(
        " return 22; ",
        3,
        TOKEN_RETURN,
        TOKEN_NUMBER,
        TOKEN_SEMICOLON
    );
}

static void should_tokenize_print_correctly() {
    assert_types(
        " print_ast(ast); ",
        5,
        TOKEN_IDENTIFIER,
        TOKEN_LEFT_PAREN,
        TOKEN_IDENTIFIER,
        TOKEN_RIGHT_PAREN,
        TOKEN_SEMICOLON
    );
}

static void should_tokenize_if_correctly() {
    assert_types(
        " if (true) ",
        4,
        TOKEN_IF,
        TOKEN_LEFT_PAREN,
        TOKEN_TRUE,
        TOKEN_RIGHT_PAREN
    );
}

static void should_tokenize_else_correctly() {
    assert_types(
        " if (bla) else ",
        5,
        TOKEN_IF,
        TOKEN_LEFT_PAREN,
        TOKEN_IDENTIFIER,
        TOKEN_RIGHT_PAREN,
        TOKEN_ELSE
    );
}

static void should_tokenize_for_correctly() {
    assert_types(
        "   for (true) {}",
        6,
        TOKEN_FOR,
        TOKEN_LEFT_PAREN,
        TOKEN_TRUE,
        TOKEN_RIGHT_PAREN,
        TOKEN_LEFT_BRACE,
        TOKEN_RIGHT_BRACE
    );
}

static void should_tokenize_while_correctly() {
    assert_types(
        "   while (true) {}",
        6,
        TOKEN_WHILE,
        TOKEN_LEFT_PAREN,
        TOKEN_TRUE,
        TOKEN_RIGHT_PAREN,
        TOKEN_LEFT_BRACE,
        TOKEN_RIGHT_BRACE
    );
}

static void should_tokenize_break_correctly() {
    assert_types(
        "   break; ",
        2,
        TOKEN_BREAK,
        TOKEN_SEMICOLON
    );
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_tokenize_print_correctly),
        cmocka_unit_test(should_tokenize_returns),
        cmocka_unit_test(should_tokenize_function_declarations),
        cmocka_unit_test(should_scan_braces),
        cmocka_unit_test(should_tokenize_type_names),
        cmocka_unit_test(should_scan_global_declarations_with_types),
        cmocka_unit_test(should_scan_global_declarations),
        cmocka_unit_test(should_scan_empty_text),
        cmocka_unit_test(should_omit_spaces),
        cmocka_unit_test(should_scan_arithmetic_operators),
        cmocka_unit_test(should_create_number_tokens_correctly),
        cmocka_unit_test(should_fail_if_float_is_malformed),
        cmocka_unit_test(should_scan_numbers),
        cmocka_unit_test(should_scan_reserved_words),
        cmocka_unit_test(should_scan_boolean_operators),
        cmocka_unit_test(should_create_string_tokens_correctly),
        cmocka_unit_test(should_fail_if_string_is_malformed),
        cmocka_unit_test(should_tokenize_if_correctly),
        cmocka_unit_test(should_tokenize_for_correctly),
        cmocka_unit_test(should_tokenize_while_correctly),
        cmocka_unit_test(should_tokenize_break_correctly)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
