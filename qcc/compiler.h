#ifndef QUARTZ_COMPILER_H
#define QUARTZ_COMPILER_H

#include "chunk.h"
#include "parser.h"
#include "expr.h"
#include "stmt.h"

typedef enum {
    COMPILATION_ERROR,
    PARSING_ERROR,
    TYPE_ERROR,
    COMPILATION_OK,
} CompilationResult;

// Compile a source a let the result into a already created
// chunk. While compiling, errors may occur.
CompilationResult compile(const char* source, Chunk* output_chunk);

#endif