#ifndef QUARTZ_ASTPRINTER_H
#define QUARTZ_ASTPRINTER_H

#include <stdio.h>

#include "token.h"
#include "expr.h"

namespace Quartz {

class AstPrinter: public ExprVisitor {
    int offset = 0;

    void print_offset() {
        for (int i = 0; i < this->offset; i++) {
            printf("  ");
        }
    }

    void print(const char* msg, ...) {
        this->print_offset();
        printf("%s", msg);
    }

public:
    void visit_binary(BinaryExpr* binary) override {
        this->print("Bianary: [\n");
        this->offset++;

        this->print("Left:\n");
        this->offset++;
        binary->left->accept(this);
        this->offset--;

        this->print("Operator: ");
        switch (binary->op) {
        case Operator::Sum: printf("+\n"); break;
        case Operator::Sub: printf("-\n"); break;
        case Operator::Mul: printf("*\n"); break;
        case Operator::Div: printf("/\n"); break;
        default: printf("[Unknown]\n");
        }

        this->print("Right: \n");
        this->offset++;
        binary->right->accept(this);
        this->offset--;

        this->offset--;
        this->print("]\n");
    }

    void visit_literal(LiteralExpr* literal) override {
        this->print("Literal: [\n");
        this->offset++;
        this->print_offset();
        print_token(literal->literal);
        this->offset--;
        this->print("]\n");
    }

    void visit_grouping(GroupingExpr* grouping) override {
        this->print("Grouping: [\n");
        this->offset++;
        grouping->inner->accept(this);
        this->offset--;
        this->print("]\n");
    }
};

}

#endif