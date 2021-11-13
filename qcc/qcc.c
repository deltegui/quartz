#include <string.h>
#include "common.h"
#include "sysexits.h"
#include "chunk.h"
#include "compiler.h"
#include "vm.h"
#include "import.h"

#ifdef DEBUG
#include "debug.h"
#endif

int run(const char* file, int length) {
    init_module_system();
    Import main = import(file, length);
    assert(!main.is_native);

#ifdef DEBUG
    printf("Read buffer:\n%s\n", main.source);
#endif

    if (main.file.source == NULL) {
        free_module_system();
        return EX_OSFILE;
    }
    int exit_code = 0;
    ObjFunction* main_func;
    init_qvm();
    if (compile(main.file.source, &main_func) == COMPILATION_OK) {
        qvm_execute(main_func);
    } else {
        exit_code = EX_DATAERR;
    }
    free_qvm();
    free_module_system();
    return exit_code;
}

static inline bool strempty(const char* str) {
    return strlen(str) == 0 || ( strlen(str) == 1 && str[0] == '\n' );
}

void repl() {
#define BUFFER_SIZE 256
    char input_buffer[BUFFER_SIZE];
    ObjFunction* main_func;
    for (;;) {
        init_module_system();
        printf("<qz> ");
        if (!fgets(input_buffer, BUFFER_SIZE, stdin)) {
            fprintf(stderr, "Error while reading from stdin!\n");
            free_module_system();
            exit(EX_IOERR);
        }
        if (strempty(input_buffer)) {
            continue;
        }
        init_qvm();
        if (compile(input_buffer, &main_func) == COMPILATION_OK) {
            qvm_execute(main_func);
        }
        free_qvm();
        free_module_system();
    }
#undef BUFFER_SIZE
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        repl();
    }
    int length = strlen(argv[1]);
    return run(argv[1], length);
}
