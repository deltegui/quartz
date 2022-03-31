#include <stdarg.h>
#include "./common.h"

#include "../parser.h"
#include "../expr.h"
#include "../debug.h"
#include "../array.h"
#include "../string.h"

static inline void assert_has_errors(const char* source);
static void assert_stmt_equals(Stmt* first, Stmt* second);
static void assert_list_stmt_equals(ListStmt* first, ListStmt* second);
static void assert_expr_equals(Expr* first, Expr* second);
static void compare_asts(Stmt* first, Stmt* second);
static void assert_ast(const char* source, Stmt* expected_ast);
static void assert_stmt_ast(const char* source, Stmt* expected);
static void assert_expr_ast(const char* source, Expr* expected);
static void should_parse_global_variables();
static void assert_params_equal(Vector* first, Vector* second);

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
        assert_true(t_token_equals(
            &first->var.identifier,
            &second->var.identifier));
        if (! first->var.definition) {
            assert_true(first->var.definition == second->var.definition);
        } else {
            assert_expr_equals(first->var.definition, second->var.definition);
        }
        break;
    }
    case STMT_LIST: {
        assert_list_stmt_equals(first->list, second->list);
        break;
    }
    case STMT_BLOCK: {
        assert_stmt_equals(first->block.stmts, second->block.stmts);
        break;
    }
    case STMT_FUNCTION: {
        assert_true(t_token_equals(
            &first->function.identifier,
            &second->function.identifier));
        assert_stmt_equals(first->function.body, second->function.body);
        break;
    }
    case STMT_RETURN: {
        assert_expr_equals(first->return_.inner, second->return_.inner);
        break;
    }
    case STMT_IF: {
        assert_expr_equals(first->if_.condition, second->if_.condition);
        assert_stmt_equals(first->if_.then, second->if_.then);
        if (first->if_.else_ != NULL) {
            assert_stmt_equals(first->if_.else_, second->if_.else_);
        }
        break;
    }
    case STMT_FOR: {
        if (first->for_.condition != NULL) {
            assert_expr_equals(first->for_.condition, second->for_.condition);
        }
        if (first->for_.init != NULL) {
            assert_stmt_equals(first->for_.init, second->for_.init);
        }
        if (first->for_.mod != NULL) {
            assert_stmt_equals(first->for_.mod, second->for_.mod);
        }
        assert_stmt_equals(first->for_.body, second->for_.body);
        break;
    }
    case STMT_WHILE: {
        assert_expr_equals(first->while_.condition, second->while_.condition);
        assert_stmt_equals(first->while_.body, second->while_.body);
        break;
    }
    case STMT_LOOPG: {
        break;
    }
    case STMT_TYPEALIAS: {
        assert_true(
            t_token_equals(
                &first->typealias.identifier,
                &second->typealias.identifier));
        break;
    }
    case STMT_IMPORT: {
        assert_true(
            t_token_equals(
                &first->import.filename,
                &second->import.filename));
        assert_stmt_equals(first->import.ast, second->import.ast);
        break;
    }
    case STMT_NATIVE: {
        assert_true(first->native.length == second->native.length);
        assert_true(
            memcmp(
                first->native.name,
                second->native.name,
                first->native.length) == 0);
        break;
    }
    case STMT_CLASS: {
        assert_true(
            t_token_equals(
                &first->klass.identifier,
                &second->klass.identifier));
        assert_stmt_equals(first->klass.body, second->klass.body);
        break;
    }
    case STMT_NATIVE_CLASS:
        break;
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
        assert_true(t_token_equals(&first->literal.literal, &second->literal.literal));
        break;
    }
    case EXPR_IDENTIFIER: {
        assert_true(t_token_equals(&first->identifier.name, &second->identifier.name));
        break;
    }
    case EXPR_ASSIGNMENT: {
        assert_expr_equals(first->assignment.value, second->assignment.value);
        assert_true(t_token_equals(&first->assignment.name, &second->assignment.name));
        break;
    }
    case EXPR_UNARY: {
        assert_true(first->unary.op.kind == second->unary.op.kind);
        break;
    }
    case EXPR_CALL: {
        assert_expr_equals(first->call.callee, second->call.callee);
        assert_params_equal(&first->call.params, &second->call.params);
        break;
    }
    case EXPR_NEW: {
        assert_true(t_token_equals(&first->new_.klass, &second->new_.klass));
        assert_params_equal(&first->new_.params, &second->new_.params);
        break;
    }
    case EXPR_PROP: {
        assert_expr_equals(first->prop.object, second->prop.object);
        assert_true(t_token_equals(&first->prop.prop, &second->prop.prop));
        break;
    }
    case EXPR_PROP_ASSIGMENT: {
        assert_expr_equals(first->prop_assigment.object, second->prop_assigment.object);
        assert_true(t_token_equals(&first->prop_assigment.prop, &second->prop_assigment.prop));
        assert_expr_equals(first->prop_assigment.value, second->prop_assigment.value);
        break;
    }
    case EXPR_ARRAY: {
        // TODO implement
        break;
    }
    case EXPR_CAST: {
        assert_expr_equals(first->cast.inner, second->cast.inner);
        assert_true(t_token_equals(&first->cast.token, &second->cast.token));
        assert_true(type_equals(first->cast.type, second->cast.type));
        break;
    }
    }
}

