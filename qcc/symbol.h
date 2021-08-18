#ifndef QUARTZ_SYMBOL_H_
#define QUARTZ_SYMBOL_H_

#include "type.h"
#include "vector.h"

typedef struct {
    const char* str;
    int length;
    uint32_t hash;
} SymbolName;

SymbolName create_symbol_name(const char* str, int length);

typedef enum {
    SYMBOL_FUNCTION,
    SYMBOL_VAR
} SymbolKind;

typedef struct {
    Vector param_names; // Vector<Token>
    // Function type information is stored in Type* type inside Symbol struct

    // This is used for out upvalue references. That is, other variables that
    // this function is closed over. Is mainly used to bind open upvalues to
    // those variables.
    Vector upvalues; // Vector<Token>.
} FunctionSymbol;

typedef struct {
    SymbolKind kind;
    SymbolName name;

    Type* type;

    uint32_t declaration_line;
    uint16_t constant_index;

    bool global;

    // This is used for in upvalue refereces. That is, other functions that
    // this variable requested to closed over them. Is mainly used to close
    // open upvalues in that functions when this variable is going to be out
    // of scope.
    Vector upvalue_fn_refs; // Vector <Token>

    union {
        FunctionSymbol function;
    };
} Symbol;

Symbol create_symbol_from_token(Token* token, Type* type);
Symbol create_symbol(SymbolName name, int line, Type* type);
void free_symbol(Symbol* const symbol);
// TODO change name (is_closed instead)
bool symbol_is_closed(Symbol* const symbol, Token fn_name);
int symbol_get_function_upvalue_index(Symbol* const symbol, Token upvalue);

#define SYMBOL_GET_FUNCTION_UPVALUE_SIZE(sym) (sym->function.upvlaues.size)

typedef struct {
    Symbol* entries;
    int size;
    int capacity;
} SymbolTable;

void init_symbol_table(SymbolTable* const table);
void free_symbol_table(SymbolTable* const table);
Symbol* symbol_lookup(SymbolTable* const table, SymbolName* name);
Symbol* symbol_lookup_str(SymbolTable* const table, const char* name, int length);
void symbol_insert(SymbolTable* const table, Symbol entry);

typedef struct _SymbolNode {
    SymbolTable symbols;
    struct _SymbolNode* father;
    struct _SymbolNode* childs;
    int size;
    int capacity;
    int next_node_to_visit;
} SymbolNode;

void init_symbol_node(SymbolNode* const node);
void free_symbol_node(SymbolNode* const node);
void symbol_node_reset(SymbolNode* const node);
SymbolNode* symbol_node_add_child(SymbolNode* const node, SymbolNode* const child);

typedef struct {
    SymbolNode global;
    SymbolNode* current;
} ScopedSymbolTable;

void init_scoped_symbol_table(ScopedSymbolTable* const table);
void free_scoped_symbol_table(ScopedSymbolTable* const table);

void symbol_create_scope(ScopedSymbolTable* const table);
void symbol_end_scope(ScopedSymbolTable* const table);
void symbol_start_scope(ScopedSymbolTable* const table);
void symbol_reset_scopes(ScopedSymbolTable* const table);

Symbol* scoped_symbol_lookup(ScopedSymbolTable* const table, SymbolName* name);
Symbol* scoped_symbol_lookup_str(ScopedSymbolTable* const table, const char* name, int length);
Symbol* scoped_symbol_lookup_levels(ScopedSymbolTable* const table, SymbolName* name, int levels);
Symbol* scoped_symbol_lookup_levels_str(ScopedSymbolTable* const table, const char* name, int length, int levels);
void scoped_symbol_insert(ScopedSymbolTable* const table, Symbol entry);
void scoped_symbol_upvalue(ScopedSymbolTable* const table, Token fn, Token var_upvalue);

#endif
