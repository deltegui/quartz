#ifndef QUARTZ_STMT_H_
#define QUARTZ_STMT_H_

#include "expr.h"
#include "native.h"

typedef enum {
    STMT_TYPEALIAS,
    STMT_EXPR,
    STMT_VAR,
    STMT_FUNCTION,
    STMT_LIST,
    STMT_BLOCK,
    STMT_RETURN,
    STMT_IF,
    STMT_FOR,
    STMT_WHILE,
    STMT_LOOPG,
    STMT_IMPORT,
    STMT_NATIVE,
    STMT_CLASS,
} StmtKind;

struct s_stmt;

typedef struct {
    Expr* inner;
} ExprStmt;

typedef struct {
    Token identifier;
    Expr* definition; // Optional (can be NULL)
} VarStmt;

typedef struct {
    Token identifier;
} TypealiasStmt;

typedef struct {
    Token identifier;
    struct s_stmt* body;
} FunctionStmt;

typedef struct {
    const char* name;
    int length;
    native_fn_t function;
} NativeFunctionStmt;

typedef struct {
    int capacity;
    int size;
    struct s_stmt** stmts;
} ListStmt;

typedef struct {
    struct s_stmt* stmts;
} BlockStmt;

typedef struct {
    Expr* inner;
} ReturnStmt;

typedef struct {
    Token token; // Just to know where the if is.
    Expr* condition;
    struct s_stmt* then;
    struct s_stmt* else_; // Optional (can be NULL)
} IfStmt;

typedef struct {
    Token token; // Just to know where the For is.
    struct s_stmt* init; // Optional (can be NULL)
    Expr* condition; // Optional (can be NULL)
    struct s_stmt* mod; // Optional (can be NULL)
    struct s_stmt* body;
} ForStmt;

typedef struct {
    Token token; // Just to know where the While is.
    Expr* condition;
    struct s_stmt* body;
} WhileStmt;

typedef enum {
    LOOP_BREAK,
    LOOP_CONTINUE,
} LoopGotoKind;

typedef struct {
    Token token; // Just to know where the loop goto is.
    LoopGotoKind kind;
} LoopGotoStmt;

typedef struct {
    Token filename;
    struct s_stmt* ast;
} ImportStmt;

typedef struct {
    Token identifier;
    struct s_stmt* body;
} ClassStmt;

ListStmt* create_stmt_list();
void stmt_list_add(ListStmt* const list, struct s_stmt* stmt);

typedef struct s_stmt {
    StmtKind kind;
    union {
        ExprStmt expr;
        VarStmt var;
        FunctionStmt function;
        ListStmt* list;
        BlockStmt block;
        ReturnStmt return_;
        IfStmt if_;
        ForStmt for_;
        WhileStmt while_;
        LoopGotoStmt loopg;
        TypealiasStmt typealias;
        ImportStmt import;
        NativeFunctionStmt native;
        ClassStmt klass;
    };
} Stmt;

typedef struct {
    void (*visit_expr)(void* ctx, ExprStmt* expr);
    void (*visit_var)(void* ctx, VarStmt* var);
    void (*visit_function)(void* ctx, FunctionStmt* function);
    void (*visit_block)(void* ctx, BlockStmt* block);
    void (*visit_return)(void* ctx, ReturnStmt* ret);
    void (*visit_if)(void* ctx, IfStmt* ifstmt);
    void (*visit_for)(void* ctx, ForStmt* forstmt);
    void (*visit_while)(void* ctx, WhileStmt* whilestmt);
    void (*visit_loopg)(void* ctx, LoopGotoStmt* loopg);
    void (*visit_typealias)(void* ctx, TypealiasStmt* typealias);
    void (*visit_import)(void* ctx, ImportStmt* import);
    void (*visit_native)(void* ctx, NativeFunctionStmt* native);
    void (*visit_class)(void* ctx, ClassStmt* klass);
} StmtVisitor;

#define STMT_IS_VAR(stmt) ((stmt).kind == STMT_VAR)
#define STMT_IS_FUNCTION(stmt) ((stmt).kind == STMT_FUNCTION)
#define STMT_IS_EXPR(stmt) ((stmt).kind == STMT_EXPR)
#define STMT_IS_LIST(stmt) ((stmt).kind == STMT_LIST)
#define STMT_IS_BLOCK(stmt) ((stmt).kind == STMT_BLOCK)
#define STMT_IS_RETURN(stmt) ((stmt).kind == STMT_RETURN)
#define STMT_IS_IF(stmt) ((stmt).kind == STMT_IF)
#define STMT_IS_FOR(stmt) ((stmt).kind == STMT_FOR)
#define STMT_IS_WHILE(stmt) ((stmt).kind == STMT_WHILE)
#define STMT_IS_LOOPG(stmt) ((stmt).kind == STMT_LOOPG)
#define STMT_IS_TYPEALIAS(stmt) ((stmt).kind == STMT_TYPEALIAS)
#define STMT_IS_IMPORT(stmt) ((stmt).kind == STMT_IMPORT)
#define STMT_IS_NATIVE(stmt) ((stmt).kind == STMT_NATIVE)
#define STMT_IS_CLASS(stmt) ((stmt).kind == STMT_CLASS)

#define CREATE_STMT_RETURN(return_) create_stmt(STMT_RETURN, &return_)
#define CREATE_STMT_VAR(var) create_stmt(STMT_VAR, &var)
#define CREATE_STMT_FUNCTION(fn) create_stmt(STMT_FUNCTION, &fn)
#define CREATE_STMT_EXPR(expr) create_stmt(STMT_EXPR, &expr)
// ListStmt is always a pointer (Because is created using create_list_stmt)
#define CREATE_STMT_LIST(list) create_stmt(STMT_LIST, list)
#define CREATE_STMT_BLOCK(block) create_stmt(STMT_BLOCK, &block)
#define CREATE_STMT_IF(if_) create_stmt(STMT_IF, &if_)
#define CREATE_STMT_FOR(for_) create_stmt(STMT_FOR, &for_)
#define CREATE_STMT_WHILE(while_) create_stmt(STMT_WHILE, &while_)
#define CREATE_STMT_LOOPG(loopg) create_stmt(STMT_LOOPG, &loopg)
#define CREATE_STMT_TYPEALIAS(typealias) create_stmt(STMT_TYPEALIAS, &typealias)
#define CREATE_STMT_IMPORT(import) create_stmt(STMT_IMPORT, &import)
#define CREATE_STMT_NATIVE(native) create_stmt(STMT_NATIVE, &native)
#define CREATE_STMT_CLASS(klass) create_stmt(STMT_CLASS, &klass)

Stmt* create_stmt(StmtKind kind, void* stmt_node);
void free_stmt(Stmt* const stmt);
void stmt_dispatch(StmtVisitor* visitor, void* ctx, Stmt* stmt);

#endif
