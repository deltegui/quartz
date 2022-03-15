#include "qstdconv.h"
#include <stdio.h>
#include <stdlib.h>
#include "../values.h"
#include "../common.h"
#include "../object.h"
#include "../native.h"

static Value stdconv_ntos(int argc, Value* argv);
static Value stdconv_btos(int argc, Value* argv);
static Value stdconv_sum(int argc, Value* argv);
static Value stdconv_ston(int argc, Value* argv);
static Value stdconv_typeof(int argc, Value* argv);

#define DEFINE_TYPE(name, param_type)\
    Type* name = create_type_function();\
    VECTOR_ADD_TYPE(&name->function.param_types, param_type);\
    name->function.return_type = CREATE_TYPE_STRING()

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

    Type* sum_type = create_type_function();
    VECTOR_ADD_TYPE(&sum_type->function.param_types, CREATE_TYPE_NUMBER());
    VECTOR_ADD_TYPE(&sum_type->function.param_types, CREATE_TYPE_NUMBER());
    VECTOR_ADD_TYPE(&sum_type->function.param_types, CREATE_TYPE_BOOL());
    sum_type->function.return_type = CREATE_TYPE_VOID();
    NativeFunction sum = (NativeFunction) {
        .name = "__t_sum",
        .length = 7,
        .function = stdconv_sum,
        .type = sum_type,
    };

    Type* typeof_type = create_type_function();
    VECTOR_ADD_TYPE(&typeof_type->function.param_types, CREATE_TYPE_ANY());
    typeof_type->function.return_type = CREATE_TYPE_VOID();
    NativeFunction typeof_ = (NativeFunction) {
        .name = "typeof",
        .length = 6,
        .function = stdconv_typeof,
        .type = typeof_type,
    };

    Type* ston_type = create_type_function();
    VECTOR_ADD_TYPE(&ston_type->function.param_types, CREATE_TYPE_STRING());
    TYPE_FN_RETURN(ston_type) = CREATE_TYPE_NUMBER();
    NativeFunction ston = (NativeFunction) {
        .name = "ston",
        .length = 4,
        .function = stdconv_ston,
        .type = ston_type,
    };

#define FN_LENGTH 5
    static NativeFunction functions[FN_LENGTH];
    functions[0] = ntos;
    functions[1] = btos;
    functions[2] = sum;
    functions[3] = ston;
    functions[4] = typeof_;

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

static Value stdconv_sum(int argc, Value* argv) {
    assert(argc == 3);
    double a = VALUE_AS_NUMBER(argv[0]);
    double b = VALUE_AS_NUMBER(argv[1]);
    bool c = VALUE_AS_BOOL(argv[2]);
    printf("SUM(%f, %f, %d) -> %f\n", a, b , c, a + b);
    return NIL_VALUE();
}

static Value stdconv_ston(int argc, Value* argv) {
    assert(argc == 1);
    char* str = OBJ_AS_CSTRING(VALUE_AS_OBJ(argv[0]));
    double result = strtod(str, NULL);
    return NUMBER_VALUE(result);
}

static Value stdconv_typeof(int argc, Value* argv) {
    assert(argc == 1);
    type_fprint(stdout, argv[0].type);
    printf("\n");
    return NIL_VALUE();
}
