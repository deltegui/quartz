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

#define BINARY_OP(op) do {\
    double b = AS_FLOAT(stack_pop());\
    double a = AS_FLOAT(stack_pop());\
    stack_push(FLOAT_VALUE(a op b));\
} while(false)

void vm_execute(Chunk* chunk) {
    stack_top = stack;
    uint8_t* pc = chunk->code;
#define READ_BYTE() *(pc++)
    for (;;) {
        switch (READ_BYTE()) {
        case OP_ADD: {
            BINARY_OP(+);
            break;
        }
        case OP_SUB: {
            BINARY_OP(-);
            break;
        }
        case OP_MUL: {
            BINARY_OP(*);
            break;
        }
        case OP_DIV: {
            BINARY_OP(/);
            break;
        }
        case OP_CONSTANT: {
            uint8_t index = READ_BYTE();
            Value val = chunk->constants.values[index];
            stack_push(val);
            break;
        }
        case OP_RETURN: {
            double result = AS_FLOAT(stack_pop());
            printf("%f\n", result);
            return;
        }
        }
    }
#undef READ_BYTE
}

