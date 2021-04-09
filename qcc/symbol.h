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

void init_symbol_table(SymbolTable* table);
void free_symbol_table(SymbolTable* table);
Symbol* symbol_lookup(SymbolTable* table, SymbolName* name);
// TODO delete this
Symbol* symbol_lookup_str(SymbolTable* table, const char* name, int length);
void symbol_insert(SymbolTable* table, Symbol entry);

typedef struct _SymbolNode {
    SymbolTable symbols;
    struct _SymbolNode* father;
    struct _SymbolNode* childs;
    int size;
    int capacity;
    int next_node_to_visit;
} SymbolNode;

void init_symbol_node(SymbolNode* node);
void free_symbol_node(SymbolNode* node);
void symbol_node_reset(SymbolNode* node);
SymbolNode* symbol_node_add_child(SymbolNode* node, SymbolNode* child);

typedef struct {
    SymbolNode global;
    SymbolNode* current;
} ScopedSymbolTable;

void init_scoped_symbol_table(ScopedSymbolTable* table);
void free_scoped_symbol_table(ScopedSymbolTable* table);

void symbol_create_scope(ScopedSymbolTable* table);
void symbol_end_scope(ScopedSymbolTable* table);
void symbol_start_scope(ScopedSymbolTable* table);
void symbol_reset_scopes(ScopedSymbolTable* table);

Symbol* scoped_symbol_lookup(ScopedSymbolTable* table, SymbolName* name);
Symbol* scoped_symbol_lookup_str(ScopedSymbolTable* table, const char* name, int length);
void scoped_symbol_insert(ScopedSymbolTable* table, Symbol entry);

#endif