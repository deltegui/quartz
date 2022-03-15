#include "array.h"
#include "vm.h"

static Value array_push(int argc, Value* argv);
static Value array_get(int argc, Value* argv);

static Value array_push(int argc, Value* argv) {
    assert(argc == 2);
    ObjArray* arr = OBJ_AS_ARRAY(VALUE_AS_OBJ(argv[0]));
    valuearray_write(&arr->elements, argv[1]);
    return NIL_VALUE();
}

static Value array_get(int argc, Value* argv) {
    assert(argc == 2);
    ObjArray* arr = OBJ_AS_ARRAY(VALUE_AS_OBJ(argv[0]));
    double d = VALUE_AS_NUMBER(argv[1]);
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
}

static Value array_set(int argc, Value* argv) {
    assert(argc == 3);
    ObjArray* arr = OBJ_AS_ARRAY(VALUE_AS_OBJ(argv[0]));
    double d = VALUE_AS_NUMBER(argv[1]);
    int index = (int) d;
    if (index < 0) {
        runtime_error("Indexing array with negative number");
        return argv[2];
    }
    if (index >= arr->elements.size) {
        runtime_error("Array index out of limits");
        return argv[2];
    }
    arr->elements.values[index] = argv[2];
    return argv[2];
}

NativeImport array_get_import() {
    Type* generic_array = create_type_array(CREATE_TYPE_ANY());

    Type* push_type = create_type_function();
    VECTOR_ADD_TYPE(&push_type->function.param_types, generic_array);
    VECTOR_ADD_TYPE(&push_type->function.param_types, CREATE_TYPE_ANY());
    push_type->function.return_type = CREATE_TYPE_VOID();

    NativeFunction arr_push = (NativeFunction) {
        .name = "array_push",
        .length = 10,
        .function = array_push,
        .type = push_type,
    };

    Type* get_type = create_type_function();
    VECTOR_ADD_TYPE(&get_type->function.param_types, generic_array);
    VECTOR_ADD_TYPE(&get_type->function.param_types, CREATE_TYPE_NUMBER());
    get_type->function.return_type = CREATE_TYPE_ANY();

    NativeFunction arr_get = (NativeFunction) {
        .name = "array_get",
        .length = 9,
        .function = array_get,
        .type = get_type,
    };

    Type* set_type = create_type_function();
    VECTOR_ADD_TYPE(&set_type->function.param_types, generic_array);
    VECTOR_ADD_TYPE(&set_type->function.param_types, CREATE_TYPE_NUMBER());
    VECTOR_ADD_TYPE(&set_type->function.param_types, CREATE_TYPE_ANY());
    set_type->function.return_type = CREATE_TYPE_VOID();

    NativeFunction arr_set = (NativeFunction) {
        .name = "array_set",
        .length = 9,
        .function = array_set,
        .type = set_type,
    };

#define FN_LENGTH 3
    static NativeFunction functions[FN_LENGTH];
    functions[0] = arr_push;
    functions[1] = arr_get;
    functions[2] = arr_set;

    NativeImport arrays_import = (NativeImport) {
        .name = "arrays",
        .length = 6,
        .functions = functions,
        .functions_length = FN_LENGTH,
    };
#undef FN_LENGTH

    return arrays_import;
}
