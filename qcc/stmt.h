#ifndef QUARTZ_STMT_H
#define QUARTZ_STMT_H

#include "expr.h"
#include "fnparams.h"

typedef enum {
    EXPR_STMT,
    VAR_STMT,
    FUNCTION_STMT,
    LIST_STMT,
    PRINT_STMT,
    BLOCK_STMT,
} StmtKind;

struct _Stmt;

typedef struct {
    Expr* inner;
} ExprStmt;

typedef struct {
    Token identifier;
    Expr* definition;
} VarStmt;

typedef struct {
    Token identifier;
    ParamArray params;
    struct _Stmt* body;
} FunctionStmt;

typedef struct {
    Expr* inner;
} PrintStmt;

typedef struct {
    int capacity;
    int length;
    struct _Stmt** stmts;
} ListStmt;

typedef struct {
    struct _Stmt* stmts;
} BlockStmt;

ListStmt* create_list_stmt();
void list_stmt_add(ListStmt* list, struct _Stmt* stmt);

typedef struct _Stmt {
    StmtKind kind;
    union {
        ExprStmt expr;
        VarStmt var;
        FunctionStmt function;
        ListStmt* list;
        PrintStmt print;
        BlockStmt block;
    };
} Stmt;

typedef struct {
    void (*visit_expr)(void* ctx, ExprStmt* expr);
    void (*visit_var)(void* ctx, VarStmt* var);
    void (*visit_function)(void* ctx, FunctionStmt* function);
    void (*visit_print)(void* ctx, PrintStmt* print);
    void (*visit_block)(void* ctx, BlockStmt* block);
} StmtVisitor;

#define IS_VAR(stmt) (stmt.kind == VAR_STMT)
#define IS_FUNCTION(stmt) (stmt.kind == FUNCTION_STMT)
#define IS_EXPR(stmt) (stmt.kind == EXPR_STMT)
#define IS_LIST(stmt) (stmt.kind == LIST_STMT)
#define IS_PRINT(stmt) (stmt.kind == PRINT_STMT)
#define IS_BLOCK(stmt) (stmt.kind == BLOCK_STMT)

#define CREATE_VAR_STMT(var) create_stmt(VAR_STMT, &var)
#define CREATE_FUNCTION_STMT(fn) create_stmt(FUNCTION_STMT, &fn)
#define CREATE_EXPR_STMT(expr) create_stmt(EXPR_STMT, &expr)
// ListStmt is always a pointer (Because is created using create_list_stmt)
#define CREATE_LIST_STMT(list) create_stmt(LIST_STMT, list)
#define CREATE_PRINT_STMT(print) create_stmt(PRINT_STMT, &print)
#define CREATE_BLOCK_STMT(block) create_stmt(BLOCK_STMT, &block)

Stmt* create_stmt(StmtKind kind, void* stmt_node);
void free_stmt(Stmt* stmt);
void stmt_dispatch(StmtVisitor* visitor, void* ctx, Stmt* stmt);

#endif
