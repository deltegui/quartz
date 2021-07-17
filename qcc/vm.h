#ifndef QUARTZ_VM_H
#define QUARTZ_VM_H

#include "chunk.h"
#include "object.h"
#include "table.h"

#define FRAMES_MAX 64
#define STACK_MAX FRAMES_MAX * 2

typedef struct {
    ObjFunction* func;
    uint8_t* pc;
    Value* slots;
} CallFrame;

typedef struct {
    Value stack[STACK_MAX];
    Value* stack_top;

    Obj* objects;

    Table strings;
    Table globals;

    CallFrame frames[FRAMES_MAX];
    int frame_count;
    CallFrame* frame;
} QVM;

void init_qvm();
void free_qvm();
void qvm_execute(ObjFunction* func);

extern QVM qvm;

#endif
