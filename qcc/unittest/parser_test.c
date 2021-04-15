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
static void assert_stmt_ast(const char* source, Stmt* expected);
static void assert_expr_ast(const char* source, Expr* expected);
static void should_parse_global_variables();

static inline void assert_has_errors(const char* source) {
    Parser parser;
    ScopedSymbolTable symbols;
    init_scoped_symbol_table(&symbols);
    init_parser(&parser, source, &symbols);
    Stmt* result = parse(&parser);
    free_scoped_symbol_table(&symbols);
    assert_true(parser.has_error);
    free_stmt(result);
}

static void assert_stmt_equals(Stmt* first, Stmt* second) {
    assert_true(first->kind == second->kind);
    switch (first->kind) {
    case EXPR_STMT: {
        assert_expr_equals(first->expr.inner, second->expr.inner);
        break;
    }
    case VAR_STMT: {
        assert_true(first->var.identifier.kind == second->var.identifier.kind);
        assert_expr_equals(first->var.definition, second->var.definition);
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
    case BLOCK_STMT: {
        assert_stmt_equals(first->block.stmts, second->block.stmts);
        break;
    }
    case FUNCTION_STMT: {
        // TODO equal function name
        assert_stmt_equals(first->function.body, second->function.body);
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

static void assert_token_str_equal(Token first, Token second) {
    char actual[first.length + 1];
    sprintf(actual, "%.*s", first.length, first.start);
    assert_string_equal(actual, second.start);
}

static void assert_expr_equals(Expr* first, Expr* second) {
    assert_true(first->kind == second->kind);
    switch (first->kind) {
    case EXPR_BINARY: {
        assert_expr_equals(first->binary.left, second->binary.left);
        assert_true(first->binary.op.kind == second->binary.op.kind);
        assert_expr_equals(first->binary.right, second->binary.right);
        break;
    }
    case EXPR_LITERAL: {
        assert_true(first->literal.literal.kind == second->literal.literal.kind);
        assert_token_str_equal(first->literal.literal, second->literal.literal);
        break;
    }
    case EXPR_IDENTIFIER: {
        assert_token_str_equal(first->identifier.name, second->identifier.name);
        break;
    }
    case EXPR_ASSIGNMENT: {
        assert_expr_equals(first->assignment.value, second->assignment.value);
        assert_token_str_equal(first->assignment.name, second->assignment.name);
        break;
    }
    case EXPR_UNARY: {
        assert_true(first->unary.op.kind == second->unary.op.kind);
        break;
    }
    }
}

static void compare_asts(Stmt* first, Stmt* second) {
    assert_stmt_equals(first, second);
}

static void assert_ast(const char* source, Stmt* expected_ast) {
    Parser parser;
    ScopedSymbolTable symbols;
    init_scoped_symbol_table(&symbols);
    init_parser(&parser, source, &symbols);
    Stmt* result = parse(&parser);
    compare_asts(result, expected_ast);
    free_scoped_symbol_table(&symbols);
    free_stmt(result);
}

static void assert_stmt_ast(const char* source, Stmt* expected) {
    ListStmt* list = create_list_stmt();
    list_stmt_add(list, expected);
    Stmt* stmt = CREATE_LIST_STMT(list);
    assert_ast(source, stmt);
    free_stmt(stmt);
}

static void assert_expr_ast(const char* source, Expr* expected) {
    ExprStmt expr_stmt = (ExprStmt){
        .inner = expected,
    };
    assert_stmt_ast(source, CREATE_EXPR_STMT(expr_stmt));
}

LiteralExpr true_ = (LiteralExpr){
    .literal = (Token){
        .length = 4,
        .line = 1,
        .start = "true",
        .kind = TOKEN_TRUE
    },
};

LiteralExpr false_ = (LiteralExpr){
    .literal = (Token){
        .length = 5,
        .line = 1,
        .start = "false",
        .kind = TOKEN_FALSE
    },
};

LiteralExpr nil = (LiteralExpr){
    .literal = (Token){
        .length = 3,
        .line = 1,
        .start = "nil",
        .kind = TOKEN_NIL
    },
};

LiteralExpr two = (LiteralExpr){
    .literal = (Token){
        .length = 1,
        .line = 1,
        .start = "2",
        .kind = TOKEN_NUMBER
    },
};

LiteralExpr five = (LiteralExpr){
    .literal = (Token){
        .length = 1,
        .line = 1,
        .start = "5",
        .kind = TOKEN_NUMBER
    },
};

Token sub_token = (Token){
    .length = 1,
    .line = 1,
    .start = "-",
    .kind = TOKEN_MINUS
};

Token sum_token = (Token){
    .length = 1,
    .line = 1,
    .start = "+",
    .kind = TOKEN_PLUS
};

Token div_token = (Token){
    .length = 1,
    .line = 1,
    .start = "/",
    .kind = TOKEN_SLASH
};

Token star_token = (Token){
    .length = 1,
    .line = 1,
    .start = "*",
    .kind = TOKEN_STAR
};

Token bang_equal = (Token){
    .length = 2,
    .line = 1,
    .start = "!=",
    .kind = TOKEN_BANG_EQUAL
};

Token equal_equal = (Token){
    .length = 2,
    .line = 1,
    .start = "==",
    .kind = TOKEN_EQUAL_EQUAL
};

Token a_token = (Token){
    .length = 1,
    .line = 1,
    .start = "a",
    .kind = TOKEN_IDENTIFIER
};

Token b_token = (Token){
    .length = 1,
    .line = 1,
    .start = "b",
    .kind = TOKEN_IDENTIFIER
};

Token string_type_token = (Token){
    .length = 6,
    .line = 1,
    .start = "String",
    .kind = TOKEN_STRING_TYPE
};

Token number_type_token = (Token){
    .length = 6,
    .line = 1,
    .start = "Number",
    .kind = TOKEN_NUMBER_TYPE
};

Token example_str = (Token){
    .length = 12,
    .line = 1,
    .start = "Hello world!",
    .kind = TOKEN_STRING,
};

Token fn_token = (Token){
     .length = 2,
     .line = 1,
     .start = "fn",
     .kind = TOKEN_FUNCTION,
};

IdentifierExpr a_identifier = (IdentifierExpr){
    .name = (Token){
        .length = 1,
        .line = 1,
        .start = "a",
        .kind = TOKEN_IDENTIFIER
    },
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

static void should_parse_global_variables() {
    VarStmt var = (VarStmt){
        .identifier = a_token,
        .definition = CREATE_LITERAL_EXPR(two)
    };
    assert_stmt_ast(
        " var a = 2; ",
        CREATE_VAR_STMT(var)
    );
}

static void should_use_of_globals() {
    VarStmt var = (VarStmt){
        .identifier = a_token,
        .definition = CREATE_LITERAL_EXPR(five)
    };
    ExprStmt expr = (ExprStmt){
        .inner = CREATE_INDENTIFIER_EXPR(a_identifier)
    };

    ListStmt* list = create_list_stmt();
    list_stmt_add(list, CREATE_VAR_STMT(var));
    list_stmt_add(list, CREATE_EXPR_STMT(expr));

    Stmt* stmt = CREATE_LIST_STMT(list);
    assert_ast(" var a = 5; a ; ", stmt);
    free_stmt(stmt);
}

static void should_assign_vars() {
    VarStmt var = (VarStmt){
        .identifier = a_token,
        .definition = CREATE_LITERAL_EXPR(five)
    };
    AssignmentExpr assigment = (AssignmentExpr){
        .name = a_token,
        .value = CREATE_LITERAL_EXPR(two),
    };
    ExprStmt expr = (ExprStmt){
        .inner = CREATE_ASSIGNMENT_EXPR(assigment),
    };

    ListStmt* list = create_list_stmt();
    list_stmt_add(list, CREATE_VAR_STMT(var));
    list_stmt_add(list, CREATE_EXPR_STMT(expr));

    Stmt* stmt = CREATE_LIST_STMT(list);
    assert_ast(" var a = 5; a = 2; ", stmt);
    free_stmt(stmt);
}

static void should_parse_blocks() {
    VarStmt var = (VarStmt){
        .identifier = a_token,
        .definition = CREATE_LITERAL_EXPR(five)
    };
    AssignmentExpr assigment = (AssignmentExpr){
        .name = a_token,
        .value = CREATE_LITERAL_EXPR(two),
    };
    ExprStmt expr = (ExprStmt){
        .inner = CREATE_ASSIGNMENT_EXPR(assigment),
    };

    ListStmt* list = create_list_stmt();
    list_stmt_add(list, CREATE_VAR_STMT(var));
    list_stmt_add(list, CREATE_EXPR_STMT(expr));
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_LIST_STMT(list),
    };

    ListStmt* global = create_list_stmt();
    list_stmt_add(global, CREATE_BLOCK_STMT(block));

    Stmt* stmt = CREATE_LIST_STMT(global);
    assert_ast(" { var a = 5; a = 2; } ", stmt);
    free_stmt(stmt);
}

static void should_parse_function_declarations() {
    FunctionStmt fn = create_function_stmt();
    Token fn_identifier = (Token){
        .length = 1,
        .line = 1,
        .start = "hola",
        .kind = TOKEN_FUNCTION,
    };
    fn.identifier = fn_identifier;
    PARAM_ARRAY_ADD_TOKEN(&fn.params, a_token);
    PARAM_ARRAY_ADD_TOKEN(&fn.params, b_token);
    BlockStmt fn_body = (BlockStmt){
        .stmts = CREATE_LIST_STMT(create_list_stmt()),
    };
    fn.body = CREATE_BLOCK_STMT(fn_body);
    assert_stmt_ast(" fn hola (a: Number, b: String) {} ", CREATE_FUNCTION_STMT(fn));
}

static void should_parse_empty_blocks() {
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_LIST_STMT(create_list_stmt()),
    };
    Stmt* stmt = CREATE_BLOCK_STMT(block);
    assert_stmt_ast("{   } ", stmt);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_parse_empty_blocks),
        cmocka_unit_test(should_parse_function_declarations),
        cmocka_unit_test(should_parse_blocks),
        cmocka_unit_test(should_assign_vars),
        cmocka_unit_test(should_use_of_globals),
        cmocka_unit_test(should_parse_global_variables),
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
