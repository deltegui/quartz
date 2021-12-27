#include "values.h"
#include <string.h>
#include "object.h" // used for print_object and mark_object
// ValueArray is a runtime data structure, so its memory
// must be managed by vm_memory.h
#include "vm_memory.h"

void value_print(Value val) {
    switch (val.kind) {
    case VALUE_NUMBER: printf("%g", VALUE_AS_NUMBER(val)); break;
    case VALUE_BOOL: printf("%s", VALUE_AS_BOOL(val) ? "true" : "false"); break;
    case VALUE_NIL: printf("nil"); break;
    case VALUE_OBJ: print_object(VALUE_AS_OBJ(val)); break;
    }
}

Value value_default(Type* type) {
    assert(! TYPE_IS_UNKNOWN(type));
    switch (type->kind) {
    case TYPE_NUMBER: return NUMBER_VALUE(0);
    case TYPE_BOOL: return BOOL_VALUE(false);
    case TYPE_STRING: {
        Value str = OBJ_VALUE(copy_string("", 0), CREATE_TYPE_STRING());
        return str;
    }
    case TYPE_NIL:
    default:
        return NIL_VALUE();
    }
}

void mark_value(Value value) {
    if (VALUE_IS_OBJ(value)) {
        mark_object(VALUE_AS_OBJ(value));
    }
}

bool value_equals(Value first, Value second) {
    switch (first.kind) {
    case VALUE_NUMBER: {
        if (!VALUE_IS_NUMBER(second)) {
            return false;
        }
        return VALUE_AS_NUMBER(first) == VALUE_AS_NUMBER(second);
    }
    case VALUE_BOOL: {
        if (!VALUE_IS_BOOL(second)) {
            return false;
        }
        return VALUE_AS_BOOL(first) == VALUE_AS_BOOL(second);
    }
    case VALUE_NIL:
        return VALUE_IS_NIL(second);
    case VALUE_OBJ: {
        if (!VALUE_IS_OBJ(second)) {
            return false;
        }
        return VALUE_AS_OBJ(first) == VALUE_AS_OBJ(second);
    }
    }
    assert(false); // We should not reach this line
}

void init_valuearray(ValueArray* const arr) {
    arr->size = 0;
    arr->capacity = 0;
    arr->values = NULL;
}

void free_valuearray(ValueArray* const arr) {
    if (arr->values == NULL) {
        return;
    }
    FREE(Value, arr->values);
    arr->size = 0;
    arr->capacity = 0;
}

int valuearray_write(ValueArray* const arr, Value value) {
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

void mark_valuearray(ValueArray* const array) {
    for (int i = 0; i < array->size; i++) {
        mark_value(array->values[i]);
    }
}

void valuearray_deep_copy(ValueArray* const origin, ValueArray* destiny) {
    destiny->capacity = origin->capacity;
    destiny->values = GROW_ARRAY(
        Value,
        destiny->values,
        0,
        destiny->capacity);
    memcpy(destiny->values, origin->values, sizeof(Value) * origin->capacity);
    destiny->size = origin->size;
}
