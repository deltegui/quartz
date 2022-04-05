#include "type.h"
#include <stdio.h>
#include <string.h>
#include "array.h"
#include "string.h"

// These variables are here to store only one instance
// of these types. Simple types only stores it's kind,
// so let repeating these inside the type pool
// eats resources and is useless.
static Type number_type;
static Type bool_type;
static Type nil_type;
static Type string_type;
static Type void_type;
static Type unknown_type;
static Type any_type;

// A type pool it's a place where types by value are
// stored. The rest of the interpreter just have
// references to the types that are stored here.
// It's implemented using a linked-list in which every
// node has an array. Storing just one type per node is
// inefficient because every new node is a system call, and
// that's expensive. A program may have the order of hundreds of
// custom types. We don't want hundreds of system calls.
// You could think reusing the Vector type defined in
// vector.h is a good idea. No, it's not. It'll automatically
// realloc memory, invalidating previous references to it and
// crashing the interpreter. Keep all that in mind before
// changing this data structure.
typedef struct s_pool_node {
    struct s_pool_node* next;
    uint32_t size;
    uint32_t capacity;
    Type types[];
} PoolNode;

static uint32_t last_capacity = 0;
static PoolNode* type_pool = NULL;
static PoolNode* current_node = NULL;

inline static uint32_t next_capacity();
static void free_pool_node(PoolNode* const node);
static void free_type(Type* const type);
static Type* type_pool_add(Type type);
static PoolNode* alloc_node();
static void type_alias_print(FILE* out, const Type* const type);
static void type_function_print(FILE* out, const Type* const type);
static void type_class_print(FILE* out, const Type* const type);
static void type_object_print(FILE* out, const Type* const type);
static void type_array_print(FILE* out, const Type* const type);
static bool fn_params_equals(FunctionType* first, FunctionType* second);
static bool type_function_equals(Type* first, Type* second);
static bool type_class_equals(Type* first, Type* second);

inline static uint32_t next_capacity() {
    last_capacity = ((last_capacity < 8) ? 8 : last_capacity * 2);
    return last_capacity;
}

void init_type_pool() {
    last_capacity = 0;
    type_pool = NULL;
    current_node = NULL;

    number_type.kind = TYPE_NUMBER;
    bool_type.kind = TYPE_BOOL;
    nil_type.kind = TYPE_NIL;
    string_type.kind = TYPE_STRING;
    void_type.kind = TYPE_VOID;
    unknown_type.kind = TYPE_UNKNOWN;
    any_type.kind = TYPE_ANY;
}

void free_type_pool() {
    PoolNode* current = type_pool;
    while (current != NULL) {
        PoolNode* next = current->next;
        free_pool_node(current);
        current = next;
    }
}

static void free_pool_node(PoolNode* const node) {
    for (uint32_t i = 0; i < node->size; i++) {
        free_type(&node->types[i]);
    }
    free(node);
}

static void free_type(Type* const type) {
    switch (type->kind) {
    case TYPE_FUNCTION: {
        free_vector(&type->function.param_types);
        break;
    }
    case TYPE_CLASS: {
        free(type->klass.identifier);
        break;
    }
    case TYPE_ALIAS: {
        free(type->alias.identifier);
        break;
    }
    case TYPE_ANY:
    case TYPE_OBJECT:
    case TYPE_NUMBER:
    case TYPE_BOOL:
    case TYPE_NIL:
    case TYPE_STRING:
    case TYPE_VOID:
    case TYPE_UNKNOWN:
    case TYPE_ARRAY:
        break;
    }
}

static Type* type_pool_add(Type type) {
    if (type_pool == NULL) {
        type_pool = alloc_node();
        current_node = type_pool;
    }
    if (current_node->capacity <= current_node->size + 1) {
        current_node->next = alloc_node();
        current_node = current_node->next;
    }
    Type* bucket = &current_node->types[current_node->size++];
    *bucket = type;
    return bucket;
}

static PoolNode* alloc_node() {
    uint32_t cap = next_capacity();
    PoolNode* node = (PoolNode*) malloc(sizeof(PoolNode) + sizeof(Type) * cap);
    node->capacity = cap;
    node->size = 0;
    node->next = NULL;
    return node;
}

