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
    qvm.frame_count = 0;
}

void free_qvm() {
    free_table(&qvm.strings);
    free_table(&qvm.globals);
    free_objects();
}

static inline void call(uint8_t param_count) {
    qvm.frame_count++;
    Value* fn_ptr = (qvm.stack_top - param_count - 1);
    Value fn_value = *fn_ptr;
    ObjFunction* fn = OBJ_AS_FUNCTION(VALUE_AS_OBJ(fn_value));
    qvm.frame = &qvm.frames[qvm.frame_count - 1];
    qvm.frame->func = fn;
    qvm.frame->pc = fn->chunk.code;
    qvm.frame->slots = fn_ptr;
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
    double b = VALUE_AS_NUMBER(stack_pop());\
    double a = VALUE_AS_NUMBER(stack_pop());\
    stack_push(NUMBER_VALUE(a op b))

#define BOOL_BINARY_OP(op)\
    bool b = VALUE_AS_BOOL(stack_pop());\
    bool a = VALUE_AS_BOOL(stack_pop());\
    stack_push(BOOL_VALUE(a op b))

#define STRING_CONCAT()\
    ObjString* b = OBJ_AS_STRING(VALUE_AS_OBJ(stack_pop()));\
    ObjString* a = OBJ_AS_STRING(VALUE_AS_OBJ(stack_pop()));\
    ObjString* concat = concat_string(a, b);\
    Value val = OBJ_VALUE(concat);\
    val.type = SIMPLE_TYPE(TYPE_STRING);\
    stack_push(val)

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

#define READ_BYTE() *(qvm.frame->pc++)

#define READ_CONSTANT() qvm.frame->func->chunk.constants.values[READ_BYTE()]
#define READ_STRING() OBJ_AS_STRING(VALUE_AS_OBJ(READ_CONSTANT()))

#define READ_CONSTANT_LONG() qvm.frame->func->chunk.constants.values[read_long(&qvm.frame->pc)]
#define READ_STRING_LONG() OBJ_AS_STRING(VALUE_AS_OBJ(READ_CONSTANT_LONG()))

#define READ_GLOBAL_CONSTANT() qvm.frames[0].func->chunk.constants.values[READ_BYTE()]
#define READ_GLOBAL_STRING() OBJ_AS_STRING(VALUE_AS_OBJ(READ_GLOBAL_CONSTANT()))

#define READ_GLOBAL_CONSTANT_LONG() qvm.frames[0].func->chunk.constants.values[read_long(&qvm.frame->pc)]
#define READ_GLOBAL_STRING_LONG() OBJ_AS_STRING(VALUE_AS_OBJ(READ_GLOBAL_CONSTANT_LONG()))

static void run(ObjFunction* func) {
#ifdef VM_DEBUG
    printf("--------[ EXECUTION ]--------\n\n");
#endif

    qvm.frame = &qvm.frames[qvm.frame_count - 1];
    qvm.frame->func = func;

    for (;;) {
#ifdef VM_DEBUG
        opcode_print(*qvm.frame->pc);
#endif
        switch (READ_BYTE()) {
        case OP_ADD: {
            Value second = stack_peek(0);
            Value first = stack_peek(1);
            if (TYPE_IS_KIND(first.type, TYPE_STRING) && TYPE_IS_KIND(second.type, TYPE_STRING)) {
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
            double d = VALUE_AS_NUMBER(val);
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
            bool a = VALUE_AS_BOOL(stack_pop());
            stack_push(BOOL_VALUE(!a));
            break;
        }
        case OP_MOD: {
            double b = VALUE_AS_NUMBER(stack_pop());
            double a = VALUE_AS_NUMBER(stack_pop());
            stack_push(NUMBER_VALUE(fmod(a, b)));
            break;
        }
        case OP_NOP:
            break;
        case OP_TRUE: {
            stack_push(BOOL_VALUE(true));
            break;
        }
        case OP_FALSE: {
            stack_push(BOOL_VALUE(false));
            break;
        }
        case OP_NIL: {
            stack_push(NIL_VALUE());
            break;
        }
        case OP_EQUAL: {
            Value b = stack_pop();
            Value a = stack_pop();
            bool result = value_equals(a, b);
            stack_push(BOOL_VALUE(result));
            break;
        }
        case OP_GREATER: {
            double b = VALUE_AS_NUMBER(stack_pop());
            double a = VALUE_AS_NUMBER(stack_pop());
            stack_push(BOOL_VALUE(a > b));
            break;
        }
        case OP_LOWER: {
            double b = VALUE_AS_NUMBER(stack_pop());
            double a = VALUE_AS_NUMBER(stack_pop());
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
            SET_GLOBAL_OP(READ_GLOBAL_STRING);
            break;
        }
        case OP_SET_GLOBAL_LONG: {
            SET_GLOBAL_OP(READ_GLOBAL_STRING_LONG);
            break;
        }
        case OP_GET_GLOBAL: {
            GET_GLOBAL_OP(READ_GLOBAL_STRING);
            break;
        }
        case OP_GET_GLOBAL_LONG: {
            GET_GLOBAL_OP(READ_GLOBAL_STRING_LONG);
            break;
        }
        case OP_GET_LOCAL: {
            uint8_t slot = READ_BYTE();
            stack_push(qvm.frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            uint8_t slot = READ_BYTE();
            qvm.frame->slots[slot] = stack_peek(0);
            break;
        }
        case OP_CALL: {
            uint8_t param_count = READ_BYTE();
            call(param_count);
            break;
        }
        case OP_PRINT: {
            value_print(stack_pop());
            printf("\n");
            break;
        }
        case OP_POP: {
            stack_pop();
            break;
        }
        case OP_RETURN: {
            Value return_val = stack_pop();
            while (qvm.stack_top != qvm.frame->slots) {
                stack_pop();
            }
            stack_push(return_val);
            qvm.frame_count--;
            qvm.frame = &qvm.frames[qvm.frame_count - 1];
            break;
        }
        case OP_END: {
            return;
        }
        }
#ifdef VM_DEBUG
        printf("\t");
        stack_print(qvm.stack_top, qvm.stack);
        printf("\n\n");
        table_print(&qvm.globals);
#endif
    }
#undef READ_BYTE
}

void qvm_execute(ObjFunction* func) {
    stack_push(OBJ_VALUE(func));
    CallFrame* frame = &qvm.frames[qvm.frame_count++];
    frame->func = func;
    frame->pc = func->chunk.code;
    frame->slots = qvm.stack;
    run(func);
}
