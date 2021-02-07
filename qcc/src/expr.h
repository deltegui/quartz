#ifndef QUARTZ_EXPR_H
#define QUARTZ_EXPR_H

#include "token.h"

namespace Quartz {

enum class Operator {
    Sum,
    Mul,
    Div,
    Sub,
};

struct BinaryExpr;
struct LiteralExpr;
struct GroupingExpr;

class ExprVisitor {
    public:
    virtual void visit_binary(BinaryExpr*) = 0;
    virtual void visit_literal(LiteralExpr*) = 0;
    virtual void visit_grouping(GroupingExpr*) = 0;
};

class Expr {
    public:
    virtual void accept(ExprVisitor* visitor) = 0;
};

struct BinaryExpr: public Expr {
    Expr* left;
    Operator op;
    Expr* right;

    ~BinaryExpr() {
        delete left;
        delete right;
    }

    void accept(ExprVisitor* visitor) override {
        visitor->visit_binary(this);
    }
};

struct LiteralExpr: public Expr {
    Token literal;
    void accept(ExprVisitor* visitor) override {
        visitor->visit_literal(this);
    }
};

struct GroupingExpr: public Expr {
    Expr* inner;

    ~GroupingExpr() {
        delete inner;
    }

    void accept(ExprVisitor* visitor) override {
        visitor->visit_grouping(this);
    }
};

}

#endif