static void assert_params_equal(Vector* first, Vector* second) {
    assert_int_equal(first->size, second->size);
    Expr** first_expr = VECTOR_AS_EXPRS(first);
    Expr** second_expr = VECTOR_AS_EXPRS(second);
    for (int i = 0; i < first->size; i++) {
        assert_expr_equals(first_expr[i], second_expr[i]);
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

#define LIST_ADD_NATIVE_CLASS(list, name_class, length_class) do {\
    NativeClassStmt native;\
    native.name = name_class;\
    native.length = length_class;\
    stmt_list_add(list, CREATE_STMT_NATIVE_CLASS(native));\
} while (false)

#define LIST_ADD_PRE_NATIVE(list)\
    LIST_ADD_NATIVE_CLASS(list, ARRAY_CLASS_NAME, ARRAY_CLASS_LENGTH);\
    LIST_ADD_NATIVE_CLASS(list, STRING_CLASS_NAME, STRING_CLASS_LENGTH)

static void assert_stmt_ast(const char* source, Stmt* expected) {
    ListStmt* list = create_stmt_list();
    LIST_ADD_PRE_NATIVE(list);
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

Token hello_token = (Token){
    .length = 5,
    .line = 1,
    .start = "Hello",
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
        .kind = TOKEN_IDENTIFIER,
    },
};

LoopGotoStmt break_ = (LoopGotoStmt){
    .token = (Token){
        .length = 5,
        .line = 1,
        .start = "break",
        .kind = TOKEN_BREAK,
    },
    .kind = LOOP_BREAK,
};

LoopGotoStmt continue_ = (LoopGotoStmt){
    .token = (Token){
        .length = 5,
        .line = 1,
        .start = "continue",
        .kind = TOKEN_CONTINUE,
    },
    .kind = LOOP_CONTINUE,
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
        " ( 2 + 2 ) * 5; ",
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
    LIST_ADD_PRE_NATIVE(list);
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
    LIST_ADD_PRE_NATIVE(list);
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
    LIST_ADD_PRE_NATIVE(global);
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

static void should_fail_if_you_use_reserved_words_as_identifiers() {
    assert_has_errors(" var var = 1; ");
    assert_has_errors(" var fn = 5; ");
    assert_has_errors(" fn {} () {} ");
}

static void should_parse_single_if() {
    IfStmt if_;
    if_.condition = CREATE_LITERAL_EXPR(true_);
    ExprStmt e = (ExprStmt){
        .inner = CREATE_LITERAL_EXPR(two),
    };
    if_.then = CREATE_STMT_EXPR(e);
    if_.else_ = NULL;
    assert_stmt_ast("if (true) 2;", CREATE_STMT_IF(if_));
}

static void should_parse_if_with_block() {
    IfStmt if_;
    if_.condition = CREATE_LITERAL_EXPR(true_);
    ListStmt* list = create_stmt_list();
    ExprStmt p = (ExprStmt){
        .inner = CREATE_LITERAL_EXPR(two),
    };
    stmt_list_add(list, CREATE_STMT_EXPR(p));
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(list),
    };
    if_.then = CREATE_STMT_BLOCK(block);
    if_.else_ = NULL;
    assert_stmt_ast("if (true) { 2; }", CREATE_STMT_IF(if_));
}

static void should_parse_if_else() {
    IfStmt if_;
    if_.condition = CREATE_LITERAL_EXPR(true_);
    ExprStmt p_two = (ExprStmt){
        .inner = CREATE_LITERAL_EXPR(two),
    };
    if_.then = CREATE_STMT_EXPR(p_two);
    ExprStmt p_five = (ExprStmt){
        .inner = CREATE_LITERAL_EXPR(five),
    };
    if_.else_ = CREATE_STMT_EXPR(p_five);
    assert_stmt_ast("if (true) 2; else 5;", CREATE_STMT_IF(if_));
}

static void should_parse_if_elif_else() {
    IfStmt if_;
    if_.condition = CREATE_LITERAL_EXPR(true_);
    ExprStmt p_two = (ExprStmt){
        .inner = CREATE_LITERAL_EXPR(two),
    };
    if_.then = CREATE_STMT_EXPR(p_two);

    IfStmt inner_if;
    inner_if.condition = CREATE_LITERAL_EXPR(false_);
    ExprStmt p_five = (ExprStmt){
        .inner = CREATE_LITERAL_EXPR(five),
    };
    inner_if.then = CREATE_STMT_EXPR(p_five);
    ExprStmt return_ = (ExprStmt){
        .inner = CREATE_LITERAL_EXPR(five),
    };
    inner_if.else_ = CREATE_STMT_EXPR(return_);

    if_.else_ = CREATE_STMT_IF(inner_if);
    assert_stmt_ast("if (true) 2; else if (false) 5; else 5;", CREATE_STMT_IF(if_));
}

static void should_parse_for_with_condition() {
    ForStmt for_;
    for_.condition = CREATE_LITERAL_EXPR(false_);
    for_.init = NULL;
    for_.mod = NULL;
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(create_stmt_list()),
    };
    for_.body = CREATE_STMT_BLOCK(block);
    assert_stmt_ast("for (;false;) {}", CREATE_STMT_FOR(for_));
}

