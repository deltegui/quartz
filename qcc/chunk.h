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
    OP_PRINT,
    OP_RETURN,

    // Stack operations
    OP_POP,
    OP_CALL,
    OP_END,

    // Declarations
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_DEFINE_GLOBAL,
	OP_GET_GLOBAL,
	OP_SET_GLOBAL,
    OP_DEFINE_GLOBAL_LONG,
	OP_GET_GLOBAL_LONG,
	OP_SET_GLOBAL_LONG,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
} OpCode;

typedef struct {
    int size;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void chunk_write(Chunk* chunk, uint8_t bytecode, int line);
bool chunk_check_last_byte(Chunk* chunk, uint8_t bytecode);
int chunk_add_constant(Chunk* chunk, Value value);

uint16_t read_long(uint8_t** pc);

#endif
