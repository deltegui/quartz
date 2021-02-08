

#include "sysexits.h"
#include "astprint.h"
#include "lexer.h"
#include "parser.h"

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
    Quartz::Lexer lexer = Quartz::Lexer(source);
    Quartz::Parser parser = Quartz::Parser(&lexer);
    auto ast = parser.parse();
    Quartz::AstPrinter printer;
    ast->accept(&printer);
    delete ast;
    delete source;
    return 0;
}
