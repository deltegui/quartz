#ifndef QUARTZ_OBJECT_H
#define QUARTZ_OBJECT_H

#include "lexer.h"
#include "chunk.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjKind;

typedef struct s_obj {
    ObjKind kind;
    struct s_obj* next;
} Obj;

// Here we use flexible array member. This lets you to
// avoid double indirection using pointers, which let
// us have better performance.
typedef struct s_obj_string {
    Obj obj;
    uint32_t hash;
    int length;
    char chars[];
} ObjString;

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

void print_object(Obj* obj);
bool is_obj_kind(Obj* obj, ObjKind kind);

#define OBJ_IS_STRING(obj) (is_obj_kind(obj, OBJ_STRING))
#define OBJ_AS_STRING(obj) ((ObjString*) obj)
#define OBJ_AS_CSTRING(obj) ( ((ObjString*) obj)->chars )

ObjString* copy_string(const char* str, int length);
uint32_t hash_string(const char* chars, int length);
ObjString* concat_string(ObjString* first, ObjString* second);

#define OBJ_IS_FUNCTION(obj) (is_obj_kind(obj, OBJ_FUNCTION))
#define OBJ_AS_FUNCTION(obj) ((ObjFunction*) obj)

ObjFunction* new_function(const char* name, int length);

#endif
