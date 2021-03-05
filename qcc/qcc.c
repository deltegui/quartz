﻿#include <string.h>
#include "common.h"
#include "sysexits.h"
#include "chunk.h"
#include "compiler.h"
#include "vm.h"

#ifdef DEBUG
#include "debug.h"
#endif

// Reads a file from "source_name" and returns a string with
// the contents of the file. The ownership of that string is
// up to you, so delete it. It can also return null, meaning
// that was an error.
const char* read_file(const char* source_name) {
    FILE* source = fopen(source_name, "r");
    if (source == NULL) {
        fprintf(stderr, "Error while reading source file: \n");
        return NULL;
    }
    fseek(source, 0, SEEK_END);
    size_t size = ftell(source);
    fseek(source, 0, SEEK_SET);
    // Size of the file plus \0 character
    char* buffer = (char*) malloc(size + 1);
    if (buffer == NULL) {
        fclose(source);
        fprintf(stderr, "Error while allocating file buffer!\n");
        return NULL;
    }
    fread(buffer, 1, size, source);
    buffer[size] = '\0';
    fclose(source);
    return buffer;
}

int compile_file(const char* file) {
    const char* source = read_file(file);

#ifdef DEBUG
    printf("Readed buffer:\n%s\n", source);
#endif

    if (source == NULL) {
        return EX_OSFILE;
    }
    Chunk chunk;
    init_chunk(&chunk);
    if (compile(source, &chunk)) {
        vm_execute(&chunk);
    }
    free_chunk(&chunk);
    free((char*) source);
    return 0;
}

static inline bool strempty(const char* str) {
    return strlen(str) == 0 || ( strlen(str) == 1 && str[0] == '\n' );
}

void repl() {
#define BUFFER_SIZE 256
    char input_buffer[BUFFER_SIZE];
    Chunk chunk;
    for (;;) {
        init_chunk(&chunk);
        printf("<qz> ");
        if (!fgets(input_buffer, BUFFER_SIZE, stdin)) {
            fprintf(stderr, "Error while reading from stdin!\n");
            free_chunk(&chunk);
            exit(EX_IOERR);
        }
        if (strempty(input_buffer)) {
            continue;
        }
        if (compile(input_buffer, &chunk)) {
            vm_execute(&chunk);
        }
        free_chunk(&chunk);
    }
#undef BUFFER_SIZE
}


int main(int argc, char** argv) {
    if (argc <= 1) {
        repl();
    }
    return compile_file(argv[1]);
}