Type* create_type_simple(TypeKind kind) {
    switch (kind) {
    case TYPE_NUMBER: return &number_type;
    case TYPE_BOOL: return &bool_type;
    case TYPE_NIL: return &nil_type;
    case TYPE_STRING: return &string_type;
    case TYPE_VOID: return &void_type;
    case TYPE_UNKNOWN: return &unknown_type;
    case TYPE_ANY: return &any_type;
    default:
        // If we reach this assert, you forget to put a case in the swith
        // or you are calling the wrong function.
        assert(false);
        return &unknown_type;
    }
}

Type* create_type_function() {
    Type type;
    type.kind = TYPE_FUNCTION;
    type.function.return_type = CREATE_TYPE_VOID();
    init_vector(&type.function.param_types, sizeof(Type*));
    return type_pool_add(type);
}

Type* create_type_alias(const char* identifier, int length, Type* original) {
    // So, the token pool outlives other compiler data structures like the original
    // code buffer, the AST or the Symbol Table. Knowing that, the alias identifier
    // must be copied.
    Type type;
    type.kind = TYPE_ALIAS;
    type.alias.def = original;
    type.alias.identifier = (char*) malloc(sizeof(char) * (length + 1));
    memcpy(type.alias.identifier, identifier, length);
    type.alias.identifier[length] = '\0';
    return type_pool_add(type);
}

Type* create_type_class(const char* identifier, int length) {
    Type type;
    type.kind = TYPE_CLASS;
    type.klass.identifier = (char*) malloc(sizeof(char) * (length + 1));
    type.klass.length = length;
    memcpy(type.klass.identifier, identifier, length);
    type.klass.identifier[length] = '\0';
    return type_pool_add(type);
}

Type* create_type_object(Type* klass) {
    Type type;
    type.kind = TYPE_OBJECT;
    type.object.klass = klass;
    return type_pool_add(type);
}

Type* create_type_array(Type* inner) {
    Type type;
    type.kind = TYPE_ARRAY;
    type.array.inner = inner;
    return type_pool_add(type);
}

Type* simple_type_from_token_kind(TokenKind kind) {
    switch (kind) {
    case TOKEN_TYPE_NUMBER: return CREATE_TYPE_NUMBER();
    case TOKEN_TYPE_STRING: return CREATE_TYPE_STRING();
    case TOKEN_TYPE_BOOL: return CREATE_TYPE_BOOL();
    case TOKEN_TYPE_NIL: return CREATE_TYPE_NIL();
    case TOKEN_TYPE_VOID: return CREATE_TYPE_VOID();
    case TOKEN_TYPE_ANY: return CREATE_TYPE_ANY();
    default: return CREATE_TYPE_UNKNOWN();
    }
}

void type_fprint(FILE* out, const Type* const type) {
    switch (type->kind) {
    case TYPE_OBJECT: type_object_print(out, type); break;
    case TYPE_ALIAS: type_alias_print(out, type); break;
    case TYPE_NUMBER: fprintf(out, "Number"); break;
    case TYPE_BOOL: fprintf(out, "Bool"); break;
    case TYPE_NIL: fprintf(out, "Nil"); break;
    case TYPE_STRING: fprintf(out, "String"); break;
    case TYPE_FUNCTION: type_function_print(out, type); break;
    case TYPE_CLASS: type_class_print(out, type); break;
    case TYPE_VOID: fprintf(out, "Void"); break;
    case TYPE_UNKNOWN: fprintf(out, "Unknown"); break;
    case TYPE_ANY: fprintf(out, "Any"); break;
    case TYPE_ARRAY: type_array_print(out, type); break;
    }
}

static void type_array_print(FILE* out, const Type* const type) {
    assert(type->kind == TYPE_ARRAY);
    fprintf(out,"[]");
    type_fprint(out, type->array.inner);
}

static void type_alias_print(FILE* out, const Type* const type) {
    assert(type->kind == TYPE_ALIAS);
    fprintf(
        out,
        "Alias: '%s' = ",
        type->alias.identifier);
    type_fprint(out, type->alias.def);
}

