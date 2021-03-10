#include "values.h"
// ValueArray is a runtime data structure, so its memory
// must be managed by vm_memory.h
#include "vm_memory.h"

void value_print(Value val) {
    switch (val.type) {
    case VALUE_NUMBER: printf("%f", AS_NUMBER(val)); break;
    case VALUE_BOOL: printf("%s", AS_BOOL(val) ? "true" : "false"); break;
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

uint8_t valuearray_write(ValueArray* arr, Value value) {
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
