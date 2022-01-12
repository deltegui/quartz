#include "qstdio.h"
#include <stdio.h>
#include <string.h>
#include "../values.h"
#include "../object.h"
#include "../common.h"
#include "../native.h"

// TODO print for numbers and so on?
static Value stdio_println(int argc, Value* argv);
static Value stdio_print(int argc, Value* argv);
static Value stdio_readstr(int argc, Value* argv);

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

    Type* readstr_type = create_type_function();
    readstr_type->function->return_type = CREATE_TYPE_STRING();

    NativeFunction readstr = (NativeFunction) {
        .name = "readstr",
        .length = 7,
        .function = stdio_readstr,
        .type = readstr_type,
    };

#define FN_LENGTH 3
    static NativeFunction functions[FN_LENGTH];
    functions[0] = println;
    functions[1] = print;
    functions[2] = readstr;

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

static Value stdio_readstr(int argc, Value* argv) {
    char* buffer;
    scanf("%ms", &buffer);
    ObjString* str = copy_string(buffer, strlen(buffer));
    free(buffer);
    return OBJ_VALUE(str, CREATE_TYPE_STRING());
}

// TODO delete this
/*
static Value stdio_readstr(int argc, Value* argv) {
    Vector buffer;
    init_vector(&buffer, sizeof(char));
    char c;
    while ((c = getchar()) != '\n' && c != EOF) {
        VECTOR_ADD(&buffer, c, char);
    }
    VECTOR_ADD(&buffer, '\0', char);
    printf("Esto es leido desde la mierda esta: %s\n", (char*)buffer.elements);
    ObjString* str = copy_string(buffer.elements, buffer.size);
    free_vector(&buffer);
    return OBJ_VALUE(str, CREATE_TYPE_STRING());
}
*/

