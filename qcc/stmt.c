#include "stmt.h"

static void free_list_stmt(ListStmt* list_stmt);
static void visit_list_stmt(StmtVisitor* visitor, void* ctx, ListStmt* list);

ListStmt* create_list_stmt() {
#define INITIAL_CAPACITY 2
    ListStmt* list_stmt = (ListStmt*) malloc(sizeof(ListStmt));
    list_stmt->stmts = (Stmt**) malloc(sizeof(Stmt*) * INITIAL_CAPACITY);
    list_stmt->capacity = INITIAL_CAPACITY;
    list_stmt->length = 0;
    return list_stmt;
#undef INITIAL_CAPACITY
}

void list_stmt_add(ListStmt* list, Stmt* stmt) {
#define GROWTH_FACTOR 2
    if (list->capacity <= list->length + 1) {
        list->capacity = list->capacity * GROWTH_FACTOR;
        list->stmts = (Stmt**) realloc(list->stmts, sizeof(Stmt**) * list->capacity);
    }
    list->stmts[list->length] = stmt;
    list->length++;
#undef GROWTH_FACTOR
}

Stmt* create_stmt(StmtKind kind, void* stmt_node) {
    Stmt* stmt = (Stmt*) malloc(sizeof(Stmt));
    switch(kind) {
    case EXPR_STMT:
        stmt->kind = EXPR_STMT;
        stmt->expr = *(ExprStmt*)stmt_node;
        break;
    case VAR_STMT:
        stmt->kind = VAR_STMT;
        stmt->var = *(VarStmt*)stmt_node;
        break;
    case LIST_STMT:
        stmt->kind = LIST_STMT;
        stmt->list = (ListStmt*)stmt_node;
        break;
    case PRINT_STMT:
        stmt->kind = PRINT_STMT;
        stmt->print = *(PrintStmt*)stmt_node;
        break;
    case BLOCK_STMT:
        stmt->kind = BLOCK_STMT;
        stmt->block = *(BlockStmt*)stmt_node;
        break;
    }
    return stmt;
}

static void free_list_stmt(ListStmt* list_stmt) {
    for (int i = 0; i < list_stmt->length; i++) {
        free_stmt(list_stmt->stmts[i]);
    }
    free(list_stmt->stmts);
}

void free_stmt(Stmt* stmt) {
    if (stmt == NULL) {
        return;
    }
    switch(stmt->kind) {
    case EXPR_STMT:
        free_expr(stmt->expr.inner);
        break;
    case VAR_STMT:
        free_expr(stmt->var.definition);
        break;
    case LIST_STMT:
        free_list_stmt(stmt->list);
        free(stmt->list);
        break;
    case PRINT_STMT:
        free_expr(stmt->print.inner);
        break;
    case BLOCK_STMT:
        free_stmt(stmt->block.stmts);
        break;
    }
    free(stmt);
}

static void visit_list_stmt(StmtVisitor* visitor, void* ctx, ListStmt* list) {
    for (int i = 0; i < list->length; i++) {
        stmt_dispatch(visitor, ctx, list->stmts[i]);
    }
}

void stmt_dispatch(StmtVisitor* visitor, void* ctx, Stmt* stmt) {
    if (stmt == NULL) {
        return;
    }
#define DISPATCH(fn_visitor, node_type) visitor->fn_visitor(ctx, &stmt->node_type)
    switch (stmt->kind) {
    case EXPR_STMT: DISPATCH(visit_expr, expr); break;
    case VAR_STMT: DISPATCH(visit_var, var); break;
    case LIST_STMT: visit_list_stmt(visitor, ctx, stmt->list); break;
    case PRINT_STMT: DISPATCH(visit_print, print); break;
    case BLOCK_STMT: DISPATCH(visit_block, block); break;
    }
#undef DISPATCH
}
