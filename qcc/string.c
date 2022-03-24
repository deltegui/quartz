#include "string.h"
#include "vm.h"

static void insert_methods(ScopedSymbolTable* const table);

static Value string_length(int argc, Value* argv);
static Value string_get_char(int argc, Value* argv);

static ObjNative *length_fn = NULL,
                 *get_char_fn = NULL;

void string_init() {
    NATIVE_CLASS_INIT(length_fn, "length", 6, string_length, {
        type_f = create_type_function();
        type_f->function.return_type = CREATE_TYPE_NUMBER();
    });

    NATIVE_CLASS_INIT(get_char_fn, "get_char", 8, string_get_char, {
        type_f = create_type_function();
        VECTOR_ADD_TYPE(&type_f->function.param_types, CREATE_TYPE_NUMBER());
        type_f->function.return_type = CREATE_TYPE_STRING();
    });
}

static Value string_length(int argc, Value* argv) {
    assert(argc == 1);
#define SELF argv[0]

    ObjString* str = OBJ_AS_STRING(VALUE_AS_OBJ(SELF));
    return NUMBER_VALUE(str->length);

#undef SELF
}

static Value string_get_char(int argc, Value* argv) {
    assert(argc == 2);
#define SELF argv[1]
#define INDEX argv[0]

    ObjString* str = OBJ_AS_STRING(VALUE_AS_OBJ(SELF));
    int index = (int) VALUE_AS_NUMBER(INDEX);
    if (index < 0 || index >= str->length) {
        runtime_error("index out of string bounds");
        return OBJ_VALUE(copy_string("", 0), CREATE_TYPE_STRING());
    }
    char c = str->chars[index];
    return OBJ_VALUE(copy_string(&c, 1), CREATE_TYPE_STRING());

#undef INDEX
#undef SELF
}

NativeClassStmt string_register(ScopedSymbolTable* const table) {
    return register_native_class(table, STRING_CLASS_NAME, STRING_CLASS_LENGTH, insert_methods);
}

static void insert_methods(ScopedSymbolTable* const table) {
    int constant_index = 0;
    NATIVE_INSERT_METHOD(table, length_fn, constant_index);
    NATIVE_INSERT_METHOD(table, get_char_fn, constant_index);
}

void string_push_props(ValueArray* props) {
    NATIVE_PUSH_PROP(props, length_fn);
    NATIVE_PUSH_PROP(props, get_char_fn);
}
