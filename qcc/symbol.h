#ifndef QUARTZ_SYMBOL_H_
#define QUARTZ_SYMBOL_H_

#include "type.h"
#include "vector.h"
#include "ctable.h"

typedef struct {
    CTableKey key;
} SymbolName;

SymbolName create_symbol_name(const char* start, int length);

#define SYMBOL_NAME_START(name) ((name).key.start)
#define SYMBOL_NAME_LENGTH(name) ((name).key.length)
#define SYMBOL_NAME_HASH(name) ((name).key.hash)

typedef enum {
    SYMBOL_TYPEALIAS,
    SYMBOL_FUNCTION,
    SYMBOL_VAR,
    SYMBOL_OBJECT,
} SymbolKind;

typedef enum {
    SYMBOL_VISIBILITY_UNDEFINED,
    SYMBOL_VISIBILITY_PUBLIC,
    SYMBOL_VISIBILITY_PRIVATE,
} SymbolVisibility;

// TODO symbol set change
struct _SymbolSet;
struct s_symbol_table;

typedef struct {
    struct s_symbol_table* symbols;
} ObjectSymbol;

typedef struct {
    Vector param_names; // Vector<Token>
    // Function type information is stored in Type* type inside Symbol struct

    // This is used for out upvalue references. That is, other variables that
    // this function is closed over. Is mainly used to bind open upvalues to
    // those variables.
    struct _SymbolSet* upvalues;
} FunctionSymbol;

typedef struct {
    SymbolKind kind;
    SymbolName name;

    Type* type;

    SymbolVisibility visibility;

    uint32_t declaration_line;
    uint16_t constant_index;

    bool global;
    bool assigned;
    bool native;

    // This is used for in upvalue refereces. That is, other functions that
    // this variable requested to closed over them. Is mainly used to close
    // open upvalues in that functions when this variable is going to be out
    // of scope.
    struct _SymbolSet* upvalue_fn_refs;

    union {
        FunctionSymbol function;
        ObjectSymbol object;
    };
} Symbol;

Symbol create_symbol_from_token(Token* token, Type* type);
Symbol create_symbol(SymbolName name, int line, Type* type);
void free_symbol(Symbol* const symbol);
int symbol_get_function_upvalue_index(Symbol* const symbol, Symbol* upvalue);

typedef struct s_symbol_table {
    CTable table; // CTable<Symbol>
} SymbolTable;

void init_symbol_table(SymbolTable* const table);
void free_symbol_table(SymbolTable* const table);
Symbol* symbol_lookup(SymbolTable* const table, SymbolName* name);
Symbol* symbol_lookup_str(SymbolTable* const table, const char* name, int length);
void symbol_insert(SymbolTable* const table, Symbol entry);

#define SYMBOL_TABLE_FOREACH(symbols, block) CTABLE_FOREACH(&(symbols)->table, Symbol, block)

typedef struct _SymbolNode {
    SymbolTable symbols;
    struct _SymbolNode* father;
    Vector childs; // Vector<SymbolNode>
    uint32_t next_node_to_visit;
} SymbolNode;

#define VECTOR_AS_SYMBOL_NODE(vect) VECTOR_AS(vect, SymbolNode)
#define VECTOR_ADD_SYMBOL_NODE(vect, node) VECTOR_ADD(vect, node, SymbolNode)

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
Symbol* scoped_symbol_lookup_function(ScopedSymbolTable* const table, SymbolName* name);
Symbol* scoped_symbol_lookup_function_str(ScopedSymbolTable* const table, const char* name, int length);
void scoped_symbol_insert(ScopedSymbolTable* const table, Symbol entry);
void scoped_symbol_upvalue(ScopedSymbolTable* const table,  Symbol* fn, Symbol* var_upvalue);

typedef struct _SymbolSet {
    CTable table; // CTable<Symbol*>
} SymbolSet;

SymbolSet* create_symbol_set();
void free_symbol_set(SymbolSet* const set);
void symbol_set_add(SymbolSet* const set, Symbol* symbol);

#define SYMBOL_SET_GET_ELEMENTS(set) ((Symbol**) ((set)->table.data.elements))
#define SYMBOL_SET_SIZE(set) ((set)->table.data.size)
#define SYMBOL_SET_FOREACH(set, block) CTABLE_FOREACH((CTable*)(set), Symbol*, block)

typedef struct {
    SymbolNode* current;
    int current_upvalue;
    int depth;
} UpvalueIterator;

void init_upvalue_iterator(UpvalueIterator* const iterator, ScopedSymbolTable* table, int depth);
Symbol* upvalue_iterator_next(UpvalueIterator* const iterator);

#endif
