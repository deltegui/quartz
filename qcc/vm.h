#ifndef QUARTZ_VM_H_
#define QUARTZ_VM_H_

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

    Obj** gray_stack;
    int gray_stack_capacity;
    int gray_stack_size;

    bool is_running;
    bool had_runtime_error;

    size_t bytes_allocated;
    size_t next_gc_trigger;
} QVM;

void init_qvm();
void free_qvm();
void stack_push(Value val);
Value stack_pop();
void qvm_execute(ObjFunction* func);
void qvm_push_gray(Obj* obj);
Obj* qvm_pop_gray();

extern QVM qvm;

#endif
