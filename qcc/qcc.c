#include "common.h"
#include "sysexits.h"
#include "lexer.h"
#include "parser.h"
#include "astprint.h"
#include "compiler.h"
#include "vm.h"

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
    printf("Readed buffer:\n%s\n", source);
    if (source == NULL) {
        return EX_OSFILE;
    }
    Parser* parser = create_parser(source);
    Expr* ast = parse(parser);
    if (!parser->has_error) {
        ast_print(ast);
        init_compiler();
        Chunk* chunk = compile(ast);
        chunk_print(chunk);
        vm_execute(chunk);
        free_compiler();
    }
    free_parser(parser);
    expr_free(ast);
    free((char*) source);
    return 0;
}