static void should_parse_for_infinite() {
    ForStmt for_;
    for_.condition = NULL;
    for_.init = NULL;
    for_.mod = NULL;
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(create_stmt_list()),
    };
    for_.body = CREATE_STMT_BLOCK(block);
    assert_stmt_ast("for (;;) {}", CREATE_STMT_FOR(for_));
}

static void should_parse_for_one_init_and_condition() {
    VarStmt var = (VarStmt){
        .identifier = a_token,
        .definition = CREATE_LITERAL_EXPR(five)
    };
    ListStmt* init_list = create_stmt_list();
    stmt_list_add(init_list, CREATE_STMT_VAR(var));

    ForStmt for_;
    for_.condition = CREATE_LITERAL_EXPR(false_);
    for_.init = CREATE_STMT_LIST(init_list);
    for_.mod = NULL;
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(create_stmt_list()),
    };
    for_.body = CREATE_STMT_BLOCK(block);
    assert_stmt_ast("for (var a = 5;false;) {}", CREATE_STMT_FOR(for_));
}

static void should_parse_for_two_init_and_condition() {
    VarStmt var_a = (VarStmt){
        .identifier = a_token,
        .definition = CREATE_LITERAL_EXPR(five)
    };

    VarStmt var_b = (VarStmt){
        .identifier = b_token,
        .definition = CREATE_LITERAL_EXPR(two)
    };

    ListStmt* init_list = create_stmt_list();
    stmt_list_add(init_list, CREATE_STMT_VAR(var_a));
    stmt_list_add(init_list, CREATE_STMT_VAR(var_b));

    ForStmt for_;
    for_.condition = CREATE_LITERAL_EXPR(false_);
    for_.init = CREATE_STMT_LIST(init_list);
    for_.mod = NULL;
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(create_stmt_list()),
    };
    for_.body = CREATE_STMT_BLOCK(block);
    assert_stmt_ast("for (var a = 5, var b = 2;false;) {}", CREATE_STMT_FOR(for_));
}

