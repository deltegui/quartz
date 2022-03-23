#include "string.h"
#include "vm.h"

static void insert_methods(ScopedSymbolTable* const table);

static Value string_length(int argc, Value* argv);

static ObjNative *length_fn = NULL;

void string_init() {
    NATIVE_CLASS_INIT(length_fn, "length", 6, string_length, {
        type_f = create_type_function();
        type_f->function.return_type = CREATE_TYPE_NUMBER();
    });
}

static Value string_length(int argc, Value* argv) {
    assert(argc == 1);
#define SELF argv[0]

    ObjString* arr = OBJ_AS_STRING(VALUE_AS_OBJ(SELF));
    return NUMBER_VALUE(arr->length);

#undef SELF
}

NativeClassStmt string_register(ScopedSymbolTable* const table) {
    return register_native_class(table, STRING_CLASS_NAME, STRING_CLASS_LENGTH, insert_methods);
}

static void insert_methods(ScopedSymbolTable* const table) {
    int constant_index = 0;
    NATIVE_INSERT_METHOD(table, length_fn, constant_index);
}

void string_push_props(ValueArray* props) {
    NATIVE_PUSH_PROP(props, length_fn);
}

