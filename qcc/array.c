#include "array.h"
#include "vm.h"

static Value array_push(int argc, Value* argv);
static Value array_get(int argc, Value* argv);
static Value array_set(int argc, Value* argv);
static Value array_length(int argc, Value* argv);

static void register_props(ScopedSymbolTable* const table);

static Type *push_type = NULL,
            *get_type = NULL,
            *set_type = NULL,
            *length_type = NULL;

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
    SymbolName name = create_symbol_name(ARRAY_CLASS_NAME, ARRAY_CLASS_LENGTH);
    Symbol symbol = create_symbol(
        name,
        0,
        0,
        CREATE_TYPE_UNKNOWN());
    symbol.kind = SYMBOL_CLASS;
    scoped_symbol_insert(table, symbol);
    Symbol* inserted = scoped_symbol_lookup(table, &name);

    symbol_create_scope(table);
    scoped_symbol_update_class_body(table, inserted);
    register_props(table);
    symbol_end_scope(table);

    NativeClassStmt native;
    native.name = ARRAY_CLASS_NAME;
    native.length = ARRAY_CLASS_LENGTH;
    return native;
}

static void register_props(ScopedSymbolTable* const table) {
    int constant_index = 0;

#define INSERT_METHOD(name, len, type) do{\
    Symbol sym = create_symbol(create_symbol_name(name, len), 0, 0, type);\
    sym.visibility = SYMBOL_VISIBILITY_PUBLIC;\
    sym.constant_index = constant_index++;\
    scoped_symbol_insert(table, sym);\
} while (false)

#define SET_TYPE(type, ...) do {\
    if (type == NULL) {\
        __VA_ARGS__\
    }\
} while (false)


    SET_TYPE(push_type, {
        push_type = create_type_function();
        VECTOR_ADD_TYPE(&push_type->function.param_types, CREATE_TYPE_ANY());
        push_type->function.return_type = CREATE_TYPE_VOID();
    });
    INSERT_METHOD("push", 4, push_type);

    SET_TYPE(get_type, {
        get_type = create_type_function();
        VECTOR_ADD_TYPE(&get_type->function.param_types, CREATE_TYPE_NUMBER());
        get_type->function.return_type = CREATE_TYPE_ANY();
    });
    INSERT_METHOD("get", 3, get_type);

    SET_TYPE(set_type, {
        set_type = create_type_function();
        VECTOR_ADD_TYPE(&set_type->function.param_types, CREATE_TYPE_NUMBER());
        VECTOR_ADD_TYPE(&set_type->function.param_types, CREATE_TYPE_ANY());
        set_type->function.return_type = CREATE_TYPE_VOID();
    });
    INSERT_METHOD("set", 3, set_type);

    SET_TYPE(length_type, {
        length_type = create_type_function();
        length_type->function.return_type = CREATE_TYPE_NUMBER();
    });
    INSERT_METHOD("length", 6, length_type);

#undef SET_TYPE
#undef INSERT_METHOD
}

void array_push_props(ValueArray* props) {
#define WRITE(name_f, len_f, fn, type_f) do {\
    ObjNative* obj = new_native(name_f, len_f, fn, type_f);\
    valuearray_write(props, OBJ_VALUE(obj, type_f));\
} while (false)

    WRITE("push", 5, array_push, push_type);
    WRITE("get", 3, array_get, get_type);
    WRITE("set", 3, array_set, set_type);
    WRITE("length", 6, array_length, length_type);

#undef WRITE
}

