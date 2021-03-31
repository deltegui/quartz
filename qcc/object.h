#ifndef QUARTZ_OBJECT_H
#define QUARTZ_OBJECT_H

#include "lexer.h"

typedef enum {
    STRING_OBJ,
} ObjKind;

typedef struct Obj {
    ObjKind kind;
    struct Obj* next;
} Obj;

// Here we use flexible array member. This lets you to
// avoid double indirection using pointers, which let
// us have better performance.
typedef struct {
    Obj obj;
    uint32_t hash;
    int length;
    char cstr[]; // @todo would be better to call this just 'chars'?
} ObjString;

#define IS_STRING(obj) (obj->kind == STRING_OBJ)

#define AS_STRING_OBJ(obj) ((ObjString*) obj)
#define AS_CSTRING(obj) ( ((ObjString*) obj)->cstr )

void print_object(Obj* obj);
ObjString* copy_string(const char* str, int length);
ObjString* concat_string(ObjString* first, ObjString* second);

#endif
