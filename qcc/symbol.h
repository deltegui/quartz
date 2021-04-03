#ifndef QUARTZ_SYMBOL_H
#define QUARTZ_SYMBOL_H

#include "typechecker.h"

typedef struct {
    const char* str;
    int length;
    uint32_t hash;
} SymbolName;

SymbolName create_symbol_name(const char* str, int length);

typedef struct {
    SymbolName name;
    int declaration_line;
    Type type;
} Symbol;

typedef struct {
    Symbol* entries;
    int size;
    int capacity;
} SymbolTable;

SymbolTable symbol_table;

#define INIT_CSYMBOL_TABLE() init_symbol_table(&symbol_table)
#define FREE_CSYMBOL_TABLE() free_symbol_table(&symbol_table)
#define CSYMBOL_LOOKUP(name) symbol_lookup(&symbol_table, name)
#define CSYMBOL_INSERT(symbol) symbol_insert(&symbol_table, symbol)

void init_symbol_table(SymbolTable* table);
void free_symbol_table(SymbolTable* table);
Symbol* symbol_lookup(SymbolTable* table, SymbolName* name);
void symbol_insert(SymbolTable* table, Symbol entry);

#endif