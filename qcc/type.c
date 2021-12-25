#include "type.h"
#include <stdio.h>
#include <string.h>

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

uint32_t last_capacity = 0;
PoolNode* type_pool = NULL;
PoolNode* current_node = NULL;

inline static uint32_t next_capacity();
static void free_pool_node(PoolNode* const node);
static void free_type(Type* const type);
static Type* type_pool_add(Type type);
static PoolNode* alloc_node();
static void type_alias_print(FILE* out, const Type* const type);
static void type_function_print(FILE* out, const Type* const type);
static void type_object_print(FILE* out, const Type* const type);
static bool fn_params_equals(FunctionType* first, FunctionType* second);
static bool type_function_equals(Type* first, Type* second);
static bool type_object_equals(Type* first, Type* second);

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
    case TYPE_OBJECT: {
        free(type->object);
        break;
    }
    case TYPE_FUNCTION: {
        free_vector(&type->function->param_types);
        free(type->function);
        break;
    }
    case TYPE_ALIAS: {
        free(type->alias);
        break;
    }
    default:
        // Simple types do not have anything to free
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
    default:
        // If we reach this assert, you forget to put a case in the swith
        // or you are calling the wrong function.
        assert(false);
        return &unknown_type;
    }
}

Type* create_type_function() {
    FunctionType* fn_type = (FunctionType*) malloc(sizeof(FunctionType));
    init_vector(&fn_type->param_types, sizeof(Type*));
    fn_type->return_type = CREATE_TYPE_VOID();
    Type type = (Type) {
        .kind = TYPE_FUNCTION,
        .function = fn_type,
    };
    return type_pool_add(type);
}

Type* create_type_alias(const char* identifier, int length, Type* original) {
    // So, the token pool outlives other compiler data structures like the original
    // code buffer, the AST or the Symbol Table. Knowing that, the alias identifier
    // must be copied.
    AliasType* alias = (AliasType*) malloc(sizeof(AliasType) + (sizeof(char) * length + 1));
    memcpy(alias->identifier, identifier, length);
    alias->identifier[length] = '\0';
    alias->def = original;
    Type type = (Type) {
        .kind = TYPE_ALIAS,
        .alias = alias,
    };
    return type_pool_add(type);
}

Type* create_type_object(const char* identifier, int length) {
    ObjectType* objt = (ObjectType*) malloc(sizeof(ObjectType) + (sizeof(char) * length + 1));
    memcpy(objt->identifier, identifier, length);
    objt->identifier[length] = '\0';
    objt->length = length;
    Type type = (Type) {
        .kind = TYPE_OBJECT,
        .object = objt,
    };
    return type_pool_add(type);
}

Type* simple_type_from_token_kind(TokenKind kind) {
    switch (kind) {
    case TOKEN_TYPE_NUMBER: return CREATE_TYPE_NUMBER();
    case TOKEN_TYPE_STRING: return CREATE_TYPE_STRING();
    case TOKEN_TYPE_BOOL: return CREATE_TYPE_BOOL();
    case TOKEN_TYPE_NIL: return CREATE_TYPE_NIL();
    case TOKEN_TYPE_VOID: return CREATE_TYPE_VOID();
    default: return CREATE_TYPE_UNKNOWN();
    }
}

void type_fprint(FILE* out, const Type* const type) {
    switch (type->kind) {
    case TYPE_ALIAS: type_alias_print(out, type); break;
    case TYPE_NUMBER: fprintf(out, "Number"); break;
    case TYPE_BOOL: fprintf(out, "Bool"); break;
    case TYPE_NIL: fprintf(out, "Nil"); break;
    case TYPE_STRING: fprintf(out, "String"); break;
    case TYPE_FUNCTION: type_function_print(out, type); break;
    case TYPE_OBJECT: type_object_print(out, type); break;
    case TYPE_VOID: fprintf(out, "Void"); break;
    case TYPE_UNKNOWN: fprintf(out, "Unknown"); break;
    }
}

static void type_alias_print(FILE* out, const Type* const type) {
    assert(type->kind == TYPE_ALIAS);
    fprintf(
        out,
        "Alias: '%s' = ",
        type->alias->identifier);
    type_fprint(out, type->alias->def);
}

static void type_function_print(FILE* out, const Type* const type) {
    assert(type->kind == TYPE_FUNCTION);
    Type** params = VECTOR_AS_TYPES(&type->function->param_types);
    uint32_t size = type->function->param_types.size;
    fprintf(out, "(");
    for (uint32_t i = 0; i < size; i++) {
        type_fprint(out, params[i]);
        if (i < size - 1) {
            fprintf(out, ", ");
        }
    }
    fprintf(out, "): ");
    type_fprint(out, type->function->return_type);
}

static void type_object_print(FILE* out, const Type* const type) {
    assert(type->kind == TYPE_OBJECT);
    fprintf(out, "Object<%.*s>", type->object->length, type->object->identifier);
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
    if (first->kind == TYPE_OBJECT) {
        return type_object_equals(first, second);
    }
    return true;
}

static bool type_function_equals(Type* first, Type* second) {
    assert(first->function != NULL && second->function != NULL);
    if (! fn_params_equals(first->function, second->function)) {
        return false;
    }
    return first->function->return_type == second->function->return_type;
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

static bool type_object_equals(Type* first, Type* second) {
    assert(first->object != NULL && second->object != NULL);
    if (first->object->length != second->object->length) {
        return false;
    }
    return memcmp(
        first->object->identifier,
        second->object->identifier,
        first->object->length) == 0;
}