static void should_parse_for_init_condition_mod() {
    VarStmt var = (VarStmt){
        .identifier = a_token,
        .definition = CREATE_LITERAL_EXPR(five)
    };
    ListStmt* init_list = create_stmt_list();
    stmt_list_add(init_list, CREATE_STMT_VAR(var));

    AssignmentExpr assigment = (AssignmentExpr){
        .name = a_token,
        .value = CREATE_LITERAL_EXPR(two),
    };
    ExprStmt stmt = (ExprStmt){
        .inner = CREATE_ASSIGNMENT_EXPR(assigment),
    };
    ListStmt* mod_list = create_stmt_list();
    stmt_list_add(mod_list, CREATE_STMT_EXPR(stmt));

    ForStmt for_;
    for_.condition = CREATE_LITERAL_EXPR(false_);
    for_.init = CREATE_STMT_LIST(init_list);
    for_.mod = CREATE_STMT_LIST(mod_list);
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(create_stmt_list()),
    };
    for_.body = CREATE_STMT_BLOCK(block);

    assert_stmt_ast("for (var a = 5;false;a = 2) {}", CREATE_STMT_FOR(for_));
}

static void should_parse_for_two_mod() {
    VarStmt var_a = (VarStmt){
        .identifier = a_token,
        .definition = CREATE_LITERAL_EXPR(five)
    };
    VarStmt var_b = (VarStmt){
        .identifier = b_token,
        .definition = CREATE_LITERAL_EXPR(two)
    };
    ListStmt* init_list = create_stmt_list();
    stmt_list_add(init_list, CREATE_STMT_VAR(var_a));
    stmt_list_add(init_list, CREATE_STMT_VAR(var_b));

    AssignmentExpr a_assigment = (AssignmentExpr){
        .name = a_token,
        .value = CREATE_LITERAL_EXPR(two),
    };
    ExprStmt a_stmt = (ExprStmt){
        .inner = CREATE_ASSIGNMENT_EXPR(a_assigment),
    };
    AssignmentExpr b_assigment = (AssignmentExpr){
        .name = b_token,
        .value = CREATE_LITERAL_EXPR(five),
    };
    ExprStmt b_stmt = (ExprStmt){
        .inner = CREATE_ASSIGNMENT_EXPR(b_assigment),
    };
    ListStmt* mod_list = create_stmt_list();
    stmt_list_add(mod_list, CREATE_STMT_EXPR(a_stmt));
    stmt_list_add(mod_list, CREATE_STMT_EXPR(b_stmt));

    ForStmt for_;
    for_.condition = NULL;
    for_.init = CREATE_STMT_LIST(init_list);
    for_.mod = CREATE_STMT_LIST(mod_list);
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(create_stmt_list()),
    };
    for_.body = CREATE_STMT_BLOCK(block);

    assert_stmt_ast("for (var a = 5, var b = 2;;a = 2, b = 5) {}", CREATE_STMT_FOR(for_));
}

static void should_fail_if_for_is_malformed() {
    assert_has_errors(" for () {}");
    assert_has_errors(" for (;) {}");
    assert_has_errors(" for (,;;) {}");
    assert_has_errors(" for (;;,) {}");
    assert_has_errors(" for (;;) ");
}

static void should_parse_while_with_condition() {
    WhileStmt while_;
    while_.condition = CREATE_LITERAL_EXPR(false_);
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(create_stmt_list()),
    };
    while_.body = CREATE_STMT_BLOCK(block);
    assert_stmt_ast("  while (false) {} ", CREATE_STMT_WHILE(while_));
}

static void should_fail_parse_loopg_if_is_not_inside_a_loop() {
    assert_has_errors("break;");
    assert_has_errors("continue;");
}

static void should_parse_break() {
    WhileStmt while_;
    while_.condition = CREATE_LITERAL_EXPR(false_);
    ListStmt* list = create_stmt_list();
    stmt_list_add(list, CREATE_STMT_LOOPG(break_));
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(list),
    };
    while_.body = CREATE_STMT_BLOCK(block);
    assert_stmt_ast("  while (false) {break;} ", CREATE_STMT_WHILE(while_));
}

static void should_parse_continue() {
    WhileStmt while_;
    while_.condition = CREATE_LITERAL_EXPR(false_);
    ListStmt* list = create_stmt_list();
    stmt_list_add(list, CREATE_STMT_LOOPG(continue_));
    BlockStmt block = (BlockStmt){
        .stmts = CREATE_STMT_LIST(list),
    };
    while_.body = CREATE_STMT_BLOCK(block);
    assert_stmt_ast("  while (false) {continue;} ", CREATE_STMT_WHILE(while_));
}

