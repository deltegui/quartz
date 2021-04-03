#include "values.h"
#include "object.h" // used for print_object
// ValueArray is a runtime data structure, so its memory
// must be managed by vm_memory.h
#include "vm_memory.h"

void value_print(Value val) {
    switch (val.kind) {
    case VALUE_NUMBER: printf("%f", AS_NUMBER(val)); break;
    case VALUE_BOOL: printf("%s", AS_BOOL(val) ? "true" : "false"); break;
    case VALUE_NIL: printf("nil"); break;
    case VALUE_OBJ: print_object(AS_OBJ(val)); break;
    }
}

Value value_default(Type type) {
    assert(type != UNKNOWN_TYPE);
    switch (type) {
    case NUMBER_TYPE: return NUMBER_VALUE(0);
    case BOOL_TYPE: return BOOL_VALUE(false);
    case STRING_TYPE: return OBJ_VALUE(copy_string("", 0));
    case NIL_TYPE:
    default:
        return NIL_VALUE();
    }
}

bool value_equals(Value first, Value second) {
    switch (first.kind) {
    case VALUE_NUMBER: {
        if (!IS_NUMBER(second)) {
            return false;
        }
        return AS_NUMBER(first) == AS_NUMBER(second);
    }
    case VALUE_BOOL: {
        if (!IS_BOOL(second)) {
            return false;
        }
        return AS_BOOL(first) == AS_BOOL(second);
    }
    case VALUE_NIL:
        return IS_NIL(second);
    case VALUE_OBJ: {
        if (!IS_OBJ(second)) {
            return false;
        }
        return AS_OBJ(first) == AS_OBJ(second);
    }
    }
}

void init_valuearray(ValueArray* arr) {
    arr->size = 0;
    arr->capacity = 0;
    arr->values = NULL;
}

void free_valuearray(ValueArray* arr) {
    if (arr->values == NULL) {
        return;
    }
    FREE(Value, arr->values);
    arr->size = 0;
    arr->capacity = 0;
}

int valuearray_write(ValueArray* arr, Value value) {
    if (arr->capacity <= arr->size + 1) {
        size_t old = arr->capacity;
        arr->capacity = GROW_CAPACITY(arr->capacity);
        arr->values = GROW_ARRAY(
            Value,
            arr->values,
            old,
            arr->capacity);
    }
    arr->values[arr->size] = value;
    return arr->size++;
}