static void type_function_print(FILE* out, const Type* const type) {
    assert(type->kind == TYPE_FUNCTION);
    Type** params = VECTOR_AS_TYPES(&type->function.param_types);
    uint32_t size = type->function.param_types.size;
    fprintf(out, "(");
    for (uint32_t i = 0; i < size; i++) {
        type_fprint(out, params[i]);
        if (i < size - 1) {
            fprintf(out, ", ");
        }
    }
    fprintf(out, "): ");
    type_fprint(out, type->function.return_type);
}

static void type_class_print(FILE* out, const Type* const type) {
    assert(type->kind == TYPE_CLASS);
    fprintf(out, "Class<%.*s>", type->klass.length, type->klass.identifier);
}

static void type_object_print(FILE* out, const Type* const type) {
    assert(type->kind == TYPE_OBJECT);
    fprintf(out, "Instance of ");
    type_class_print(out, type->object.klass);
}

bool type_equals(Type* first, Type* second) {
    assert(first != NULL && second != NULL);
    if (first == second) {
        return true;
    }
    first = RESOLVE_IF_TYPEALIAS(first);
    second = RESOLVE_IF_TYPEALIAS(second);
    if (first->kind != second->kind) {
        return false;
    }
    if (first->kind == TYPE_FUNCTION) {
        return type_function_equals(first, second);
    }
    if (first->kind == TYPE_CLASS) {
        return type_class_equals(first, second);
    }
    if (first->kind == TYPE_ARRAY) {
        return type_equals(first->array.inner, second->array.inner);
    }
    return true;
}

static bool type_function_equals(Type* first, Type* second) {
    assert(first->kind == TYPE_FUNCTION && second->kind == TYPE_FUNCTION);
    if (! fn_params_equals(&first->function, &second->function)) {
        return false;
    }
    return first->function.return_type == second->function.return_type;
}

static bool fn_params_equals(FunctionType* first, FunctionType* second) {
    assert(first != NULL && second != NULL);
    if (first->param_types.size != second->param_types.size) {
        return false;
    }
    Type** first_types = VECTOR_AS_TYPES(&first->param_types);
    Type** second_types = VECTOR_AS_TYPES(&second->param_types);
    for (uint32_t i = 0; i < first->param_types.size; i++) {
        bool param_equals = type_equals(first_types[i], second_types[i]);
        if (! param_equals) {
            return false;
        }
    }
    return true;
}

static bool type_class_equals(Type* first, Type* second) {
    assert(first->kind == TOKEN_CLASS && second->kind == TOKEN_CLASS);
    if (first->klass.length != second->klass.length) {
        return false;
    }
    return memcmp(
        first->klass.identifier,
        second->klass.identifier,
        first->klass.length) == 0;
}

Type* type_cast(Type* from, Type* to) {
    if (TYPE_IS_ASSIGNABLE(to, from)) {
        return from;
    }
    if (TYPE_IS_BOOL(to) || TYPE_IS_ANY(from)) {
        return to;
    }
    return NULL;
}

// TODO refactor this
const char* type_get_class_name(Type* any_type) {
    assert(TYPE_IS_OBJECT(any_type) || TYPE_IS_ARRAY(any_type) || TYPE_IS_STRING(any_type));
    if (TYPE_IS_ARRAY(any_type)) {
        return ARRAY_CLASS_NAME;
    }
    if (TYPE_IS_STRING(any_type)) {
        return STRING_CLASS_NAME;
    }
    return TYPE_OBJECT_CLASS_NAME(any_type);
}

int type_get_class_length(Type* any_type) {
    assert(TYPE_IS_OBJECT(any_type) || TYPE_IS_ARRAY(any_type) || TYPE_IS_STRING(any_type));
    if (TYPE_IS_ARRAY(any_type)) {
        return ARRAY_CLASS_LENGTH;
    }
    if (TYPE_IS_STRING(any_type)) {
        return STRING_CLASS_LENGTH;
    }
    return TYPE_OBJECT_CLASS_LENGTH(any_type);
}
