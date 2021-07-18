#include "chunk.h"
// Chunk is a runtime data structure, so its memory
// must be managed using vm_memory.h
#include "vm_memory.h"

void init_chunk(Chunk* chunk) {
    chunk->size = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    init_valuearray(&chunk->constants);
}

void free_chunk(Chunk* chunk) {
    if (chunk->code != NULL) {
        FREE(uint8_t, chunk->code);
    }
    if (chunk->lines != NULL) {
        FREE(int, chunk->lines);
    }
    chunk->size = 0;
    chunk->capacity = 0;
    free_valuearray(&chunk->constants);
}

void chunk_write(Chunk* chunk, uint8_t bytecode, int line) {
    if (chunk->capacity < chunk->size + 1) {
        size_t old = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(
            uint8_t,
            chunk->code,
            old,
            chunk->capacity);
        chunk->lines = GROW_ARRAY(
            int,
            chunk->lines,
            old,
            chunk->capacity);
    }
    chunk->code[chunk->size] = bytecode;
    chunk->lines[chunk->size] = line;
    chunk->size++;
}

int chunk_add_constant(Chunk* chunk, Value value) {
    return valuearray_write(&chunk->constants, value);
}

uint16_t read_long(uint8_t** pc) {
    uint8_t high = *((*pc)++);
    uint8_t low = *((*pc)++);
    return high << 8 | low;
}
