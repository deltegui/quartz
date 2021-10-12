#include "stmt.h"

static void free_list_stmt(ListStmt* const list_stmt);
static void visit_list_stmt(StmtVisitor* visitor, void* ctx, ListStmt* list);

ListStmt* create_stmt_list() {
#define INITIAL_CAPACITY 2
    ListStmt* list_stmt = (ListStmt*) malloc(sizeof(ListStmt));
    list_stmt->stmts = (Stmt**) malloc(sizeof(Stmt*) * INITIAL_CAPACITY);
    list_stmt->capacity = INITIAL_CAPACITY;
    list_stmt->size = 0;
    return list_stmt;
#undef INITIAL_CAPACITY
}

void stmt_list_add(ListStmt* const list, Stmt* stmt) {
#define GROWTH_FACTOR 2
    if (list->capacity <= list->size + 1) {
        list->capacity = list->capacity * GROWTH_FACTOR;
        list->stmts = (Stmt**) realloc(list->stmts, sizeof(Stmt**) * list->capacity);
    }
    list->stmts[list->size] = stmt;
    list->size++;
#undef GROWTH_FACTOR
}

Stmt* create_stmt(StmtKind kind, void* stmt_node) {
    Stmt* stmt = (Stmt*) malloc(sizeof(Stmt));

#define CASE_STMT(k, f, s)\
    case k:\
        stmt->kind = k;\
        stmt->f = *(s*)stmt_node;\
        break

    switch(kind) {
    CASE_STMT(STMT_EXPR, expr, ExprStmt);
    CASE_STMT(STMT_VAR, var, VarStmt);
    CASE_STMT(STMT_FUNCTION, function, FunctionStmt);
    CASE_STMT(STMT_PRINT, print, PrintStmt);
    CASE_STMT(STMT_BLOCK, block, BlockStmt);
    CASE_STMT(STMT_RETURN, return_, ReturnStmt);
    CASE_STMT(STMT_IF, if_, IfStmt);
    CASE_STMT(STMT_FOR, for_, ForStmt);
    CASE_STMT(STMT_WHILE, while_, WhileStmt);
    CASE_STMT(STMT_BREAK, break_, BreakStmt);
    case STMT_LIST:
        stmt->kind = STMT_LIST;
        stmt->list = (ListStmt*)stmt_node;
        break;
    }
    return stmt;

#undef SET_STMT
}

static void free_list_stmt(ListStmt* const list_stmt) {
    for (int i = 0; i < list_stmt->size; i++) {
        free_stmt(list_stmt->stmts[i]);
    }
    free(list_stmt->stmts);
}

void free_stmt(Stmt* const stmt) {
    if (stmt == NULL) {
        return;
    }
    switch(stmt->kind) {
    case STMT_EXPR:
        free_expr(stmt->expr.inner);
        break;
    case STMT_VAR:
        free_expr(stmt->var.definition);
        break;
    case STMT_FUNCTION:
        free_stmt(stmt->function.body);
        break;
    case STMT_LIST:
        free_list_stmt(stmt->list);
        free(stmt->list);
        break;
    case STMT_PRINT:
        free_expr(stmt->print.inner);
        break;
    case STMT_BLOCK:
        free_stmt(stmt->block.stmts);
        break;
    case STMT_RETURN:
        free_expr(stmt->return_.inner);
        break;
    case STMT_IF:
        free_expr(stmt->if_.condition);
        free_stmt(stmt->if_.then);
        free_stmt(stmt->if_.else_);
        break;
    case STMT_FOR:
        free_expr(stmt->for_.condition);
        free_stmt(stmt->for_.init);
        free_stmt(stmt->for_.mod);
        free_stmt(stmt->for_.body);
        break;
    case STMT_WHILE:
        free_expr(stmt->while_.condition);
        free_stmt(stmt->while_.body);
        break;
    case STMT_BREAK:
        break;
    }
    free(stmt);
}

static void visit_list_stmt(StmtVisitor* visitor, void* ctx, ListStmt* list) {
    for (int i = 0; i < list->size; i++) {
        stmt_dispatch(visitor, ctx, list->stmts[i]);
    }
}

void stmt_dispatch(StmtVisitor* visitor, void* ctx, Stmt* stmt) {
    if (stmt == NULL) {
        return;
    }
#define DISPATCH(fn_visitor, node_type) visitor->fn_visitor(ctx, &stmt->node_type)
    switch (stmt->kind) {
    case STMT_EXPR: DISPATCH(visit_expr, expr); break;
    case STMT_VAR: DISPATCH(visit_var, var); break;
    case STMT_LIST: visit_list_stmt(visitor, ctx, stmt->list); break;
    case STMT_PRINT: DISPATCH(visit_print, print); break;
    case STMT_BLOCK: DISPATCH(visit_block, block); break;
    case STMT_FUNCTION: DISPATCH(visit_function, function); break;
    case STMT_RETURN: DISPATCH(visit_return, return_); break;
    case STMT_IF: DISPATCH(visit_if, if_); break;
    case STMT_FOR: DISPATCH(visit_for, for_); break;
    case STMT_WHILE: DISPATCH(visit_while, while_); break;
    case STMT_BREAK: DISPATCH(visit_break, break_); break;
    default: assert(false);
    }
#undef DISPATCH
}
