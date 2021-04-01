#ifndef QUARTZ_SYMBOL_H
#define QUARTZ_SYMBOL_H

#include "typechecker.h"

typedef struct {
    const char* str;
    int length;
    uint32_t hash;
} Key;

typedef struct {
    Key key;
    int declaration_line;
    Type type;
} Entry;

typedef struct {
    Entry* entries;
    int size;
    int capacity;
} SymbolTable;

Key create_symbol_key(const char* str, int length);
void init_symbol_table(SymbolTable* table);
void free_symbol_table(SymbolTable* table);
Entry* symbol_lookup(SymbolTable* table, Key* key);
void symbol_insert(SymbolTable* table, Entry entry);

#endif