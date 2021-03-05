#ifndef QUARTZ_CHUNK_H
#define QUARTZ_CHUNK_H

#include "common.h"
#include "values.h"

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_INVERT_SIGN,
    OP_NOT,
    OP_AND,
    OP_OR,
    OP_CONSTANT,
    OP_NOP,
    OP_RETURN,
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