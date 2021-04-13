#include "symbol.h"
#include <string.h>
#include "object.h" // for hash function

#define LOAD_FACTOR 0.7

#define IS_EMPTY(symbol) ((symbol)->name.length == 0)
#define S_GROW_CAPACITY(cap) (cap < 8 ? 8 : cap * 2)
#define SHOULD_GROW(table) (table->size + 1 > table->capacity * LOAD_FACTOR)

static Symbol* find(SymbolTable* table, SymbolName* name);
static void grow_symbol_table(SymbolTable* table);

FunctionSymbol create_function_symbol() {
    FunctionSymbol fn_sym = (FunctionSymbol) {
        .return_type = NIL_TYPE
    };
    init_param_array(&fn_sym.param_types);
    return fn_sym;
}

void free_symbol(Symbol* symbol) {
    switch (symbol->kind) {
    case FUNCTION_SYMBOL: {
        free_param_array(&symbol->function.param_types);
        break;
    }
    case VAR_SYMBOL:
        break;
    }
}

void init_symbol_table(SymbolTable* table) {
    table->size = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_symbol_table(SymbolTable* table) {
    for (int i = 0; i < table->size; i++) {
        free_symbol(&table->entries[i]);
    }
    free(table->entries);
    init_symbol_table(table);
}

SymbolName create_symbol_name(const char* str, int length) {
    assert(length != 0);
    assert(str != NULL);
    SymbolName name = (SymbolName){
        .str = str,
        .length = length,
        .hash = hash_string(str, length),
    };
    return name;
}

Symbol* symbol_lookup_str(SymbolTable* table, const char* name, int length) {
    SymbolName symbol_name = create_symbol_name(name, length);
    return symbol_lookup(table, &symbol_name);
}

Symbol* symbol_lookup(SymbolTable* table, SymbolName* name) {
    if (table->capacity == 0) {
        return NULL;
    }
    Symbol* symbol = find(table, name);
    if (IS_EMPTY(symbol)) {
        return NULL;
    }
    return symbol;
}

void symbol_insert(SymbolTable* table, Symbol symbol) {
    if (SHOULD_GROW(table)) {
        grow_symbol_table(table);
    }
    assert(table->size + 1 < table->capacity);
    Symbol* destination = find(table, &symbol.name);
    (*destination) = symbol;
    table->size++;
}

static void grow_symbol_table(SymbolTable* table) {
    Symbol* old_entries = table->entries;
    int old_capacity = table->capacity;
    table->capacity = S_GROW_CAPACITY(old_capacity);
    table->entries = (Symbol*) malloc(sizeof(Symbol) * table->capacity);

    for (int i = 0; i < table->capacity; i++) {
        Symbol* symbol = &table->entries[i];
        symbol->declaration_line = 0;
        symbol->type = UNKNOWN_TYPE;
        symbol->name.str = NULL;
        symbol->name.length = 0;
        symbol->name.hash = 0;
    }

    for (int i = 0; i < old_capacity; i++) {
        if (IS_EMPTY(&old_entries[i])) {
            continue;
        }
        Symbol* symbol = find(table, &old_entries[i].name);
        *symbol = old_entries[i];
    }

    // Just free the array. Do not free key str.
    free(old_entries);
}

static Symbol* find(SymbolTable* table, SymbolName* name) {
    assert(table != NULL);
    assert(name != NULL);
    assert(name->str != NULL);
    assert(name->length != 0);
    int index = name->hash & (table->capacity - 1);
    for (;;) {
        Symbol* current = &table->entries[index];
        if (IS_EMPTY(current)) {
            return current;
        }
        if (
            current->name.hash == name->hash &&
            current->name.length == name->length &&
            memcmp(current->name.str, name->str, name->length) == 0
        ) {
            return current;
        }
        index = index + 1 & (table->capacity - 1);
    }
}

#define NODE_CHILDS_SHOULD_GROW(node) (node->size + 1 > node->capacity)
#define NODE_GROW_CAPACITY(node) ((node->capacity == 0) ? 8 : node->capacity * 2)

void init_symbol_node(SymbolNode* node) {
    init_symbol_table(&node->symbols);
    node->father = NULL;
    node->childs = NULL;
    node->size = 0;
    node->capacity = 0;
    node->next_node_to_visit = 0;
}

void free_symbol_node(SymbolNode* node) {
    free_symbol_table(&node->symbols);
    for (int i = 0; i < node->size; i++) {
        free_symbol_node(&node->childs[i]);
    }
    free(node->childs);
}

void symbol_node_reset(SymbolNode* node) {
    node->next_node_to_visit = 0;
    for (int i = 0; i < node->size; i++) {
        symbol_node_reset(&node->childs[i]);
    }
}

SymbolNode* symbol_node_add_child(SymbolNode* node, SymbolNode* child) {
    if (NODE_CHILDS_SHOULD_GROW(node)) {
        node->capacity = NODE_GROW_CAPACITY(node);
        node->childs = (SymbolNode*) realloc(node->childs, sizeof(SymbolNode) * node->capacity);
    }
    assert(node->capacity > 0);
    assert(node->childs != NULL);
    child->father = node;
    node->childs[node->size] = *child;
    node->size++;
    return &node->childs[node->size - 1];
}

void init_scoped_symbol_table(ScopedSymbolTable* table) {
    init_symbol_node(&table->global);
    table->current = &table->global;
}

void free_scoped_symbol_table(ScopedSymbolTable* table) {
    free_symbol_node(&table->global);
    table->current = NULL;
}

void symbol_create_scope(ScopedSymbolTable* table) {
    assert(table->current != NULL);
    SymbolNode child;
    init_symbol_node(&child);
    table->current = symbol_node_add_child(table->current, &child);
}

void symbol_end_scope(ScopedSymbolTable* table) {
    assert(table->current != NULL);
    assert(table->current->father != NULL);
    table->current = table->current->father;
}

void symbol_start_scope(ScopedSymbolTable* table) {
    assert(table->current != NULL);
    assert(table->current->childs != NULL);
    assert(table->current->capacity > 0);
    assert(table->current->size > 0);
    assert(table->current->next_node_to_visit < table->current->size);
    table->current->next_node_to_visit++;
    table->current = &table->current->childs[table->current->next_node_to_visit - 1];
}

void symbol_reset_scopes(ScopedSymbolTable* table) {
    symbol_node_reset(&table->global);
    table->current = &table->global;
}

Symbol* scoped_symbol_lookup(ScopedSymbolTable* table, SymbolName* name) {
    assert(table->current != NULL);
    SymbolNode* current = table->current;
    Symbol* symbol = NULL;
    while (current != NULL) {
        symbol = symbol_lookup(&current->symbols, name);
        if (symbol != NULL) {
            return symbol;
        }
        current = current->father;
    }
    return NULL;
}

Symbol* scoped_symbol_lookup_str(ScopedSymbolTable* table, const char* name, int length) {
    SymbolName symbol_name = create_symbol_name(name, length);
    return scoped_symbol_lookup(table, &symbol_name);
}

void scoped_symbol_insert(ScopedSymbolTable* table, Symbol entry) {
    assert(table->current != NULL);
    symbol_insert(&table->current->symbols, entry);
}
