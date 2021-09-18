#ifndef QUARTZ_OBJECT_H_
#define QUARTZ_OBJECT_H_

#include "lexer.h"
#include "chunk.h"
#include "obj_kind.h"
#include "type.h"

typedef struct s_obj {
    ObjKind kind;
    Type* type;
    bool is_marked;
    struct s_obj* next;
} Obj;

typedef struct s_obj_string {
    Obj obj;
    uint32_t hash;
    int length;
    char chars[];
} ObjString;

typedef struct {
    Obj obj;
    Value value;
} ObjClosed;

typedef struct {
    bool is_closed;
    union {
        Value* open;
        ObjClosed* closed;
    };
} Upvalue;

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
    int upvalue_count;
    Upvalue upvalues[];
} ObjFunction;

void print_object(Obj* obj);
// TODO rename this shit
bool is_obj_kind(Obj* obj, ObjKind kind);
void mark_object(Obj* obj);

#define OBJ_IS_STRING(obj) (is_obj_kind(obj, OBJ_STRING))
#define OBJ_AS_STRING(obj) ((ObjString*) obj)
#define OBJ_AS_CSTRING(obj) ( ((ObjString*) obj)->chars )

ObjString* copy_string(const char* str, int length);
uint32_t hash_string(const char* chars, int length);
ObjString* concat_string(ObjString* first, ObjString* second);

#define OBJ_IS_FUNCTION(obj) (is_obj_kind(obj, OBJ_FUNCTION))
#define OBJ_AS_FUNCTION(obj) ((ObjFunction*) obj)

ObjFunction* new_function(const char* name, int length, int upvalues, Type* type);
void function_close_upvalue(ObjFunction* const function, int upvalue, ObjClosed* closed);
void function_open_upvalue(ObjFunction* const function, int upvalue, Value* value);
Value* function_get_upvalue(ObjFunction* const function, int slot);

#define OBJ_IS_CLOSED(obj) (is_obj_kind(obj, OBJ_CLOSED))
#define OBJ_AS_CLOSED(obj) ((ObjClosed*) obj)

ObjClosed* new_closed(Value value);

#endif
