#ifndef QUARTZ_CHUNK_H
#define QUARTZ_CHUNK_H

#include "common.h"
#include "values.h"

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

typedef struct {
    int size;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

void chunk_init(Chunk* chunk);
void chunk_free(Chunk* chunk);
void chunk_write(Chunk* chunk, uint8_t bytecode, int line);
void chunk_print(Chunk* chunk);

#endif