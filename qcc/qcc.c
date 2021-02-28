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

int main(int argc, char** argv) {
    if (argc <= 1) {
        fprintf(stderr, "You should pass the file to compile!");
        return EX_USAGE;
    }
    const char* source = read_file(argv[1]);

#ifdef DEBUG
    printf("Readed buffer:\n%s\n", source);
#endif

    if (source == NULL) {
        return EX_OSFILE;
    }
    Chunk chunk;
    chunk_init(&chunk);
    compile(source, &chunk);
    vm_execute(&chunk);
    chunk_free(&chunk);
    free((char*) source);
    return 0;
}
