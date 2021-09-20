#ifndef QUARTZ_VALUES_H_
#define QUARTZ_VALUES_H_

#include "common.h"
#include "type.h"

typedef struct s_obj Obj;
typedef struct s_obj_string ObjString;

typedef enum {
    VALUE_NUMBER,
    VALUE_BOOL,
    VALUE_NIL,
    VALUE_OBJ,
} ValueKind;

typedef struct {
    Type* type;
    ValueKind kind;
    union {
        double number;
        bool boolean;
        Obj* object;
    } as;
} Value;

void value_print(Value val);
bool value_equals(Value first, Value second);
Value value_default(Type* type);
void mark_value(Value value);

#define NUMBER_VALUE(i) ((Value){ CREATE_TYPE_NUMBER(), VALUE_NUMBER, { .number = i } })
#define BOOL_VALUE(b) ((Value){ CREATE_TYPE_BOOL(), VALUE_BOOL, { .boolean = b } })
#define NIL_VALUE() ((Value){ CREATE_TYPE_NIL(), VALUE_NIL, { .object = NULL } })
#define OBJ_VALUE(ob, obj_type) ((Value){ obj_type, VALUE_OBJ, { .object = (Obj*) ob } })

#define VALUE_IS_NUMBER(val) (val.kind == VALUE_NUMBER)
#define VALUE_IS_BOOL(val) (val.kind == VALUE_BOOL)
#define VALUE_IS_NIL(val) (val.kind == VALUE_NIL)
#define VALUE_IS_OBJ(val) (val.kind == VALUE_OBJ)

#define VALUE_AS_NUMBER(val) val.as.number
#define VALUE_AS_BOOL(val) val.as.boolean
#define VALUE_AS_OBJ(val) ((Obj*) val.as.object)

typedef struct {
    int size;
    int capacity;
    Value* values;
} ValueArray;

void init_valuearray(ValueArray* const values);
void free_valuearray(ValueArray* const values);
int valuearray_write(ValueArray* const values, Value value);
void mark_valuearray(ValueArray* const array);

#endif
