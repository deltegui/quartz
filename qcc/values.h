#ifndef QUARTZ_VALUES_H
#define QUARTZ_VALUES_H

#include "common.h"

typedef enum {
    VALUE_INTEGER,
    VALUE_FLOAT,
    VALUE_BOOL,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int number_int;
        double number_float;
        bool boolean;
    } as;
} Value;

#define INTEGER_VALUE(i) ((Value){ VALUE_INTEGER, { .number_int = i } })
#define FLOAT_VALUE(f) ((Value){ VALUE_FLOAT, { .number_float = f } })
#define BOOL_VALUE(b) ((Value){ VALUE_BOOL, { .boolean = b } })

#define IS_INTEGER(val) val.type == VALUE_INTEGER
#define IS_FLOAT(val) val.type == VALUE_FLOAT
#define IS_BOOL(val) val.type == VALUE_BOOL

#define AS_INTEGER(val) val.as.number_int
#define AS_FLOAT(val) val.as.number_float
#define AS_BOOL(val) val.as.boolean

typedef struct {
    int size;
    int capacity;
    Value* values;
} ValueArray;

void init_valuearray(ValueArray* values);
void free_valuearray(ValueArray* values);
uint8_t valuearray_write(ValueArray* values, Value value);

#endif