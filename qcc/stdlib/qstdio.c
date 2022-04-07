#include "qstdio.h"
#include <stdio.h>
#include <string.h>
#include "../values.h"
#include "../object.h"
#include "../common.h"
#include "../native.h"
#include "../vm.h"

#if defined(WIN32) || defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

static Value stdio_println(int argc, Value* argv);
static Value stdio_print(int argc, Value* argv);
static Value stdio_readstr(int argc, Value* argv);
static Value stdio_read_stdin(int argc, Value* argv);

static bool ensure_is_tty();

void register_stdio(CTable* table) {
    Type* print_type = create_type_function();
    VECTOR_ADD_TYPE(&print_type->function.param_types, CREATE_TYPE_STRING());
    print_type->function.return_type = CREATE_TYPE_VOID();

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
    readstr_type->function.return_type = CREATE_TYPE_STRING();

    NativeFunction readstr = (NativeFunction) {
        .name = "readstr",
        .length = 7,
        .function = stdio_readstr,
        .type = readstr_type,
    };

    NativeFunction read_stdin = (NativeFunction) {
        .name = "stdin",
        .length = 5,
        .function = stdio_read_stdin,
        .type = readstr_type,
    };

#define FN_LENGTH 4
    static NativeFunction functions[FN_LENGTH];
    functions[0] = println;
    functions[1] = print;
    functions[2] = readstr;
    functions[3] = read_stdin;

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
    if (! ensure_is_tty()) {
        return NIL_VALUE();
    }

    Vector buffer;
    init_vector(&buffer, sizeof(char));

    char c = getchar();
    while (c != '\n' && c != '\r' && c != EOF) {
        VECTOR_ADD(&buffer, c, char);
        c = getchar();
    }

    ObjString* str = copy_string((char*) buffer.elements, buffer.size);
    free_vector(&buffer);
    return OBJ_VALUE(str, CREATE_TYPE_STRING());
}

static bool ensure_is_tty() {
#if defined(WIN32) || defined(_WIN32)
    #define ISATTY(dsc) _isatty(dsc)
    #define TTYNAME(dsc) "CON"
#else
    #define ISATTY(dsc) isatty(dsc)
    #define TTYNAME(dsc) ttyname(dsc)
#endif

    int dscin = fileno(stdin);
    if (dscin == -1) {
        runtime_error("Cannot get stdin descriptor");
        return false;
    }
    if (ISATTY(dscin)) {
        return true;
    }

    int dscout = fileno(stdout);
    if (dscout == -1) {
        runtime_error("Cannot get stdout descriptor");
        return false;
    }
    char* tty = TTYNAME(dscout);
    if (tty == NULL) {
        runtime_error("Cannot get tty name");
        return false;
    }
    freopen(tty, "r", stdin);
    return true;

#undef ISATTY
#undef TTYNAME
}

static Value stdio_read_stdin(int argc, Value* argv) {
    Vector buffer;
    init_vector(&buffer, sizeof(char));

    char c = getchar();
    while (c != EOF) {
        VECTOR_ADD(&buffer, c, char);
        c = getchar();
    }

    ObjString* contents = copy_string((char*) buffer.elements, buffer.size);
    free_vector(&buffer);
    return OBJ_VALUE(contents, CREATE_TYPE_STRING());
}
