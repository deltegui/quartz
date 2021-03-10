#ifndef QUARTZ_VALUES_H
#define QUARTZ_VALUES_H

#include "common.h"

typedef enum {
    VALUE_NUMBER,
    VALUE_BOOL,
} ValueType;

typedef struct {
    ValueType type;
    union {
        double number;
        bool boolean;
    } as;
} Value;

void value_print(Value val);

#define NUMBER_VALUE(i) ((Value){ VALUE_NUMBER, { .number = i } })
#define BOOL_VALUE(b) ((Value){ VALUE_BOOL, { .boolean = b } })

#define IS_NUMBER(val) val.type == VALUE_NUMBER
#define IS_BOOL(val) val.type == VALUE_BOOL

#define AS_NUMBER(val) val.as.number
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