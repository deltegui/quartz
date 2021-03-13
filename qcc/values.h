#ifndef QUARTZ_VALUES_H
#define QUARTZ_VALUES_H

#include "common.h"
#include "object.h"

typedef enum {
    VALUE_NUMBER,
    VALUE_BOOL,
    VALUE_NIL,
    VALUE_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        double number;
        bool boolean;
        Obj* object;
    } as;
} Value;

void value_print(Value val);
bool value_equals(Value first, Value second);

#define NUMBER_VALUE(i) ((Value){ VALUE_NUMBER, { .number = i } })
#define BOOL_VALUE(b) ((Value){ VALUE_BOOL, { .boolean = b } })
#define NIL_VALUE(val) ((Value){ VALUE_NIL, { .object = NULL } })
#define OBJ_VALUE(obj) ((Value){ VALUE_OBJ, { .object = (Obj*) obj } })

#define IS_NUMBER(val) (val.type == VALUE_NUMBER)
#define IS_BOOL(val) (val.type == VALUE_BOOL)
#define IS_NIL(val) (val.type == VALUE_NIL)
#define IS_OBJ(val) (val.type == VALUE_OBJ)

#define AS_NUMBER(val) val.as.number
#define AS_BOOL(val) val.as.boolean
#define AS_OBJ(val) ((Obj*) val.as.object)

typedef struct {
    int size;
    int capacity;
    Value* values;
} ValueArray;

void init_valuearray(ValueArray* values);
void free_valuearray(ValueArray* values);
uint8_t valuearray_write(ValueArray* values, Value value);

#endif