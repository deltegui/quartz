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

// Well, i know macros should't be used
// to define large pieces of code, but
// this is the only way to generate the
// same code to some binary operations
// based only the operation itself.
#define BINARY_OP(op) do {\
    Value b = stack_pop();\
    Value a = stack_pop();\
    if (IS_FLOAT(a) && IS_FLOAT(b)) {\
        double dB = AS_FLOAT(b);\
        double dA = AS_FLOAT(a);\
        stack_push(FLOAT_VALUE(dA op dB));\
    }\
    if (IS_INTEGER(a) && IS_INTEGER(b)) {\
        int iB = AS_INTEGER(b);\
        int iA = AS_INTEGER(a);\
        stack_push(INTEGER_VALUE(iA op iB));\
    }\
    if (IS_FLOAT(a) && IS_INTEGER(b)) {\
        int iB = AS_INTEGER(b);\
        double iA = AS_FLOAT(a);\
        stack_push(FLOAT_VALUE(iA op iB));\
    }\
    if (IS_INTEGER(a) && IS_FLOAT(b)) {\
        double iB = AS_FLOAT(b);\
        int iA = AS_INTEGER(a);\
        stack_push(FLOAT_VALUE(iA op iB));\
    }\
} while (false)

static inline void sum_op() {
    BINARY_OP(+);
}

static inline void sub_op() {
    BINARY_OP(-);
}

static inline void mul_op() {
    BINARY_OP(*);
}

static inline double to_double(Value value) {
    if (IS_FLOAT(value)) {
        return AS_FLOAT(value);
    }
    return (double) AS_INTEGER(value);
}

// Div operation is special. The two
// arguments of a div operation must
// be casted to double and should obtain
// a double.
static inline void div_op() {
    double b = to_double(stack_pop());
    double a = to_double(stack_pop());
    stack_push(FLOAT_VALUE(a / b));
}

static inline void and_op() {
    bool b = AS_BOOL(stack_pop());
    bool a = AS_BOOL(stack_pop());
    stack_push(BOOL_VALUE(a && b));
}

static inline void or_op() {
    bool b = AS_BOOL(stack_pop());
    bool a = AS_BOOL(stack_pop());
    stack_push(BOOL_VALUE(a || b));
}

static inline void not_op() {
    bool a = AS_BOOL(stack_pop());
    stack_push(BOOL_VALUE(!a));
}

static inline void mod_op() {
    Value b = stack_pop();
    Value a = stack_pop();
    if (IS_INTEGER(a) && IS_INTEGER(b)) {
        int ai = AS_INTEGER(a);
        int bi = AS_INTEGER(b);
        stack_push(INTEGER_VALUE(ai % bi));
        return;
    }
    double ad;
    double bd;
    if (IS_INTEGER(a) && IS_FLOAT(b)) {
        ad = (double) AS_INTEGER(a);
        bd = AS_FLOAT(b);
    } else if (IS_FLOAT(a) && IS_INTEGER(b)) {
        ad = AS_FLOAT(a);
        bd = (double) AS_INTEGER(b);
    } else {
        ad = AS_FLOAT(a);
        bd = AS_FLOAT(b);
    }
    double modulus = fmod(ad, bd);
    stack_push(FLOAT_VALUE(modulus));
}

// @todo value print comes from debug.c. Fix this shitty thing.
static void value_print(Value val) {
    switch (val.type) {
    case VALUE_INTEGER: printf("%d", AS_INTEGER(val)); break;
    case VALUE_FLOAT: printf("%f", AS_FLOAT(val)); break;
    case VALUE_BOOL: printf("%s", AS_BOOL(val) ? "true" : "false"); break;
    }
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
        case OP_INVERT_SIGN: {
            Value val = stack_pop();
            if (IS_FLOAT(val)) {
                double d = AS_FLOAT(val);
                stack_push(FLOAT_VALUE(d * -1));
            } else {
                int i = AS_INTEGER(val);
                stack_push(INTEGER_VALUE(i * -1));
            }
            break;
        }
        case OP_AND: {
            and_op();
            break;
        }
        case OP_OR: {
            or_op();
            break;
        }
        case OP_NOT: {
            not_op();
            break;
        }
        case OP_MOD: {
            mod_op();
            break;
        }
        case OP_NOP:
            break;
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

