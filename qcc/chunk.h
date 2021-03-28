#ifndef QUARTZ_CHUNK_H
#define QUARTZ_CHUNK_H

#include "common.h"
#include "values.h"

typedef enum {
    // Number operations
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEGATE,

    // Boolean operations
    OP_NOT,
    OP_AND,
    OP_OR,
    OP_EQUAL,
    OP_GREATER,
    OP_LOWER,

    // Reserved words and other special operations
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    OP_NOP,
    OP_RETURN,

    // Stack operations
    OP_POP,

    // Declarations
    OP_CONSTANT,
} OpCode;

typedef struct {
    int size;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

// Initializes an exiting chunk.
void init_chunk(Chunk* chunk);

// Frees an existing chunk.
void free_chunk(Chunk* chunk);

// Writes a bytecode to a chunk. It will store
// the original line
void chunk_write(Chunk* chunk, uint8_t bytecode, int line);

#endif