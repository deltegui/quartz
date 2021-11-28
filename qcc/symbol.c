#include "symbol.h"
#include <string.h>
#include <limits.h> // for INT_MAX
#include "token.h" // for Vector<Token>

typedef bool (*ExitCondition) (Symbol*);

static bool symbol_name_equals(SymbolName* first, SymbolName* second);
static SymbolKind kind_from_type(Type* type);
static void create_function_symbol(Symbol* const symbol);
static bool find_next_scope_with_upvalues(UpvalueIterator* const iterator);
static Symbol* scoped_symbol_lookup_levels_conditional(ScopedSymbolTable* const table, SymbolName* name, int levels, ExitCondition test_condition);

SymbolName create_symbol_name(const char* start, int length) {
    CTableKey key = create_ctable_key(start, length);
    return (SymbolName) {
        .key = key,
    };
}

static bool symbol_name_equals(SymbolName* first, SymbolName* second) {
    return ctable_key_equals((CTableKey*)first, (CTableKey*)second);
}

Symbol create_symbol_from_token(Token* token, Type* type) {
    return create_symbol(
        create_symbol_name(token->start, token->length),
        token->line,
        type);
}

Symbol create_symbol(SymbolName name, int line, Type* type) {
    Symbol symbol = (Symbol){
        .kind = kind_from_type(type),
        .name = name,
        .type = type,
        .visibility = SYMBOL_VISIBILITY_UNDEFINED,
        .declaration_line = line,
        .constant_index = UINT16_MAX,
        .global = false, // we dont know
        .assigned = true, // normally is
        .native = false, // normally its not native
        .static_ = false, // normally its not static
    };
    symbol.upvalue_fn_refs = create_symbol_set();
    if (symbol.kind == SYMBOL_FUNCTION) {
        create_function_symbol(&symbol);
    }
    if (symbol.kind == SYMBOL_OBJECT) {
        symbol.object.body = NULL;
    }
    return symbol;
}

static SymbolKind kind_from_type(Type* type) {
    switch (type->kind) {
    case TYPE_FUNCTION: return SYMBOL_FUNCTION;
    case TYPE_OBJECT: return SYMBOL_OBJECT;
    default: return SYMBOL_VAR;
    }
}

static void create_function_symbol(Symbol* const symbol) {
    FunctionSymbol* fn_sym = &symbol->function;
    init_vector(&fn_sym->param_names, sizeof(Token));
    fn_sym->upvalues = create_symbol_set();
}

void free_symbol(Symbol* const symbol) {
    // Notice we dont own Type* (symbol->type). Please DO NOT FREE Type*.
    switch (symbol->kind) {
    case SYMBOL_FUNCTION: {
        free_vector(&symbol->function.param_names);
        free_symbol_set(symbol->function.upvalues);
        break;
    }
    case SYMBOL_OBJECT:
    case SYMBOL_TYPEALIAS:
    case SYMBOL_VAR:
        break;
    }
    free_symbol_set(symbol->upvalue_fn_refs);
}

int symbol_get_function_upvalue_index(Symbol* const symbol, Symbol* upvalue) {
    assert(symbol->kind == SYMBOL_FUNCTION);
    SYMBOL_SET_FOREACH(symbol->function.upvalues, {
        if (symbol_name_equals(&elements[i]->name, &upvalue->name)) {
            return i;
        }
    });
    return -1;
}

void init_symbol_table(SymbolTable* const table) {
    init_ctable((CTable*)table, sizeof(Symbol));
}

void free_symbol_table(SymbolTable* const table) {
    CTable* ctable = (CTable*) table;
    CTABLE_FOREACH(ctable, Symbol, {
        free_symbol(&elements[i]);
    });
    free_ctable(ctable);
}

Symbol* symbol_lookup(SymbolTable* const table, SymbolName* name) {
    CTable* ctable = (CTable*) table;
    CTableEntry* entry = ctable_find(ctable, (CTableKey*)name);
    if (entry == NULL) {
        return NULL;
    }
    Symbol* elements = (Symbol*) ctable->data.elements;
    return &elements[entry->vector_pos];
}

