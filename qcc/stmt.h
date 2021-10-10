#ifndef QUARTZ_STMT_H_
#define QUARTZ_STMT_H_

#include "expr.h"

typedef enum {
    STMT_EXPR,
    STMT_VAR,
    STMT_FUNCTION,
    STMT_LIST,
    STMT_PRINT,
    STMT_BLOCK,
    STMT_RETURN,
    STMT_IF,
    STMT_FOR,
    STMT_WHILE,
} StmtKind;

struct _Stmt;

typedef struct {
    Expr* inner;
} ExprStmt;

typedef struct {
    Token identifier;
    Expr* definition; // Optional (can be NULL)
} VarStmt;

typedef struct {
    Token identifier;
    struct _Stmt* body;
} FunctionStmt;

typedef struct {
    Expr* inner;
} PrintStmt;

typedef struct {
    int capacity;
    int size;
    struct _Stmt** stmts;
} ListStmt;

typedef struct {
    struct _Stmt* stmts;
} BlockStmt;

typedef struct {
    Expr* inner;
} ReturnStmt;

typedef struct {
    Token token; // Just only to know where the if is.
    Expr* condition;
    struct _Stmt* then;
    struct _Stmt* else_; // Optional (can be NULL)
} IfStmt;

typedef struct {
    Token token; // Just only to know where the For is.
    struct _Stmt* init; // Optional (can be NULL)
    Expr* condition; // Optional (can be NULL)
    struct _Stmt* mod; // Optional (can be NULL)
    struct _Stmt* body;
} ForStmt;

typedef struct {
    Token token; // Just only to know where the For is.
    Expr* condition;
    struct _Stmt* body;
} WhileStmt;

ListStmt* create_stmt_list();
void stmt_list_add(ListStmt* const list, struct _Stmt* stmt);

typedef struct _Stmt {
    StmtKind kind;
    union {
        ExprStmt expr;
        VarStmt var;
        FunctionStmt function;
        ListStmt* list;
        PrintStmt print;
        BlockStmt block;
        ReturnStmt return_;
        IfStmt if_;
        ForStmt for_;
        WhileStmt while_;
    };
} Stmt;

typedef struct {
    void (*visit_expr)(void* ctx, ExprStmt* expr);
    void (*visit_var)(void* ctx, VarStmt* var);
    void (*visit_function)(void* ctx, FunctionStmt* function);
    void (*visit_print)(void* ctx, PrintStmt* print);
    void (*visit_block)(void* ctx, BlockStmt* block);
    void (*visit_return)(void* ctx, ReturnStmt* ret);
    void (*visit_if)(void* ctx, IfStmt* ifstmt);
    void (*visit_for)(void* ctx, ForStmt* forstmt);
    void (*visit_while)(void* ctx, WhileStmt* whilestmt);
} StmtVisitor;

#define STMT_IS_VAR(stmt) (stmt.kind == STMT_VAR)
#define STMT_IS_FUNCTION(stmt) (stmt.kind == STMT_FUNCTION)
#define STMT_IS_EXPR(stmt) (stmt.kind == STMT_EXPR)
#define STMT_IS_LIST(stmt) (stmt.kind == STMT_LIST)
#define STMT_IS_PRINT(stmt) (stmt.kind == STMT_PRINT)
#define STMT_IS_BLOCK(stmt) (stmt.kind == STMT_BLOCK)
#define STMT_IS_RETURN(stmt) (stmt.kind == STMT_RETURN)
#define STMT_IS_IF(stmt) (stmt.kind == STMT_IF)
#define STMT_IS_FOR(stmt) (stmt.kind == STMT_FOR)
#define STMT_IS_WHILE(stmt) (stmt.kind == STMT_WHILE)

#define CREATE_STMT_RETURN(return_) create_stmt(STMT_RETURN, &return_)
#define CREATE_STMT_VAR(var) create_stmt(STMT_VAR, &var)
#define CREATE_STMT_FUNCTION(fn) create_stmt(STMT_FUNCTION, &fn)
#define CREATE_STMT_EXPR(expr) create_stmt(STMT_EXPR, &expr)
// ListStmt is always a pointer (Because is created using create_list_stmt)
#define CREATE_STMT_LIST(list) create_stmt(STMT_LIST, list)
#define CREATE_STMT_PRINT(print) create_stmt(STMT_PRINT, &print)
#define CREATE_STMT_BLOCK(block) create_stmt(STMT_BLOCK, &block)
#define CREATE_STMT_IF(if_) create_stmt(STMT_IF, &if_)
#define CREATE_STMT_FOR(for_) create_stmt(STMT_FOR, &for_)
#define CREATE_STMT_WHILE(while_) create_stmt(STMT_WHILE, &while_)

Stmt* create_stmt(StmtKind kind, void* stmt_node);
void free_stmt(Stmt* const stmt);
void stmt_dispatch(StmtVisitor* visitor, void* ctx, Stmt* stmt);

#endif
