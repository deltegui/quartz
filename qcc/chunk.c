#include "chunk.h"
// Chunk is a runtime data structure, so its memory
// must be managed using memory.h
#include "memory.h"

void chunk_init(Chunk* chunk) {
    chunk->size = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    valuearray_init(&chunk->constants);
}

void chunk_free(Chunk* chunk) {
    if (chunk->code != NULL) {
        FREE(uint8_t, chunk->code);
    }
    if (chunk->lines != NULL) {
        FREE(int, chunk->lines);
    }
    chunk->size = 0;
    chunk->capacity = 0;
    valuearray_free(&chunk->constants);
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

const char* OpCodesString[] = {
    "OP_ADD",
    "OP_SUB",
    "OP_MUL",
    "OP_DIV",
    "OP_CONSTANT",
    "OP_RETURN",
};

void chunk_print(Chunk* chunk) {
    printf("--------[ CHUNK DUMP ]--------\n\n");
    for (int i = 0; i < chunk->size; i++) {
        printf("[%d] %04x\n", i, chunk->code[i]);
    }
    printf("\n\n");
    int i = 0;
    while (i < chunk->size) {
        switch(chunk->code[i]) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV: {
            printf("[%d] %s\n", i, OpCodesString[chunk->code[i]]);
            i++;
            break;
        }
        case OP_CONSTANT: {
            printf("[%d] %s\n", i, OpCodesString[chunk->code[i]]);
            i++;
            printf("[%d] %04x\t%d\n", i, chunk->code[i], chunk->lines[i]);
            i++;
            break;
        }
        case OP_RETURN: {
            printf("[%d] OP_RETURN\n", i);
            i++;
            break;
        }
        }
    }
}