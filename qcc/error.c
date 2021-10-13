#include "error.h"
#include "common.h"
#include "math.h"

static const char* get_line(const char* source, int line) {
    int current_line = 1;
    const char* letter = source;
    while (current_line < line) {
        if (*letter == '\n') {
            current_line++;
        }
        if (*letter == '\0') {
            break;
        }
        letter++;
        assert(*letter != '\0');
    }
    return letter;
}

static const char* print_line(const char* str, int line) {
    fprintf(stderr, "%d | ", line);
    for (;;) {
        if (*str == '\0') {
            break;
        }
        if (*str == '\n') {
            str++;
            break;
        }
        fprintf(stderr, "%c", *str);
        str++;
    }
    fprintf(stderr, "\n");
    return str;
}

static void print_arrow(Token* token) {
    int line_len = floor(log10(token->line)) + 1;
    for (int i = 0; i < line_len; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, " | ");
    for (uint32_t i = 0; i < token->column; i++) {
        fprintf(stderr, "~");
    }
    fprintf(stderr, "^\n");
}

void print_error_context(const char* source, Token* at) {
    const char* line = NULL;
    if (at->line == 1) {
        line = get_line(source, at->line);
    } else {
        line = get_line(source, at->line - 1);
        line = print_line(line, at->line - 1);
    }
    print_line(line, at->line);
    print_arrow(at);
    fprintf(stderr, "\n");
}

