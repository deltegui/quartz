#ifndef QUARTZ_VALUES_H
#define QUARTZ_VALUES_H

#include "common.h"

typedef enum {
    VALUE_INTEGER,
    VALUE_FLOAT,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int number_int;
        double number_float;
    } as;
} Value;

#define INTEGER_VALUE(i) ((Value){ VALUE_INTEGER, { .number_int = i } })
#define FLOAT_VALUE(f) ((Value){ VALUE_FLOAT, { .number_float = f } })

#define IS_INTEGER(val) val.type == VALUE_INTEGER
#define IS_FLOAT(val) val.type == VALUE_FLOAT

#define AS_INTEGER(val) val.as.number_int
#define AS_FLOAT(val) val.as.number_float

typedef struct {
    int size;
    int capacity;
    Value* values;
} ValueArray;

void valuearray_init(ValueArray* values);
void valuearray_free(ValueArray* values);
uint8_t valuearray_write(ValueArray* values, Value value);

void valuearray_print(ValueArray* values);

#endif