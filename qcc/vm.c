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

// OMG i really like this shitty part
static inline void sum_op() {
    Value b = stack_pop();
    Value a = stack_pop();
    if (IS_FLOAT(a) && IS_FLOAT(b)) {
        double dB = AS_FLOAT(b);
        double dA = AS_FLOAT(a);
        stack_push(FLOAT_VALUE(dA + dB));
    }
    if (IS_INTEGER(a) && IS_INTEGER(b)) {
        int iB = AS_INTEGER(b);
        int iA = AS_INTEGER(a);
        stack_push(INTEGER_VALUE(iA + iB));
    }
    if (IS_FLOAT(a) && IS_INTEGER(b)) {
        int iB = AS_INTEGER(b);
        double iA = AS_FLOAT(a);
        stack_push(FLOAT_VALUE(iA + iB));
    }
    if (IS_INTEGER(a) && IS_FLOAT(b)) {
        double iB = AS_FLOAT(b);
        int iA = AS_INTEGER(a);
        stack_push(FLOAT_VALUE(iA + iB));
    }
}

static inline void sub_op() {
    Value b = stack_pop();
    Value a = stack_pop();
    if (IS_FLOAT(a) && IS_FLOAT(b)) {
        double dB = AS_FLOAT(b);
        double dA = AS_FLOAT(a);
        stack_push(FLOAT_VALUE(dA - dB));
    }
    if (IS_INTEGER(a) && IS_INTEGER(b)) {
        int iB = AS_INTEGER(b);
        int iA = AS_INTEGER(a);
        stack_push(INTEGER_VALUE(iA - iB));
    }
    if (IS_FLOAT(a) && IS_INTEGER(b)) {
        int iB = AS_INTEGER(b);
        double iA = AS_FLOAT(a);
        stack_push(FLOAT_VALUE(iA - iB));
    }
    if (IS_INTEGER(a) && IS_FLOAT(b)) {
        double iB = AS_FLOAT(b);
        int iA = AS_INTEGER(a);
        stack_push(FLOAT_VALUE(iA - iB));
    }
}

static inline void mul_op() {
    Value b = stack_pop();
    Value a = stack_pop();
    if (IS_FLOAT(a) && IS_FLOAT(b)) {
        double dB = AS_FLOAT(b);
        double dA = AS_FLOAT(a);
        stack_push(FLOAT_VALUE(dA * dB));
    }
    if (IS_INTEGER(a) && IS_INTEGER(b)) {
        int iB = AS_INTEGER(b);
        int iA = AS_INTEGER(a);
        stack_push(INTEGER_VALUE(iA * iB));
    }
    if (IS_FLOAT(a) && IS_INTEGER(b)) {
        int iB = AS_INTEGER(b);
        double iA = AS_FLOAT(a);
        stack_push(FLOAT_VALUE(iA * iB));
    }
    if (IS_INTEGER(a) && IS_FLOAT(b)) {
        double iB = AS_FLOAT(b);
        int iA = AS_INTEGER(a);
        stack_push(FLOAT_VALUE(iA * iB));
    }
}

static inline void div_op() {
    Value b = stack_pop();
    Value a = stack_pop();
    if (IS_FLOAT(a) && IS_FLOAT(b)) {
        double dB = AS_FLOAT(b);
        double dA = AS_FLOAT(a);
        stack_push(FLOAT_VALUE(dA / dB));
    }
    if (IS_INTEGER(a) && IS_INTEGER(b)) {
        double dB = (double) AS_INTEGER(b);
        double dA = (double) AS_INTEGER(a);
        stack_push(FLOAT_VALUE(dA / dB));
    }
    if (IS_FLOAT(a) && IS_INTEGER(b)) {
        double dB = AS_INTEGER(b);
        double dA = AS_FLOAT(a);
        stack_push(FLOAT_VALUE(dA / dB));
    }
    if (IS_INTEGER(a) && IS_FLOAT(b)) {
        double dB = AS_FLOAT(b);
        double dA = AS_INTEGER(a);
        stack_push(FLOAT_VALUE(dA / dB));
    }
}

static void value_print(Value val) {
    switch (val.type) {
    case VALUE_INTEGER: printf("%d\n", AS_INTEGER(val)); break;
    case VALUE_FLOAT: printf("%f\n", AS_FLOAT(val)); break;
    }
}

void vm_execute(Chunk* chunk) {
    stack_top = stack;
    uint8_t* pc = chunk->code;
#define READ_BYTE() *(pc++)
    for (;;) {
        switch (READ_BYTE()) {
        case OP_ADD: {
            sum_op();
            break;
        }
        case OP_SUB: {
            sub_op();
            break;
        }
        case OP_MUL: {
            mul_op();
            break;
        }
        case OP_DIV: {
            div_op();
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
            return;
        }
        }
    }
#undef READ_BYTE
}

