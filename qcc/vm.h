#ifndef QUARTZ_VM_H
#define QUARTZ_VM_H

#include "chunk.h"

#define STACK_MAX 256

typedef struct {
    Value stack[STACK_MAX];
    Value* stack_top;
    Obj* objects;
} QVM;

extern QVM qvm;

void vm_execute(Chunk* chunk);

#endif