#ifndef QUARTZ_SYMBOL_H
#define QUARTZ_SYMBOL_H

#include "type.h"

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
    uint16_t constant_index;
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
#define CSYMBOL_LOOKUP_STR(name, length) symbol_lookup_str(&symbol_table, name, length)
#define CSYMBOL_INSERT(symbol) symbol_insert(&symbol_table, symbol)

void init_symbol_table(SymbolTable* table);
void free_symbol_table(SymbolTable* table);
Symbol* symbol_lookup(SymbolTable* table, SymbolName* name);
Symbol* symbol_lookup_str(SymbolTable* table, const char* name, int length);
void symbol_insert(SymbolTable* table, Symbol entry);

#endif