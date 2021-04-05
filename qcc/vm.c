#include "vm.h"
#include "values.h"
#include "math.h"
#include "vm_memory.h"

#ifdef VM_DEBUG
#include "debug.h"
#endif

QVM qvm;

void init_qvm() {
    init_table(&qvm.strings);
    init_table(&qvm.globals);
    qvm.stack_top = qvm.stack;
    qvm.objects = NULL;
}

void free_qvm() {
    free_table(&qvm.strings);
    free_table(&qvm.globals);
    free_objects();
}

static inline void stack_push(Value val) {
    *(qvm.stack_top++) = val;
}

static inline Value stack_pop() {
    return *(--qvm.stack_top);
}

static inline Value stack_peek(uint8_t distance) {
    return *(qvm.stack_top - distance - 1);
}

#define NUM_BINARY_OP(op)\
    double b = AS_NUMBER(stack_pop());\
    double a = AS_NUMBER(stack_pop());\
    stack_push(NUMBER_VALUE(a op b))

#define BOOL_BINARY_OP(op)\
    bool b = AS_BOOL(stack_pop());\
    bool a = AS_BOOL(stack_pop());\
    stack_push(BOOL_VALUE(a op b))

#define STRING_CONCAT()\
    ObjString* b = AS_STRING_OBJ(AS_OBJ(stack_pop()));\
    ObjString* a = AS_STRING_OBJ(AS_OBJ(stack_pop()));\
    ObjString* concat = concat_string(a, b);\
    stack_push(OBJ_VALUE(concat))

#define CONSTANT_OP(read)\
    Value val = read();\
    stack_push(val)

#define DEFINE_GLOBAL_OP(str_read)\
    ObjString* identifier = str_read();\
    table_set(&qvm.globals, identifier, stack_peek(0));\
    stack_pop()

#define SET_GLOBAL_OP(str_read)\
    ObjString* identifier = str_read();\
    table_set(&qvm.globals, identifier, stack_peek(0))

#define GET_GLOBAL_OP(str_read)\
    ObjString* identifier = str_read();\
    stack_push(table_find(&qvm.globals, identifier))

void qvm_execute(Chunk* chunk) {
#ifdef VM_DEBUG
    printf("--------[ EXECUTION ]--------\n\n");
#endif

    uint8_t* pc = chunk->code;

#define READ_BYTE() *(pc++)
#define READ_CONSTANT() chunk->constants.values[READ_BYTE()]
#define READ_STRING() AS_STRING_OBJ(AS_OBJ(READ_CONSTANT()))
#define READ_CONSTANT_LONG() chunk->constants.values[read_long(&pc)]
#define READ_STRING_LONG() AS_STRING_OBJ(AS_OBJ(READ_CONSTANT_LONG()))

    for (;;) {
#ifdef VM_DEBUG
        opcode_print(*pc);
#endif
        switch (READ_BYTE()) {
        case OP_ADD: {
            Value second = stack_peek(0);
            Value first = stack_peek(1);
            if (first.type == STRING_TYPE && second.type == STRING_TYPE) {
                STRING_CONCAT();
                break;
            }
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
            Value val = *(qvm.stack_top - 1);
            double d = AS_NUMBER(val);
            *(qvm.stack_top - 1) = NUMBER_VALUE(d * -1);
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
            Value b = stack_pop();
            Value a = stack_pop();
            bool result = value_equals(a, b);
            stack_push(BOOL_VALUE(result));
            break;
        }
        case OP_GREATER: {
            double b = AS_NUMBER(stack_pop());
            double a = AS_NUMBER(stack_pop());
            stack_push(BOOL_VALUE(a > b));
            break;
        }
        case OP_LOWER: {
            double b = AS_NUMBER(stack_pop());
            double a = AS_NUMBER(stack_pop());
            stack_push(BOOL_VALUE(a < b));
            break;
        }
        case OP_CONSTANT: {
            CONSTANT_OP(READ_CONSTANT);
            break;
        }
        case OP_CONSTANT_LONG: {
            CONSTANT_OP(READ_CONSTANT_LONG);
            break;
        }
        case OP_DEFINE_GLOBAL: {
            DEFINE_GLOBAL_OP(READ_STRING);
            break;
        }
        case OP_DEFINE_GLOBAL_LONG: {
            DEFINE_GLOBAL_OP(READ_STRING_LONG);
            break;
        }
        case OP_SET_GLOBAL: {
            SET_GLOBAL_OP(READ_STRING);
            break;
        }
        case OP_SET_GLOBAL_LONG: {
            SET_GLOBAL_OP(READ_STRING_LONG);
            break;
        }
        case OP_GET_GLOBAL: {
            GET_GLOBAL_OP(READ_STRING);
            break;
        }
        case OP_GET_GLOBAL_LONG: {
            GET_GLOBAL_OP(READ_STRING_LONG);
            break;
        }
        case OP_PRINT:
            value_print(stack_pop());
            printf("\n");
            break;
        case OP_POP:
            stack_pop();
            break;
        case OP_RETURN: {
            return;
        }
        }
#ifdef VM_DEBUG
        stack_print(qvm.stack_top, qvm.stack);
        printf("\n\n");
#endif
    }
#undef READ_BYTE
}
