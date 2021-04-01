#include "symbol.h"
#include <string.h>
#include "object.h" // for hash function

#define LOAD_FACTOR 0.7

#define IS_EMPTY(entry) ((entry)->key.length == 0)
#define S_GROW_CAPACITY(cap) (cap < 8 ? 8 : cap * 2)
#define SHOULD_GROW(table) (table->size + 1 > table->capacity * LOAD_FACTOR)

static Entry* find(SymbolTable* table, Key* key);
static void grow_symbol_table(SymbolTable* table);

void init_symbol_table(SymbolTable* table) {
    table->size = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_symbol_table(SymbolTable* table) {
    for (int i = 0; i < table->capacity; i++) {
        if (! IS_EMPTY(&table->entries[i])) {
            free((char*) table->entries[i].key.str);
        }
    }
    free(table->entries);
    init_symbol_table(table);
}

Key create_symbol_key(const char* str, int length) {
    assert(length != 0);
    assert(str != NULL);
    char* owned = (char*) malloc(length + 1);
    memcpy(owned, str, length);
    owned[length] = '\0';
    Key k = (Key){
        .str = owned,
        .length = length,
        .hash = hash_string(str, length),
    };
    return k;
}

Entry* symbol_lookup(SymbolTable* table, Key* key) {
    Entry* entry = find(table, key);
    if (IS_EMPTY(entry)) {
        return NULL;
    }
    return entry;
}

void symbol_insert(SymbolTable* table, Entry entry) {
    if (SHOULD_GROW(table)) {
        grow_symbol_table(table);
    }
    assert(table->size + 1 < table->capacity);
    Entry* destination = find(table, &entry.key);
    (*destination) = entry;
    table->size++;
}

static void grow_symbol_table(SymbolTable* table) {
    Entry* old_entries = table->entries;
    int old_capacity = table->capacity;
    table->capacity = S_GROW_CAPACITY(old_capacity);
    table->entries = (Entry*) malloc(sizeof(Entry) * table->capacity);

    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        entry->declaration_line = 0;
        entry->type = UNKNOWN_TYPE;
        entry->key.str = NULL;
        entry->key.length = 0;
        entry->key.hash = 0;
    }

    for (int i = 0; i < old_capacity; i++) {
        Entry* entry = find(table, &old_entries[i].key);
        *entry = old_entries[i];
    }

    // Just free the array. Do not free key str.
    free(old_entries);
}

static Entry* find(SymbolTable* table, Key* key) {
    assert(table != NULL);
    assert(key != NULL);
    assert(key->str != NULL);
    assert(key->length != 0);
    int index = key->hash & (table->capacity - 1);
    for (;;) {
        Entry* current = &table->entries[index];
        if (IS_EMPTY(current)) {
            return current;
        }
        if (
            current->key.hash == key->hash &&
            current->key.length == key->length &&
            memcmp(current->key.str, key->str, key->length) == 0
        ) {
            return current;
        }
        index = index + 1 & (table->capacity - 1);
    }
}
