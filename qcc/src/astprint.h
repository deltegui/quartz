#ifndef QUARTZ_ASTPRINTER_H
#define QUARTZ_ASTPRINTER_H

#include <stdio.h>

#include "token.h"
#include "expr.h"

namespace Quartz {

class AstPrinter: public ExprVisitor {
    int offset = 0;

    void print(const char* msg, ...) {
        for (int i = 0; i < this->offset; i++) {
            printf("\t");
        }
        printf("%s\n", msg);
    }

    public:
    void visit_binary(BinaryExpr* binary) override {
        this->print("Left: [");
        this->offset++;
        binary->left->accept(this);
        this->print("Operator: ");
        switch(binary->op) {
        case Operator::Sum: this->print("+"); break;
        case Operator::Sub: this->print("-"); break;
        case Operator::Mul: this->print("*"); break;
        case Operator::Div: this->print("/"); break;
        default: this->print("[Unknown]");
        }
        this->print("Right: ");
        binary->right->accept(this);
        this->print("]");
        this->offset--;
    }

    void visit_literal(LiteralExpr* literal) override {
        this->print("Literal: [");
        this->offset++;
        print_token(literal->literal);
        this->print("]");
        this->offset--;
    }

    void visit_grouping(GroupingExpr* grouping) override {
        this->print("Grouping: [");
        grouping->inner->accept(this);
        this->print("]");
        this->offset--;
    }
};

}

#endif