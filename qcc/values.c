#include "values.h"
#include "object.h" // @todo used for value_free.
// ValueArray is a runtime data structure, so its memory
// must be managed by vm_memory.h
#include "vm_memory.h"

void value_print(Value val) {
    switch (val.type) {
    case VALUE_NUMBER: printf("%f", AS_NUMBER(val)); break;
    case VALUE_BOOL: printf("%s", AS_BOOL(val) ? "true" : "false"); break;
    case VALUE_NIL: printf("nil"); break;
    case VALUE_OBJ: printf("Object"); break; // @todo this should be changed
    }
}

void init_valuearray(ValueArray* arr) {
    arr->size = 0;
    arr->capacity = 0;
    arr->values = NULL;
}

static void value_free(Value value) {
    if (value.type == VALUE_OBJ) {
        ObjString* str = AS_STRING_OBJ(AS_OBJ(value));
        FREE(char, (void*)str->cstr);
        FREE(ObjString, str);
    }
}

void free_valuearray(ValueArray* arr) {
    if (arr->values == NULL) {
        return;
    }
    for (int i = 0; i < arr->size; i++) {
        value_free(arr->values[i]);
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
