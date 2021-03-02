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

// Compile a source a let the result into a already created
// chunk. compile can return true or false. true means it was
// able to compile the source. false means there was an error.
bool compile(const char* source, Chunk* output_chunk);

#endif