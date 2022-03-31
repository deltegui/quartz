#include "array.h"
#include "vm.h"
#include "object.h"

static void insert_methods(ScopedSymbolTable* const table);

static Value array_push(int argc, Value* argv);
static Value array_get(int argc, Value* argv);
static Value array_set(int argc, Value* argv);
static Value array_length(int argc, Value* argv);

static ObjNative *push_fn = NULL,
                 *get_fn = NULL,
                 *set_fn = NULL,
                 *length_fn = NULL;

void init_array() {
    NATIVE_CLASS_INIT(push_fn, "push", 4, array_push, {
        type_f = create_type_function();
        VECTOR_ADD_TYPE(&type_f->function.param_types, CREATE_TYPE_ANY());
        type_f->function.return_type = CREATE_TYPE_VOID();
    });

    NATIVE_CLASS_INIT(get_fn, "get", 3, array_get, {
        type_f = create_type_function();
        VECTOR_ADD_TYPE(&type_f->function.param_types, CREATE_TYPE_NUMBER());
        type_f->function.return_type = CREATE_TYPE_ANY();
    });

    NATIVE_CLASS_INIT(set_fn, "set", 3, array_set, {
        type_f = create_type_function();
        VECTOR_ADD_TYPE(&type_f->function.param_types, CREATE_TYPE_NUMBER());
        VECTOR_ADD_TYPE(&type_f->function.param_types, CREATE_TYPE_ANY());
        type_f->function.return_type = CREATE_TYPE_VOID();
    });

    NATIVE_CLASS_INIT(length_fn, "length", 6, array_length, {
        type_f = create_type_function();
        type_f->function.return_type = CREATE_TYPE_NUMBER();
    });
}

static Value array_push(int argc, Value* argv) {
    assert(argc == 2);
#define SELF argv[1]
#define VALUE argv[0]

    ObjArray* arr = OBJ_AS_ARRAY(VALUE_AS_OBJ(SELF));
    valuearray_write(&arr->elements, VALUE);
    return NIL_VALUE();

#undef VALUE
#undef SELF
}

static Value array_get(int argc, Value* argv) {
    assert(argc == 2);
#define SELF argv[1]
#define INDEX argv[0]

    ObjArray* arr = OBJ_AS_ARRAY(VALUE_AS_OBJ(SELF));
    double d = VALUE_AS_NUMBER(INDEX);
    int index = (int) d;
    if (index < 0) {
        runtime_error("Indexing array with negative number");
        return NIL_VALUE();
    }
    if (index >= arr->elements.size) {
        runtime_error("Array index out of limits");
        return NIL_VALUE();
    }
    return arr->elements.values[index];

#undef INDEX
#undef SELF
}

static Value array_set(int argc, Value* argv) {
    assert(argc == 3);
#define SELF argv[2]
#define INDEX argv[0]
#define VALUE argv[1]

    ObjArray* arr = OBJ_AS_ARRAY(VALUE_AS_OBJ(SELF));
    double d = VALUE_AS_NUMBER(INDEX);
    int index = (int) d;
    if (index < 0) {
        runtime_error("Indexing array with negative number");
        return VALUE;
    }
    if (index >= arr->elements.size) {
        runtime_error("Array index out of limits");
        return VALUE;
    }
    arr->elements.values[index] = VALUE;
    return VALUE;

#undef SELF
#undef INDEX
#undef VALUE
}

static Value array_length(int argc, Value* argv) {
    assert(argc == 1);
#define SELF argv[0]

    ObjArray* arr = OBJ_AS_ARRAY(VALUE_AS_OBJ(SELF));
    return NUMBER_VALUE(arr->elements.size);

#undef SELF
}

NativeClassStmt array_register(ScopedSymbolTable* const table) {
    return register_native_class(table, ARRAY_CLASS_NAME, ARRAY_CLASS_LENGTH, insert_methods);
}

static void insert_methods(ScopedSymbolTable* const table) {
    int constant_index = 0;
    NATIVE_INSERT_METHOD(table, push_fn, constant_index);
    NATIVE_INSERT_METHOD(table, get_fn, constant_index);
    NATIVE_INSERT_METHOD(table, set_fn, constant_index);
    NATIVE_INSERT_METHOD(table, length_fn, constant_index);
}

void array_push_props(ValueArray* props) {
    NATIVE_PUSH_PROP(props, push_fn);
    NATIVE_PUSH_PROP(props, get_fn);
    NATIVE_PUSH_PROP(props, set_fn);
    NATIVE_PUSH_PROP(props, length_fn);
}

void mark_array() {
    mark_object((Obj*) push_fn);
    mark_object((Obj*) get_fn);
    mark_object((Obj*) set_fn);
    mark_object((Obj*) length_fn);
}