Symbol* symbol_lookup_str(SymbolTable* const table, const char* name, int length) {
    SymbolName symbol_name = create_symbol_name(name, length);
    return symbol_lookup(table, &symbol_name);
}

void symbol_insert(SymbolTable* const table, Symbol symbol) {
    CTABLE_SET((CTable*)table, symbol.name.key, symbol, Symbol);
}

void init_symbol_node(SymbolNode* const node) {
    init_symbol_table(&node->symbols);
    node->father = NULL;
    init_vector(&node->childs, sizeof(SymbolNode));
    node->next_node_to_visit = 0;
}

void free_symbol_node(SymbolNode* const node) {
    free_symbol_table(&node->symbols);
    SymbolNode* childs = VECTOR_AS_SYMBOL_NODE(&node->childs);
    for (uint32_t i = 0; i < node->childs.size; i++) {
        free_symbol_node(&childs[i]);
    }
    free_vector(&node->childs);
}

void symbol_node_reset(SymbolNode* const node) {
    node->next_node_to_visit = 0;
    SymbolNode* childs = VECTOR_AS_SYMBOL_NODE(&node->childs);
    for (uint32_t i = 0; i < node->childs.size; i++) {
        symbol_node_reset(&childs[i]);
    }
}

SymbolNode* symbol_node_add_child(SymbolNode* const node, SymbolNode* const child) {
    child->father = node;
    VECTOR_ADD_SYMBOL_NODE(&node->childs, *child);
    SymbolNode* childs = VECTOR_AS_SYMBOL_NODE(&node->childs);
    return &childs[node->childs.size - 1];
}

void init_scoped_symbol_table(ScopedSymbolTable* const table) {
    init_symbol_node(&table->global);
    table->current = &table->global;
}

void free_scoped_symbol_table(ScopedSymbolTable* const table) {
    free_symbol_node(&table->global);
    table->current = NULL;
}

void symbol_create_scope(ScopedSymbolTable* const table) {
    assert(table->current != NULL);
    SymbolNode child;
    init_symbol_node(&child);
    table->current = symbol_node_add_child(table->current, &child);
}

void symbol_end_scope(ScopedSymbolTable* const table) {
    assert(table->current != NULL);
    assert(table->current->father != NULL);
    table->current = table->current->father;
}

void symbol_start_scope(ScopedSymbolTable* const table) {
    assert(table->current != NULL);
    assert(table->current->next_node_to_visit < table->current->childs.size);
    table->current->next_node_to_visit++;
    SymbolNode* childs = VECTOR_AS_SYMBOL_NODE(&table->current->childs);
    table->current = &childs[table->current->next_node_to_visit - 1];
}

void symbol_reset_scopes(ScopedSymbolTable* const table) {
    symbol_node_reset(&table->global);
    table->current = &table->global;
}

bool test_all(Symbol* symbol) {
    return true;
}

bool test_only_functions(Symbol* symbol) {
    return symbol->kind == SYMBOL_FUNCTION;
}

static Symbol* scoped_symbol_lookup_levels_conditional(ScopedSymbolTable* const table, SymbolName* name, int levels, ExitCondition test_condition) {
    assert(table->current != NULL);
    SymbolNode* current = table->current;
    Symbol* symbol = NULL;
    while (current != NULL && levels >= 0) {
        symbol = symbol_lookup(&current->symbols, name);
        if (symbol != NULL && test_condition(symbol)) {
            return symbol;
        }
        current = current->father;
        levels--;
    }
    return NULL;
}

Symbol* scoped_symbol_lookup(ScopedSymbolTable* const table, SymbolName* name) {
    return scoped_symbol_lookup_levels_conditional(table, name, INT_MAX, test_all);
}

