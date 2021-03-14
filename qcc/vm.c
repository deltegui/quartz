#include "vm.h"
#include "values.h"
#include "math.h"
#include "vm_memory.h"

#ifdef VM_DEBUG
#include "debug.h"
#endif

QVM qvm;

static void init_qvm(QVM* qvm) {
    qvm->stack_top = qvm->stack;
    qvm->objects = NULL;
}

static void free_qvm(QVM* qvm) {
    free_objects();
}

static inline void stack_push(Value val) {
    *(qvm.stack_top++) = val;
}

static inline Value stack_pop() {
    return *(--qvm.stack_top);
}

#define NUM_BINARY_OP(op)\
    double b = AS_NUMBER(stack_pop());\
    double a = AS_NUMBER(stack_pop());\
    stack_push(NUMBER_VALUE(a op b))

#define BOOL_BINARY_OP(op)\
    bool b = AS_BOOL(stack_pop());\
    bool a = AS_BOOL(stack_pop());\
    stack_push(BOOL_VALUE(a op b))

static inline bool last_values_equal() {
    Value b = stack_pop();
    Value a = stack_pop();
    return value_equals(a, b);
}

void vm_execute(Chunk* chunk) {
#ifdef VM_DEBUG
    printf("--------[ EXECUTION ]--------\n\n");
#endif

    init_qvm(&qvm);
    uint8_t* pc = chunk->code;

#define READ_BYTE() *(pc++)

    for (;;) {
#ifdef VM_DEBUG
        opcode_print(*pc);
#endif
        switch (READ_BYTE()) {
        case OP_ADD: {
            NUM_BINARY_OP(+);
            break;
        }
        case OP_SUB: {
            NUM_BINARY_OP(-);
            break;
        }
        case OP_MUL: {
            NUM_BINARY_OP(*);
            break;
        }
        case OP_DIV: {
            NUM_BINARY_OP(/);
            break;
        }
        case OP_NEGATE: {
            Value val = *qvm.stack_top;
            double d = AS_NUMBER(val);
            *qvm.stack_top = NUMBER_VALUE(d * -1);
            break;
        }
        case OP_AND: {
            BOOL_BINARY_OP(&&);
            break;
        }
        case OP_OR: {
            BOOL_BINARY_OP(||);
            break;
        }
        case OP_NOT: {
            bool a = AS_BOOL(stack_pop());
            stack_push(BOOL_VALUE(!a));
            break;
        }
        case OP_MOD: {
            double b = AS_NUMBER(stack_pop());
            double a = AS_NUMBER(stack_pop());
            stack_push(NUMBER_VALUE(fmod(a, b)));
            break;
        }
        case OP_NOP:
            break;
        case OP_TRUE:
            stack_push(BOOL_VALUE(true));
            break;
        case OP_FALSE:
            stack_push(BOOL_VALUE(false));
            break;
        case OP_NIL:
            stack_push(NIL_VALUE());
            break;
        case OP_EQUAL: {
            bool result = last_values_equal();
            stack_push(BOOL_VALUE(result));
            break;
        }
        case OP_NOT_EQUAL: {
            bool result = last_values_equal();
            stack_push(BOOL_VALUE(!result));
            break;
        }
        case OP_CONSTANT: {
            uint8_t index = READ_BYTE();
            Value val = chunk->constants.values[index];
            stack_push(val);
            break;
        }
        case OP_RETURN: {
            value_print(stack_pop());
            printf("\n");
            return;
        }
        }
#ifdef VM_DEBUG
        stack_print(qvm.stack_top, qvm.stack);
        printf("\n\n");
#endif
    }
    free_qvm(&qvm);
#undef READ_BYTE
}
