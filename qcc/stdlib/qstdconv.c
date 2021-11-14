#include "qstdconv.h"
#include <stdio.h>
#include "../values.h"
#include "../common.h"
#include "../object.h"
#include "../native.h"

static Value stdconv_ntos(int argc, Value* argv);
static Value stdconv_btos(int argc, Value* argv);

#define DEFINE_TYPE(name, param_type)\
    Type* name = create_type_function();\
    VECTOR_ADD_TYPE(&name->function->param_types, param_type);\
    name->function->return_type = CREATE_TYPE_STRING()

void register_stdconv(CTable* table) {
    DEFINE_TYPE(ntos_type, CREATE_TYPE_NUMBER());
    NativeFunction ntos = (NativeFunction) {
        .name = "ntos",
        .length = 4,
        .function = stdconv_ntos,
        .type = ntos_type,
    };

    DEFINE_TYPE(btos_type, CREATE_TYPE_BOOL());
    NativeFunction btos = (NativeFunction) {
        .name = "btos",
        .length = 4,
        .function = stdconv_btos,
        .type = btos_type,
    };

#define FN_LENGTH 2
    static NativeFunction functions[FN_LENGTH];
    functions[0] = ntos;
    functions[1] = btos;

    NativeImport stdconv_import = (NativeImport) {
        .name = "stdconv",
        .length = 7,
        .functions = functions,
        .functions_length = FN_LENGTH,
    };
#undef FN_LENGTH
    CTABLE_SET(
        table,
        create_ctable_key(stdconv_import.name, stdconv_import.length),
        stdconv_import,
        NativeImport);
}

static Value stdconv_ntos(int argc, Value* argv) {
    assert(argc == 1);
    double number = VALUE_AS_NUMBER(argv[0]);
    char buffer[32];
    int length = sprintf(buffer, "%g", number);
    ObjString* str = copy_string(buffer, length);
    return OBJ_VALUE(str, CREATE_TYPE_STRING());
}

static Value stdconv_btos(int argc, Value* argv) {
    assert(argc == 1);
    bool b = VALUE_AS_BOOL(argv[0]);
    ObjString* str = NULL;
    if (b) {
        str = copy_string("true", 4);
    } else {
        str = copy_string("false", 5);
    }
    return OBJ_VALUE(str, CREATE_TYPE_STRING());
}
