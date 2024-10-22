#include "vm.h"
#include "values.h"
#include "math.h"
#include "vm_memory.h"
#include "type.h" // to init and free type_pool
#include "stdlib/stdlib.h" // to init and free stdlib
#include "array.h"
#include "string.h"

#ifdef VM_DEBUG
#include "debug.h"
#endif

QVM qvm;

static void init_gray_stack();
static void free_gray_stack();
void runtime_error(const char* message);
static inline void call_native(ObjNative* native, uint8_t param_count);
static inline void call_function(Obj* obj, Value* slots, uint8_t param_count);
static inline void call(uint8_t param_count);
static inline void invoke(uint8_t prop_index, uint8_t param_count);
static inline Value stack_peek(uint8_t distance);

static void init_gray_stack() {
    qvm.gray_stack = NULL;
    qvm.gray_stack_capacity = 0;
    qvm.gray_stack_size = 0;
}

static void free_gray_stack() {
    if (qvm.gray_stack_capacity > 0) {
        free(qvm.gray_stack);
    }
}

void init_qvm() {
    init_type_pool();
    init_stdlib();

    init_table(&qvm.strings);
    init_table(&qvm.globals);

    qvm.stack_top = qvm.stack;
    qvm.objects = NULL;

    init_string();
    init_array();

    init_gray_stack();

    qvm.frame_count = 0;

    qvm.is_running = false;
    qvm.had_runtime_error = false;

    qvm.bytes_allocated = 0;
    qvm.next_gc_trigger = 2048;
}

void free_qvm() {
    free_stdlib();
    free_type_pool();
    free_table(&qvm.strings);
    free_table(&qvm.globals);
    free_objects();
    free_gray_stack();
}

void qvm_push_gray(Obj* obj) {
    if (qvm.gray_stack_capacity <= qvm.gray_stack_size + 1) {
        qvm.gray_stack_capacity = GROW_CAPACITY(qvm.gray_stack_capacity);
        qvm.gray_stack = (Obj**) realloc(qvm.gray_stack, sizeof(Obj*) * qvm.gray_stack_capacity);
        if (qvm.gray_stack == NULL) {
            exit(1);
        }
    }
    qvm.gray_stack[qvm.gray_stack_size] = obj;
    qvm.gray_stack_size++;
}

Obj* qvm_pop_gray() {
    assert(qvm.gray_stack_size >= 1);
    return qvm.gray_stack[--qvm.gray_stack_size];
}

void runtime_error(const char* message) {
    qvm.had_runtime_error = true;
    printf("%s\n", message);
}

static inline void call_native(ObjNative* native, uint8_t param_count) {
    Value* params = (Value*) malloc(sizeof(Value) * param_count);
    for (int i = param_count - 1; i >= 0; i--) {
        int distance_to_peek = param_count - (i + 1);
        params[i] = stack_peek(distance_to_peek);
    }
    Value result = native->function(param_count, params);
    free(params);

    for (int i = 0; i < param_count; i++) {
        stack_pop();
    }
    stack_pop(); // pop obj native value from stack
    stack_push(result);
}

static inline ObjFunction* prepare_binded_method(ObjBindedMethod* binded, uint8_t param_count) {
    stack_push(OBJ_VALUE(binded->instance, binded->instance->type));
    assert(OBJ_IS_FUNCTION(binded->method));
    return OBJ_AS_FUNCTION(binded->method);
}

static inline void call_function(Obj* obj, Value* slots, uint8_t param_count) {
    if (obj->kind == OBJ_NATIVE) {
        call_native(OBJ_AS_NATIVE(obj), param_count);
        return;
    }

    if (qvm.frame_count + 1 >= FRAMES_MAX) {
        runtime_error("Frame overflow");
        return;
    }

    ObjFunction* fn = NULL;
    if (obj->kind == OBJ_BINDED_METHOD) {
        fn = prepare_binded_method(OBJ_AS_BINDED_METHOD(obj), param_count);
        param_count++;
        slots = (qvm.stack_top - param_count - 1);
    } else {
        assert(OBJ_IS_FUNCTION(obj));
        fn = OBJ_AS_FUNCTION(obj);
    }

    qvm.frame_count++;
    qvm.frame = &qvm.frames[qvm.frame_count - 1];
    qvm.frame->func = fn;
    qvm.frame->pc = fn->chunk.code;
    qvm.frame->slots = slots;
}

