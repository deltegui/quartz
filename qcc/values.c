#include "values.h"
// ValueArray is a runtime data structure, so its memory
// must be managed by memory.h
#include "memory.h"

void valuearray_init(ValueArray* arr) {
    arr->size = 0;
    arr->capacity = 0;
    arr->values = NULL;
}

void valuearray_free(ValueArray* arr) {
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

void valuearray_print(ValueArray* values) {
    printf("--------[ VALUE ARRAY ]--------\n\n");
    for (int i = 0; i < values->size; i++) {
        printf("| %d\t\t", i);
    }
    printf("\n");
    printf("|---------------|---------------|--------------\n");
    for (int i = 0; i < values->size; i++) {
        Value val = values->values[i];
        switch (val.type) {
        case VALUE_INTEGER: printf("| %d\t", AS_INTEGER(val)); break;
        case VALUE_FLOAT: printf("| %f\t", AS_FLOAT(val)); break;
        }
    }
    printf("\n\n");
}
