#include "qstdio.h"
#include <stdio.h>
#include "../values.h"
#include "../object.h"
#include "../common.h"
#include "../native.h"

static Value stdio_println(int argc, Value* argv);
static Value stdio_print(int argc, Value* argv);

void register_stdio(CTable* table) {
    Type* print_type = create_type_function();
    VECTOR_ADD_TYPE(&print_type->function->param_types, CREATE_TYPE_STRING());
    print_type->function->return_type = CREATE_TYPE_VOID();

    NativeFunction println = (NativeFunction) {
        .name = "println",
        .length = 7,
        .function = stdio_println,
        .type = print_type,
    };

    NativeFunction print = (NativeFunction) {
        .name = "print",
        .length = 5,
        .function = stdio_print,
        .type = print_type,
    };

#define FN_LENGTH 2
    NativeFunction functions[FN_LENGTH] = {
        println,
        print,
    };

    NativeImport stdio_import = (NativeImport) {
        .name = "stdio",
        .length = 5,
        .functions = functions,
        .functions_length = FN_LENGTH,
    };
#undef FN_LENGTH
    CTABLE_SET(
        table,
        create_ctable_key(stdio_import.name, stdio_import.length),
        stdio_import,
        NativeImport);
}

static Value stdio_println(int argc, Value* argv) {
    assert(argc == 1);
    char* str = OBJ_AS_CSTRING(VALUE_AS_OBJ(argv[0]));
    printf("%s\n", str);
    return NIL_VALUE();
}

static Value stdio_print(int argc, Value* argv) {
    assert(argc == 1);
    char* str = OBJ_AS_CSTRING(VALUE_AS_OBJ(argv[0]));
    printf("%s", str);
    return NIL_VALUE();
}