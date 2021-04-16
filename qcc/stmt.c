#include "stmt.h"

static void free_list_stmt(ListStmt* list_stmt);
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

void stmt_list_add(ListStmt* list, Stmt* stmt) {
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
    switch(kind) {
    case STMT_EXPR:
        stmt->kind = STMT_EXPR;
        stmt->expr = *(ExprStmt*)stmt_node;
        break;
    case STMT_VAR:
        stmt->kind = STMT_VAR;
        stmt->var = *(VarStmt*)stmt_node;
        break;
    case STMT_FUNCTION:
        stmt->kind = STMT_FUNCTION;
        stmt->function = *(FunctionStmt*)stmt_node;
        break;
    case STMT_LIST:
        stmt->kind = STMT_LIST;
        stmt->list = (ListStmt*)stmt_node;
        break;
    case STMT_PRINT:
        stmt->kind = STMT_PRINT;
        stmt->print = *(PrintStmt*)stmt_node;
        break;
    case STMT_BLOCK:
        stmt->kind = STMT_BLOCK;
        stmt->block = *(BlockStmt*)stmt_node;
        break;
    case STMT_RETURN:
        stmt->kind = STMT_RETURN;
        stmt->return_ = *(ReturnStmt*)stmt_node;
        break;
    }
    return stmt;
}

static void free_list_stmt(ListStmt* list_stmt) {
    for (int i = 0; i < list_stmt->size; i++) {
        free_stmt(list_stmt->stmts[i]);
    }
    free(list_stmt->stmts);
}

void free_stmt(Stmt* stmt) {
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
    }
#undef DISPATCH
}
