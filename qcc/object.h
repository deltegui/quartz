#ifndef QUARTZ_OBJECT_H
#define QUARTZ_OBJECT_H

#include "lexer.h"

typedef enum {
    STRING_OBJ,
} ObjType;

typedef struct Obj {
    ObjType obj_type;
    struct Obj* next;
} Obj;

// Here we use flexible array member. This lets you to
// avoid double indirection using pointers, which let
// us have better performance.
typedef struct {
    Obj obj;
    int length;
    char cstr[];
} ObjString;

#define IS_STRING(obj) (obj->obj_type == STRING_OBJ)

#define AS_STRING_OBJ(obj) ((ObjString*) obj)
#define AS_CSTRING(obj) ( ((ObjString*) obj)->cstr )

void print_object(Obj* obj);
ObjString* copy_string(const char* str, int length);
ObjString* concat_string(ObjString* first, ObjString* second);

#endif
