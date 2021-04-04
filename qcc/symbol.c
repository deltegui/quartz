#include "symbol.h"
#include <string.h>
#include "object.h" // for hash function

#define LOAD_FACTOR 0.7

#define IS_EMPTY(symbol) ((symbol)->name.length == 0)
#define S_GROW_CAPACITY(cap) (cap < 8 ? 8 : cap * 2)
#define SHOULD_GROW(table) (table->size + 1 > table->capacity * LOAD_FACTOR)

static Symbol* find(SymbolTable* table, SymbolName* name);
static void grow_symbol_table(SymbolTable* table);

void init_symbol_table(SymbolTable* table) {
    table->size = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_symbol_table(SymbolTable* table) {
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
