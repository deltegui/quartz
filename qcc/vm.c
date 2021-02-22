#include "vm.h"
#include "values.h"

#define STACK_MAX 256

Value stack[STACK_MAX];
Value* stack_top;

static inline void stack_push(Value val) {
    *(stack_top++) = val;
}

static inline Value stack_pop() {
    return *(--stack_top);
}

void vm_execute(Chunk* chunk) {
    stack_top = stack;
    uint8_t* pc = chunk->code;
#define READ_BYTE() *(pc++)
    for (;;) {
        switch (READ_BYTE()) {
        case OP_ADD: {
            double b = AS_FLOAT(stack_pop());
            double a = AS_FLOAT(stack_pop());
            printf("%f + %f\n", a, b);
            stack_push(FLOAT_VALUE(a + b));
            break;
        }
        case OP_SUB: {
            double b = AS_FLOAT(stack_pop());
            double a = AS_FLOAT(stack_pop());
            stack_push(FLOAT_VALUE(a - b));
            break;
        }
        case OP_MUL: {
            double b = AS_FLOAT(stack_pop());
            double a = AS_FLOAT(stack_pop());
            stack_push(FLOAT_VALUE(a * b));
            break;
        }
        case OP_DIV: {
            double b = AS_FLOAT(stack_pop());
            double a = AS_FLOAT(stack_pop());
            stack_push(FLOAT_VALUE(a / b));
            break;
        }
        case OP_CONSTANT: {
            uint8_t index = READ_BYTE();
            printf("PUSH cte %d\n", index);
            Value val = chunk->constants.values[index];
            stack_push(val);
            break;
        }
        case OP_RETURN: {
            double result = AS_FLOAT(stack_pop());
            printf("Result: %f\n", result);
            return;
        }
        }
    }
#undef READ_BYTE
}

