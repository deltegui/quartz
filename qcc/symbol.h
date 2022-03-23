#ifndef QUARTZ_SYMBOL_H_
#define QUARTZ_SYMBOL_H_

#include "type.h"
#include "vector.h"
#include "ctable.h"
#include "stmt.h" // for NativeClassStmt

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
    SYMBOL_CLASS,
} SymbolKind;

typedef enum {
    SYMBOL_VISIBILITY_UNDEFINED,
    SYMBOL_VISIBILITY_PRIVATE,
    SYMBOL_VISIBILITY_PUBLIC,
} SymbolVisibility;

struct s_symbol_set;
struct s_symbol_table;

typedef struct {
    // This must not be NULL, but it cannot
    // be setted until the body node is created
    struct s_symbol_table* body;
} ClassSymbol;

typedef struct {
    Vector param_names; // Vector<Token>
    // Function type information is stored in Type* type inside Symbol struct

    // This is used for out upvalue references. That is, other variables that
    // this function is closed over. Is mainly used to bind open upvalues to
    // those variables.
    struct s_symbol_set* upvalues;
} FunctionSymbol;

typedef struct {
    SymbolKind kind;
    SymbolName name;

    Type* type;

    SymbolVisibility visibility;

    uint32_t line;
    uint32_t column;
    uint16_t constant_index;

    bool global;
    bool assigned;
    bool native;

    // This is used for in upvalue refereces. That is, other functions that
    // this variable requested to closed over them. Is mainly used to close
    // open upvalues in that functions when this variable is going to be out
    // of scope.
    struct s_symbol_set* upvalue_fn_refs;

    union {
        FunctionSymbol function;
        ClassSymbol klass;
    };
} Symbol;

Symbol create_symbol_from_token(Token* token, Type* type);
Symbol create_symbol(SymbolName name, int line, int column, Type* type);
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

typedef struct s_symbol_node {
    SymbolTable symbols;
    struct s_symbol_node* father;
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
Symbol* scoped_symbol_lookup_object_prop(Symbol* const obj_sym, SymbolName* name);
Symbol* scoped_symbol_lookup_object_prop_str(Symbol* const obj_sym, const char* name, int legnth);
void scoped_symbol_insert(ScopedSymbolTable* const table, Symbol entry);
void scoped_symbol_upvalue(ScopedSymbolTable* const table, Symbol* fn, Symbol* var_upvalue);
void scoped_symbol_update_class_body(ScopedSymbolTable* const table, Symbol* obj);
Symbol* scoped_symbol_get_class_prop(ScopedSymbolTable* const table, Type* class_type, Token* prop, Symbol** class_sym_out);
Symbol* scoped_symbol_get_class_prop_str(ScopedSymbolTable* const table, const char* const class_name, int length, Token* prop, Symbol** class_sym_out);

#define SCOPED_SYMBOL_LOOKUP_OBJECT_INIT(sym) (scoped_symbol_lookup_object_prop_str(sym, CLASS_CONSTRUCTOR_NAME, CLASS_CONSTRUCTOR_LENGTH))

typedef struct s_symbol_set {
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

NativeClassStmt register_native_class(ScopedSymbolTable* const table, char* name, int length, void (*register_fn)(ScopedSymbolTable* const table));

#define NATIVE_CLASS_INIT(var_name, name_f, len_f, fn, ...) do {\
    Type* type_f;\
    __VA_ARGS__\
    if (var_name == NULL) {\
        var_name = new_native(name_f, len_f, fn, type_f);\
    }\
} while (false)

#define NATIVE_INSERT_METHOD(table, native_obj, cte) do {\
    Symbol sym = create_symbol(\
        create_symbol_name(\
            native_obj->name,\
            native_obj->length),\
        0,\
        0,\
        native_obj->obj.type);\
    sym.visibility = SYMBOL_VISIBILITY_PUBLIC;\
    sym.constant_index = cte++;\
    scoped_symbol_insert(table, sym);\
} while (false)

#define NATIVE_PUSH_PROP(props, native_obj) do {\
valuearray_write(props, OBJ_VALUE(native_obj, native_obj->obj.type));\
} while (false)

#endif
