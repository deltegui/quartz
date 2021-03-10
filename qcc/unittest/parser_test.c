#include <stdarg.h>
#include "./common.h"

#include "../parser.h"
#include "../expr.h"

static inline void assert_has_errors(const char* source) {
    Parser parser;
    init_parser(&parser, source);
    parse(&parser);
    assert_true(parser.has_error);
}

static void assert_expr_equals(Expr* first, Expr* second) {
    assert_true(first->type == second->type);
    switch (first->type) {
    case EXPR_BINARY: {
        assert_expr_equals(first->binary.left, second->binary.left);
        assert_true(first->binary.op.type == second->binary.op.type);
        assert_expr_equals(first->binary.right, second->binary.right);
        break;
    }
    case EXPR_LITERAL: {
        assert_true(first->literal.literal.type == second->literal.literal.type);
        char actual[first->literal.literal.length + 1];
        sprintf(actual, "%.*s", first->literal.literal.length, first->literal.literal.start);
        assert_string_equal(actual, second->literal.literal.start);
        break;
    }
    case EXPR_UNARY: {
        assert_true(first->unary.op.type == second->unary.op.type);
        break;
    }
    }
}

static void compare_asts(Expr* first, Expr* second) {
    assert_expr_equals(first, second);
}

static void assert_ast(const char* source, Expr* expected_ast) {
    Parser parser;
    init_parser(&parser, source);
    Expr* result = parse(&parser);
    compare_asts(result, expected_ast);
}

LiteralExpr two = (LiteralExpr){
    .literal = (Token){
        .length = 1,
        .line = 1,
        .start = "2",
        .type = TOKEN_NUMBER
    },
};

LiteralExpr five = (LiteralExpr){
    .literal = (Token){
        .length = 1,
        .line = 1,
        .start = "5",
        .type = TOKEN_NUMBER
    },
};

Token sub_token = (Token){
    .length = 1,
    .line = 1,
    .start = "-",
    .type = TOKEN_MINUS
};

Token sum_token = (Token){
    .length = 1,
    .line = 1,
    .start = "+",
    .type = TOKEN_PLUS
};

Token div_token = (Token){
    .length = 1,
    .line = 1,
    .start = "/",
    .type = TOKEN_SLASH
};

Token star_token = (Token){
    .length = 1,
    .line = 1,
    .start = "*",
    .type = TOKEN_STAR
};

static void should_parse_additions() {
    BinaryExpr sum = (BinaryExpr){
        .left = CREATE_LITERAL_EXPR(two),
        .op = sum_token,
        .right = CREATE_LITERAL_EXPR(two)
    };
    assert_ast(
        "2+2",
        CREATE_BINARY_EXPR(sum)
    );
}

static void should_parse_precedence() {
    BinaryExpr division = (BinaryExpr){
        .left = CREATE_LITERAL_EXPR(two),
        .op = div_token,
        .right = CREATE_LITERAL_EXPR(five)
    };
    BinaryExpr substract = (BinaryExpr){
        .left = CREATE_LITERAL_EXPR(two),
        .op = sub_token,
        .right = CREATE_BINARY_EXPR(division)
    };
    assert_ast(
        "   2-  2 / 5  ",
        CREATE_BINARY_EXPR(substract)
    );
}

static void should_parse_grouping() {
    BinaryExpr sum = (BinaryExpr){
        .left = CREATE_LITERAL_EXPR(two),
        .op = sum_token,
        .right = CREATE_LITERAL_EXPR(two)
    };
    BinaryExpr mul = (BinaryExpr){
        .left = CREATE_BINARY_EXPR(sum),
        .op = star_token,
        .right = CREATE_LITERAL_EXPR(five)
    };
    assert_ast(
        " ( 2 + 2 ) * 5 ",
        CREATE_BINARY_EXPR(mul)
    );
}

static void should_fail() {
    assert_has_errors(" ) 2 + 2 ( ");
    assert_has_errors(" 2 * ");
    assert_has_errors(" 2 + ");
    assert_has_errors(" 2 / ");
    assert_has_errors(" 2 - ");
    assert_has_errors(" 2 ** 3 ");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_parse_additions),
        cmocka_unit_test(should_parse_precedence),
        cmocka_unit_test(should_parse_grouping),
        cmocka_unit_test(should_fail)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
