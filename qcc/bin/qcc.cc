#include <stdio.h>
#include <stdlib.h>

#include "astprint.h"
#include "lexer.h"
#include "parser.h"

// Reads all contents of file source_name and let it
// in a char* buffer. This function can return NULL
// if the file does not exists or if there was an error
// while allocating memory. The ownership of the returning char*
// is yours.
char* read_file(const char *source_name) {
    FILE* source = fopen(source_name, "r");
    if (source == NULL) {
        fprintf(stderr, "Error while reading source file: \n");
        return NULL;
    }
    fseek(source, 0, SEEK_END);
    size_t size = ftell(source);
    fseek(source, 0, SEEK_SET);
    printf("Allocating for %zu bytes\n", size + 1);
    // Size of the file plus \0 character
    char* buffer = new char[size + 1];
    if (buffer == NULL) {
        fprintf(stderr, "Error while allocating file buffer!\n");
        return NULL;
    }
    fread(buffer, 1, size, source);
    fclose(source);
    buffer[size] = '\0';
    return buffer;
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "You should pass the file to compile!");
        return 1;
    }
    char *source = read_file(argv[1]);
    printf("Readed buffer:\n%s\n", source);
    if (source == NULL) {
        return -1;
    }
    Quartz::Lexer lexer = Quartz::Lexer(source);
    Quartz::Parser parser = Quartz::Parser(&lexer);
    Quartz::AstPrinter printer;
    auto ast = parser.parse();
    ast->accept(&printer);
    delete ast;
    delete source;
    return 0;
}