static inline void call(uint8_t param_count) {
    Value* slots = (qvm.stack_top - param_count - 1);
    Value fn_value = *slots;
    Obj* obj = VALUE_AS_OBJ(fn_value);
    call_function(obj, slots, param_count);
}

static inline void invoke(uint8_t prop_index, uint8_t param_count) {
    Value* slots = (qvm.stack_top - param_count - 1);
    Value instance_value = *slots;

    Obj* instance = VALUE_AS_OBJ(instance_value);
    Value fn_value = object_get_property(instance, prop_index);
    Obj* fn = VALUE_AS_OBJ(fn_value);

    stack_push(instance_value); // Push self
    call_function(fn, slots, ++param_count);
}

void stack_push(Value val) {
    if ((qvm.stack_top - qvm.stack) + 1 >= STACK_MAX) {
        runtime_error("Stack overflow");
        return;
    }
    *(qvm.stack_top++) = val;
}

Value stack_pop() {
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
    ObjString* b = OBJ_AS_STRING(VALUE_AS_OBJ(stack_peek(0)));\
    ObjString* a = OBJ_AS_STRING(VALUE_AS_OBJ(stack_peek(1)));\
    ObjString* concat = concat_string(a, b);\
    Value val = OBJ_VALUE(concat, CREATE_TYPE_STRING());\
    stack_pop();\
    stack_pop();\
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
#define READ_LONG() read_long(&qvm.frame->pc)

#define READ_CONSTANT() qvm.frame->func->chunk.constants.values[READ_BYTE()]
#define READ_STRING() OBJ_AS_STRING(VALUE_AS_OBJ(READ_CONSTANT()))

#define READ_CONSTANT_LONG() qvm.frame->func->chunk.constants.values[READ_LONG()]
#define READ_STRING_LONG() OBJ_AS_STRING(VALUE_AS_OBJ(READ_CONSTANT_LONG()))

#define READ_GLOBAL_CONSTANT() qvm.frames[0].func->chunk.constants.values[READ_BYTE()]
#define READ_GLOBAL_STRING() OBJ_AS_STRING(VALUE_AS_OBJ(READ_GLOBAL_CONSTANT()))

#define READ_GLOBAL_CONSTANT_LONG() qvm.frames[0].func->chunk.constants.values[READ_LONG()]
#define READ_GLOBAL_STRING_LONG() OBJ_AS_STRING(VALUE_AS_OBJ(READ_GLOBAL_CONSTANT_LONG()))

#define GOTO(pos) do { qvm.frame->pc = &qvm.frame->func->chunk.code[pos]; } while(false)

#define ABORT_IF_NIL(val)\
    do {\
        if (VALUE_IS_NIL(val)) {\
            runtime_error("Null pointer object!");\
            return;\
        }\
    } while (false)

static inline Type* read_type() {
    uint8_t index = READ_BYTE();
    Type** types = VECTOR_AS_TYPES(&qvm.frame->func->chunk.types);
    return types[index];
}

static void run(ObjFunction* func) {
#ifdef VM_DEBUG
    printf("--------[ EXECUTION ]--------\n\n");
#endif

    qvm.frame = &qvm.frames[qvm.frame_count - 1];
    qvm.frame->func = func;

    for (;;) {
        if (qvm.had_runtime_error) {
            return;
        }
#ifdef VM_DEBUG
        opcode_print(*qvm.frame->pc);
#endif
        switch (READ_BYTE()) {
        case OP_ADD: {
            Value second = stack_peek(0);
            Value first = stack_peek(1);
            if (TYPE_IS_STRING(first.type) && TYPE_IS_STRING(second.type)) {
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
        case OP_SET_UPVALUE: {
            Value* target = function_get_upvalue(qvm.frame->func, READ_BYTE());
            *target = stack_peek(0);
            break;
        }
        case OP_GET_UPVALUE: {
            Value* readed = function_get_upvalue(qvm.frame->func, READ_BYTE());
            stack_push(*readed);
            break;
        }
        case OP_CALL: {
            uint8_t param_count = READ_BYTE();
            call(param_count);
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
        case OP_BIND_UPVALUE: {
            uint8_t slot = READ_BYTE();
            uint8_t upvalue = READ_BYTE();
            Value* stack_ptr = &qvm.frame->slots[slot];
            Obj* function_obj = VALUE_AS_OBJ(stack_pop());
            ObjFunction* function = OBJ_AS_FUNCTION(function_obj);
            function_open_upvalue(function, upvalue, stack_ptr);
            break;
        }
        case OP_CLOSE: {
            Value val = stack_pop();
            ObjClosed* closed = new_closed(val);
            // TODO Which type should be for a ObjClosed?
            Value obj_closed = OBJ_VALUE(closed, CREATE_TYPE_UNKNOWN());
            stack_push(obj_closed);
            break;
        }
        case OP_BIND_CLOSED: {
            uint8_t upvalue = READ_BYTE();
            Obj* function_obj = VALUE_AS_OBJ(stack_pop());
            ObjFunction* function = OBJ_AS_FUNCTION(function_obj);
            Obj* closed_obj = VALUE_AS_OBJ(stack_peek(0));
            ObjClosed* closed = OBJ_AS_CLOSED(closed_obj);
            function_close_upvalue(function, upvalue, closed);
            break;
        }
        case OP_JUMP: {
            GOTO(READ_LONG());
            break;
        }
        case OP_JUMP_IF_FALSE: {
            Value condition = stack_pop();
            uint8_t dst = READ_LONG();
            if (! VALUE_AS_BOOL(condition)) {
                GOTO(dst);
            }
            break;
        }
        case OP_NEW: {
            Value val = stack_pop();
            ObjClass* klass = OBJ_AS_CLASS(VALUE_AS_OBJ(val));
            ObjInstance* instance = new_instance(klass);
            stack_push(OBJ_VALUE(instance, klass->obj.type)); // This is to assign to the var
            stack_push(OBJ_VALUE(instance, klass->obj.type)); // This is to call init (or to be POPed)
            break;
        }
        case OP_INVOKE: {
            uint8_t prop_index = READ_BYTE();
            uint8_t params = READ_BYTE();
            invoke(prop_index, params);
            break;
        }
        case OP_GET_PROP: {
            Value val = stack_pop();
            ABORT_IF_NIL(val);
            Obj* instance = VALUE_AS_OBJ(val);
            uint8_t pos = READ_BYTE();
            stack_push(object_get_property(instance, pos));
            break;
        }
        case OP_SET_PROP: {
            Value val = stack_pop();
            // This is an expression, so something should be in the stack
            // to be popped later.
            Value obj_val = stack_peek(0);
            ABORT_IF_NIL(obj_val);
            Obj* instance = VALUE_AS_OBJ(obj_val);
            uint8_t pos = READ_BYTE();
            object_set_property(instance, pos, val);
            break;
        }
        case OP_BINDED_METHOD: {
            Value val = stack_peek(0);
            ABORT_IF_NIL(val);
            Obj* instance = VALUE_AS_OBJ(val);
            uint8_t pos = READ_BYTE();
            Value method = object_get_property(instance, pos);
            ObjBindedMethod* binded = new_binded_method(instance, VALUE_AS_OBJ(method));
            stack_pop(); // Now its safe to pop the instance
            stack_push(OBJ_VALUE(binded, binded->obj.type));
            break;
        }
        case OP_ARRAY: {
            Type* inner = read_type();
            ObjArray* arr = new_array(inner);
            stack_push(OBJ_VALUE(arr, arr->obj.type));
            // Just let the array in the top of the stack.
            break;
        }
        case OP_ARRAY_PUSH: {
            Value val = stack_pop();
            Value target = stack_peek(0);
            ObjArray* arr = OBJ_AS_ARRAY(VALUE_AS_OBJ(target));
            valuearray_write(&arr->elements, val);
            break;
        }
        case OP_CAST: {
            Value value = stack_pop();
            Type* cast = read_type();
            stack_push(value_cast(value, cast));
            break;
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
    stack_push(OBJ_VALUE(func, create_type_function()));
    CallFrame* frame = &qvm.frames[qvm.frame_count++];
    frame->func = func;
    frame->pc = func->chunk.code;
    frame->slots = qvm.stack;
    qvm.is_running = true;
    run(func);
}
