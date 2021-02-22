#include "astprint.h"
#include "common.h"

static void print_offset();
static void pretty_print(const char *msg, ...);
static void print_binary(BinaryExpr* binary);
static void print_literal(LiteralExpr *literal);

ExprVisitor printer_visitor = (ExprVisitor){
    .visit_literal = print_literal,
    .visit_binary = print_binary,
};

#define ACCEPT(expr) expr_dispatch(&printer_visitor, expr)

int offset = 0;

void ast_print(Expr* root) {
    ACCEPT(root);
}

static void print_offset() {
    for (int i = 0; i < offset; i++) {
        printf("  ");
    }
}

static void pretty_print(const char *msg, ...) {
    print_offset();
    printf("%s", msg);
}

static void print_binary(BinaryExpr* binary) {
    pretty_print("Bianary: [\n");
    offset++;

    pretty_print("Left:\n");
    offset++;
    ACCEPT(binary->left);
    offset--;

    pretty_print("Operator: ");
    print_offset();
    print_token(binary->op);

    pretty_print("Right: \n");
    offset++;
    ACCEPT(binary->right);
    offset--;

    offset--;
    pretty_print("]\n");
}

static void print_literal(LiteralExpr *literal) {
    pretty_print("Literal: [\n");
    offset++;
    print_offset();
    print_token(literal->literal);
    offset--;
    pretty_print("]\n");
}
