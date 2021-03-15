#ifndef QUARTZ_VM_H
#define QUARTZ_VM_H

#include "chunk.h"

#define STACK_MAX 256

typedef struct {
    Value stack[STACK_MAX];
    Value* stack_top;
    Obj* objects;
} QVM;

void init_qvm();
void free_qvm();
void qvm_execute(Chunk* chunk);

extern QVM qvm;

#endif