#ifndef QUARTZ_STMT_H
#define QUARTZ_STMT_H

#include "expr.h"

typedef enum {
    EXPR_STMT,
    VAR_STMT,
    LIST_STMT,
    PRINT_STMT,
} StmtType;

struct _Stmt;

typedef struct {
    Expr* inner;
} ExprStmt;

typedef struct {
    Token identifier;
    Expr* definition;
} VarStmt;

typedef struct {
    Expr* inner;
} PrintStmt;

typedef struct {
    int capacity;
    int length;
    struct _Stmt** stmts;
} ListStmt;

ListStmt* create_list_stmt();
void list_stmt_add(ListStmt* list, struct _Stmt* stmt);

typedef struct _Stmt {
    StmtType type;
    union {
        ExprStmt expr;
        VarStmt var;
        ListStmt* list;
        PrintStmt print;
    };
} Stmt;

typedef struct {
    void (*visit_expr)(void* ctx, ExprStmt* expr);
    void (*visit_var)(void* ctx, VarStmt* var);
    void (*visit_print)(void* ctx, PrintStmt* print);
} StmtVisitor;

#define IS_VAR(stmt) (stmt.type == VAR_STMT)
#define IS_EXPR(stmt) (stmt.type == EXPR_STMT)
#define IS_LIST(stmt) (stmt.type == LIST_STMT)
#define IS_PRINT(stmt) (stmt.type == PRINT_STMT)

#define CREATE_VAR_STMT(var) create_stmt(VAR_STMT, &var)
#define CREATE_EXPR_STMT(expr) create_stmt(EXPR_STMT, &expr)
// ListStmt is always a pointer (Because is created using create_list_stmt)
#define CREATE_LIST_STMT(list) create_stmt(LIST_STMT, list)
#define CREATE_PRINT_STMT(print) create_stmt(PRINT_STMT, &print)

Stmt* create_stmt(StmtType type, void* stmt_node);
void free_stmt(Stmt* stmt);
void stmt_dispatch(StmtVisitor* visitor, void* ctx, Stmt* stmt);

#endif
