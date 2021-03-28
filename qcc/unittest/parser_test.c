#include <stdarg.h>
#include "./common.h"

#include "../parser.h"
#include "../expr.h"
#include "../debug.h"

static inline void assert_has_errors(const char* source);
static void assert_stmt_equals(Stmt* first, Stmt* second);
static void assert_list_stmt_equals(ListStmt* first, ListStmt* second);
static void assert_expr_equals(Expr* first, Expr* second);
static void compare_asts(Stmt* first, Stmt* second);
static void assert_ast(const char* source, Stmt* expected_ast);
static void assert_expr_ast(const char* source, Expr* expected);

static inline void assert_has_errors(const char* source) {
    Parser parser;
    init_parser(&parser, source);
    parse(&parser);
    assert_true(parser.has_error);
}

static void assert_stmt_equals(Stmt* first, Stmt* second) {
    assert_true(first->type == second->type);
    switch (first->type) {
    case EXPR_STMT: {
        assert_expr_equals(first->expr.inner, second->expr.inner);
        break;
    }
    case VAR_STMT: {
        // @todo implement
        fail();
        break;
    }
    case LIST_STMT: {
        assert_list_stmt_equals(first->list, second->list);
        break;
    }
    case PRINT_STMT: {
        assert_expr_equals(first->print.inner, second->print.inner);
        break;
    }
    }
}

static void assert_list_stmt_equals(ListStmt* first, ListStmt* second) {
    assert_true(first->length == second->length);
    assert_true(first->capacity == second->capacity);
    for (int i = 0; i < first->length; i++) {
        assert_stmt_equals(first->stmts[i], second->stmts[i]);
    }
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

static void compare_asts(Stmt* first, Stmt* second) {
    assert_stmt_equals(first, second);
}

static void assert_ast(const char* source, Stmt* expected_ast) {
    Parser parser;
    init_parser(&parser, source);
    Stmt* result = parse(&parser);
    compare_asts(result, expected_ast);
}

static void assert_expr_ast(const char* source, Expr* expected) {
    ExprStmt expr_stmt = (ExprStmt){
        .inner = expected,
    };
    ListStmt* list = create_list_stmt();
    list_stmt_add(list, CREATE_EXPR_STMT(expr_stmt));
    assert_ast(source, CREATE_LIST_STMT(list));
    free(list);
}

LiteralExpr true_ = (LiteralExpr){
    .literal = (Token){
        .length = 4,
        .line = 1,
        .start = "true",
        .type = TOKEN_TRUE
    },
};

LiteralExpr false_ = (LiteralExpr){
    .literal = (Token){
        .length = 5,
        .line = 1,
        .start = "false",
        .type = TOKEN_FALSE
    },
};

LiteralExpr nil = (LiteralExpr){
    .literal = (Token){
        .length = 3,
        .line = 1,
        .start = "nil",
        .type = TOKEN_NIL
    },
};

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

Token bang_equal = (Token){
    .length = 2,
    .line = 1,
    .start = "!=",
    .type = TOKEN_BANG_EQUAL
};

Token equal_equal = (Token){
    .length = 2,
    .line = 1,
    .start = "==",
    .type = TOKEN_EQUAL_EQUAL
};

Token example_str = (Token){
    .length = 12,
    .line = 1,
    .start = "Hello world!",
    .type = TOKEN_STRING,
};

static void should_parse_additions() {
    BinaryExpr sum = (BinaryExpr){
        .left = CREATE_LITERAL_EXPR(two),
        .op = sum_token,
        .right = CREATE_LITERAL_EXPR(two)
    };
    assert_expr_ast(
        "2+2;",
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
    assert_expr_ast(
        "   2-  2 / 5 ; ",
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
    assert_expr_ast(
        " ( 2 + 2 ) * 5 ",
        CREATE_BINARY_EXPR(mul)
    );
}

static void should_fail() {
    assert_has_errors(" ) 2 + 2 (; ");
    assert_has_errors(" 2 *; ");
    assert_has_errors(" 2 +; ");
    assert_has_errors(" 2 /; ");
    assert_has_errors(" 2 -; ");
    assert_has_errors(" 2 ** 3; ");
    assert_has_errors(" 2  ");
}

static void should_parse_strings() {
    assert_expr_ast(
        "   'Hello world!';   ",
        CREATE_LITERAL_EXPR(example_str)
    );
}

static void should_parse_equality() {
    BinaryExpr equality = (BinaryExpr){
        .left = CREATE_LITERAL_EXPR(two),
        .op = equal_equal,
        .right = CREATE_LITERAL_EXPR(two)
    };
    BinaryExpr not_equal = (BinaryExpr){
        .left = CREATE_BINARY_EXPR(equality),
        .op = bang_equal,
        .right = CREATE_LITERAL_EXPR(five)
    };
    assert_expr_ast(
        " 2 == 2 != 5; ",
        CREATE_BINARY_EXPR(not_equal)
    );
}

static void should_parse_reserved_words_as_literals() {
    assert_expr_ast(
        " true;   ",
        CREATE_LITERAL_EXPR(true_)
    );
    assert_expr_ast(
        " \n    false;  ",
        CREATE_LITERAL_EXPR(false_)
    );
    assert_expr_ast(
        "     nil;        ",
        CREATE_LITERAL_EXPR(nil)
    );
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_parse_additions),
        cmocka_unit_test(should_parse_precedence),
        cmocka_unit_test(should_parse_grouping),
        cmocka_unit_test(should_fail),
        cmocka_unit_test(should_parse_strings),
        cmocka_unit_test(should_parse_reserved_words_as_literals),
        cmocka_unit_test(should_parse_equality)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
