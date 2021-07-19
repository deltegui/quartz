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
    case STMT_EXPR: {
        assert_expr_equals(first->expr.inner, second->expr.inner);
        break;
    }
    case STMT_VAR: {
        assert_true(token_equals(
            &first->var.identifier,
            &second->var.identifier));
        assert_expr_equals(first->var.definition, second->var.definition);
        break;
    }
    case STMT_LIST: {
        assert_list_stmt_equals(first->list, second->list);
        break;
    }
    case STMT_PRINT: {
        assert_expr_equals(first->print.inner, second->print.inner);
        break;
    }
    case STMT_BLOCK: {
        assert_stmt_equals(first->block.stmts, second->block.stmts);
        break;
    }
    case STMT_FUNCTION: {
        assert_true(token_equals(
            &first->function.identifier,
            &second->function.identifier));
        assert_stmt_equals(first->function.body, second->function.body);
        break;
    }
    case STMT_RETURN: {
        assert_expr_equals(first->return_.inner, second->return_.inner);
        break;
    }
    }
}

static void assert_list_stmt_equals(ListStmt* first, ListStmt* second) {
    assert_true(first->size == second->size);
    assert_true(first->capacity == second->capacity);
    for (int i = 0; i < first->size; i++) {
        assert_stmt_equals(first->stmts[i], second->stmts[i]);
    }
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
        assert_true(token_equals(&first->literal.literal, &second->literal.literal));
        break;
    }
    case EXPR_IDENTIFIER: {
        assert_true(token_equals(&first->identifier.name, &second->identifier.name));
        break;
    }
    case EXPR_ASSIGNMENT: {
        assert_expr_equals(first->assignment.value, second->assignment.value);
        assert_true(token_equals(&first->assignment.name, &second->assignment.name));
        break;
    }
    case EXPR_UNARY: {
        assert_true(first->unary.op.kind == second->unary.op.kind);
        break;
    }
    case EXPR_CALL: {
        assert_true(token_equals(&first->call.identifier, &second->call.identifier));
        assert_int_equal(first->call.params.size, second->call.params.size);
        for (int i = 0; i < first->call.params.size; i++) {
            assert_expr_equals(
                first->call.params.params[i].expr,
                second->call.params.params[i].expr);
        }
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
    ListStmt* list = create_stmt_list();
    stmt_list_add(list, expected);
    Stmt* stmt = CREATE_STMT_LIST(list);
    assert_ast(source, stmt);
    free_stmt(stmt);
}

static void assert_expr_ast(const char* source, Expr* expected) {
    ExprStmt stmt_expr = (ExprStmt){
        .inner = expected,
    };
    assert_stmt_ast(source, CREATE_STMT_EXPR(stmt_expr));
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
    .kind = TOKEN_TYPE_STRING
};

Token number_type_token = (Token){
    .length = 6,
    .line = 1,
    .start = "Number",
    .kind = TOKEN_TYPE_NUMBER
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

static void should_fail_parsing_malformed_expr() {
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
        "     false;  ",
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
        CREATE_STMT_VAR(var)
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

    ListStmt* list = create_stmt_list();
    stmt_list_add(list, CREATE_STMT_VAR(var));
    stmt_list_add(list, CREATE_STMT_EXPR(expr));

    Stmt* stmt = CREATE_STMT_LIST(list);
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

    ListStmt* list = create_stmt_list();
    stmt_list_add(list, CREATE_STMT_VAR(var));
    stmt_list_add(list, CREATE_STMT_EXPR(expr));

    Stmt* stmt = CREATE_STMT_LIST(list);
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

    ListStmt* list = create_stmt_list();
    stmt_list_add(list, CREATE_STMT_VAR(var));
    stmt_list_add(list, CREATE_STMT_EXPR(expr));
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(list),
    };

    ListStmt* global = create_stmt_list();
    stmt_list_add(global, CREATE_STMT_BLOCK(block));

    Stmt* stmt = CREATE_STMT_LIST(global);
    assert_ast(" { var a = 5; a = 2; } ", stmt);
    free_stmt(stmt);
}

static void should_parse_function_declarations() {
    FunctionStmt fn;
    Token fn_identifier = (Token){
        .length = 4,
        .line = 1,
        .start = "hola",
        .kind = TOKEN_IDENTIFIER,
    };
    fn.identifier = fn_identifier;
    BlockStmt fn_body = (BlockStmt){
        .stmts = CREATE_STMT_LIST(create_stmt_list()),
    };
    fn.body = CREATE_STMT_BLOCK(fn_body);
    assert_stmt_ast(" fn hola (a: Number, b: String) {} ", CREATE_STMT_FUNCTION(fn));
}

static void should_parse_returns() {
    FunctionStmt fn;
    Token fn_identifier = (Token){
        .length = 4,
        .line = 1,
        .start = "hola",
        .kind = TOKEN_IDENTIFIER,
    };
    fn.identifier = fn_identifier;
    ListStmt* body = create_stmt_list();
    ReturnStmt return_ = (ReturnStmt){
        .inner = CREATE_LITERAL_EXPR(true_),
    };
    stmt_list_add(body, CREATE_STMT_RETURN(return_));
    BlockStmt fn_body = (BlockStmt){
        .stmts = CREATE_STMT_LIST(body),
    };
    fn.body = CREATE_STMT_BLOCK(fn_body);
    assert_stmt_ast(
        " fn hola (a: Number, b: String): Bool { return true; } ",
        CREATE_STMT_FUNCTION(fn));
}

static void should_parse_empty_blocks() {
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(create_stmt_list()),
    };
    Stmt* stmt = CREATE_STMT_BLOCK(block);
    assert_stmt_ast("{   } ", stmt);
}

static void should_fail_if_return_is_not_inside_a_function() {
    assert_has_errors("return 1;");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_parse_returns),
        cmocka_unit_test(should_parse_empty_blocks),
        cmocka_unit_test(should_parse_function_declarations),
        cmocka_unit_test(should_parse_blocks),
        cmocka_unit_test(should_assign_vars),
        cmocka_unit_test(should_use_of_globals),
        cmocka_unit_test(should_parse_global_variables),
        cmocka_unit_test(should_parse_additions),
        cmocka_unit_test(should_parse_precedence),
        cmocka_unit_test(should_parse_grouping),
        cmocka_unit_test(should_fail_parsing_malformed_expr),
        cmocka_unit_test(should_parse_strings),
        cmocka_unit_test(should_parse_reserved_words_as_literals),
        cmocka_unit_test(should_parse_equality),
        cmocka_unit_test(should_fail_if_return_is_not_inside_a_function)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
