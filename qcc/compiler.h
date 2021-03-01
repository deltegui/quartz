#ifndef QUARTZ_COMPILER_H
#define QUARTZ_COMPILER_H

#include "chunk.h"
#include "parser.h"
#include "expr.h"

typedef struct {
    Parser parser;
    Chunk* chunk;
    bool has_error;
} Compiler;

bool compile(const char* source, Chunk* output_chunk);

#endif