#ifndef QUARTZ_OBJECT_H
#define QUARTZ_OBJECT_H

#include "lexer.h"

typedef enum {
    STRING_OBJ,
} ObjType;

typedef struct {
    ObjType obj_type;
} Obj;

typedef struct {
    Obj obj;
    int length;
    const char* cstr;
} ObjString;

#define IS_STRING(obj) (obj->obj_type == STRING_OBJ)

#define AS_STRING_OBJ(obj) ((ObjString*) obj)
#define AS_CSTRING(obj) ( ((ObjString*) obj)->cstr )

ObjString* new_string(Token* token);

#endif
