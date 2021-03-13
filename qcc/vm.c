#include "vm.h"
#include "values.h"
#include "math.h"

#ifdef VM_DEBUG
#include "debug.h"
#endif

#define STACK_MAX 256

Value stack[STACK_MAX];
Value* stack_top;

static inline void stack_push(Value val) {
    *(stack_top++) = val;
}

static inline Value stack_pop() {
    return *(--stack_top);
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

    stack_top = stack;
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
            Value val = stack_pop();
            double d = AS_NUMBER(val);
            stack_push(NUMBER_VALUE(d * -1));
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
        stack_print(stack_top, stack);
        printf("\n\n");
#endif
    }
#undef READ_BYTE
}