Symbol* scoped_symbol_lookup_str(ScopedSymbolTable* const table, const char* name, int length) {
    SymbolName symbol_name = create_symbol_name(name, length);
    return scoped_symbol_lookup(table, &symbol_name);
}

Symbol* scoped_symbol_lookup_function(ScopedSymbolTable* const table, SymbolName* name) {
    return scoped_symbol_lookup_levels_conditional(table, name, INT_MAX, test_only_functions);
}

Symbol* scoped_symbol_lookup_function_str(ScopedSymbolTable* const table, const char* name, int length) {
    SymbolName symbol_name = create_symbol_name(name, length);
    return scoped_symbol_lookup_function(table, &symbol_name);
}

Symbol* scoped_symbol_lookup_levels(ScopedSymbolTable* const table, SymbolName* name, int levels) {
    return scoped_symbol_lookup_levels_conditional(table, name, levels, test_all);
}

Symbol* scoped_symbol_lookup_levels_str(ScopedSymbolTable* const table, const char* name, int length, int levels) {
    SymbolName symbol_name = create_symbol_name(name, length);
    return scoped_symbol_lookup_levels(table, &symbol_name, levels);
}

void scoped_symbol_insert(ScopedSymbolTable* const table, Symbol entry) {
    assert(table->current != NULL);
    symbol_insert(&table->current->symbols, entry);
}

void scoped_symbol_upvalue(ScopedSymbolTable* const table, Symbol* fn, Symbol* var_upvalue) {
    assert(fn != NULL);
    assert(fn->kind == SYMBOL_FUNCTION);
    assert(var_upvalue != NULL);
    symbol_set_add(fn->function.upvalues, var_upvalue);
    symbol_set_add(var_upvalue->upvalue_fn_refs, fn);
}

void scoped_symbol_update_object_body(ScopedSymbolTable* const table, Symbol* obj) {
    obj->object.body = &table->current->symbols;
}

SymbolSet* create_symbol_set() {
    SymbolSet* set = (SymbolSet*) malloc(sizeof(SymbolSet));
    init_ctable((CTable*)set, sizeof(Symbol*));
    return set;
}

void free_symbol_set(SymbolSet* const set) {
    free_ctable((CTable*)set);
    free(set);
    // Dont free the table symbols. We dont own that.
}

void symbol_set_add(SymbolSet* const set, Symbol* symbol) {
    CTable* ctable = (CTable*) set;
    CTableEntry* existing = ctable_find(ctable, (CTableKey*)&symbol->name);
    if (existing != NULL) {
        return;
    }
    CTABLE_SET(ctable, symbol->name.key, symbol, Symbol*);
}

void init_upvalue_iterator(UpvalueIterator* const iterator, ScopedSymbolTable* table, int depth) {
    iterator->current = table->current;
    iterator->current_upvalue = 0;
    iterator->depth = depth;
}

Symbol* upvalue_iterator_next(UpvalueIterator* const iterator) {
    if (iterator->depth < 0) {
        return NULL;
    }
    for (;;) {
        int size = CTABLE_SIZE(iterator->current->symbols.table);
        if (iterator->current_upvalue >= size) {
            if (! find_next_scope_with_upvalues(iterator)) {
                return NULL;
            }
        }
        assert(CTABLE_SIZE(iterator->current->symbols.table) > 0);
        Symbol* symbols = CTABLE_AS(iterator->current->symbols.table, Symbol);
        Symbol* sym = &symbols[iterator->current_upvalue];
        iterator->current_upvalue++;
        if (SYMBOL_SET_SIZE(sym->upvalue_fn_refs) > 0) {
            return sym;
        }
    }
    return NULL;
}

static bool find_next_scope_with_upvalues(UpvalueIterator* const iterator) {
    while (iterator->current->father != NULL && iterator->depth >= 1) {
        iterator->depth--;
        iterator->current = iterator->current->father;
        iterator->current_upvalue = 0;

        int size = CTABLE_SIZE(iterator->current->symbols.table);
        if (size > 0) {
            return true;
        }
    }
    return false;
}