static void should_parse_typealias() {
    TypealiasStmt typealias;
    typealias.identifier = b_token;
    assert_stmt_ast(" typedef b = Number;", CREATE_STMT_TYPEALIAS(typealias));
}

static void should_fail_typealias() {
    assert_has_errors("typedef Hola Number;");
    assert_has_errors("typedef Hola = Number");
}

static void should_parse_classes() {
    ClassStmt klass;
    klass.identifier = hello_token;

    ListStmt* list = create_stmt_list();

    VarStmt a_var;
    a_var.identifier = a_token;
    a_var.definition = NULL;
    stmt_list_add(list, CREATE_STMT_VAR(a_var));

    VarStmt b_var;
    b_var.identifier = b_token;
    b_var.definition = NULL;
    stmt_list_add(list, CREATE_STMT_VAR(b_var));

    klass.body = CREATE_STMT_LIST(list);

    assert_stmt_ast(" class Hello { pub var a; var b; } ", CREATE_STMT_CLASS(klass));
}

static void should_fail_parsing_classes() {
    assert_has_errors(" class Hello { pub }");
    assert_has_errors(" class Hello { for (;;) {} }");
}

static void should_parse_empty_class() {
    ClassStmt klass;
    klass.identifier = hello_token;

    ListStmt* list = create_stmt_list();
    klass.body = CREATE_STMT_LIST(list);

    assert_stmt_ast(" class Hello { } ", CREATE_STMT_CLASS(klass));
}

static void should_parse_new_expr() {
    ListStmt* main = create_stmt_list();
    LIST_ADD_PRE_NATIVE(main);

    ClassStmt klass;
    klass.identifier = hello_token;
    ListStmt* body = create_stmt_list();
    klass.body = CREATE_STMT_LIST(body);
    stmt_list_add(main, CREATE_STMT_CLASS(klass));

    NewExpr new_;
    new_.klass = hello_token;
    init_vector(&new_.params, sizeof(Expr*));
    ExprStmt expr;
    expr.inner = CREATE_NEW_EXPR(new_);
    stmt_list_add(main, CREATE_STMT_EXPR(expr));

    assert_ast(" class Hello {} new Hello();   ", CREATE_STMT_LIST(main));
}

static void should_fail_parsing_new() {
    assert_has_errors(" new Hello();");
    assert_has_errors(" class Hello {} new Hello);");
    assert_has_errors(" class Hello {} new Hello(;");
}

static int test_setup(void** args) {
    init_type_pool();
    return 0; }

static int test_teardown(void** args) {
    free_type_pool();
    return 0;
}

int main(void) {
    init_array();
    init_string();
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
        cmocka_unit_test(should_fail_if_you_use_reserved_words_as_identifiers),
        cmocka_unit_test(should_fail_if_return_is_not_inside_a_function),
        cmocka_unit_test(should_parse_single_if),
        cmocka_unit_test(should_parse_if_with_block),
        cmocka_unit_test(should_parse_if_else),
        cmocka_unit_test(should_parse_if_elif_else),
        cmocka_unit_test(should_parse_for_infinite),
        cmocka_unit_test(should_parse_for_with_condition),
        cmocka_unit_test(should_parse_for_one_init_and_condition),
        cmocka_unit_test(should_parse_for_two_init_and_condition),
        cmocka_unit_test(should_parse_for_init_condition_mod),
        cmocka_unit_test(should_parse_for_two_mod),
        cmocka_unit_test(should_fail_if_for_is_malformed),
        cmocka_unit_test(should_parse_while_with_condition),
        cmocka_unit_test(should_fail_parse_loopg_if_is_not_inside_a_loop),
        cmocka_unit_test(should_parse_break),
        cmocka_unit_test(should_parse_continue),
        cmocka_unit_test(should_parse_typealias),
        cmocka_unit_test(should_fail_typealias),
        cmocka_unit_test(should_parse_classes),
        cmocka_unit_test(should_fail_parsing_classes),
        cmocka_unit_test(should_parse_empty_class),
        cmocka_unit_test(should_parse_new_expr),
        cmocka_unit_test(should_fail_parsing_new)
    };
    return cmocka_run_group_tests(tests, test_setup, test_teardown);
}
