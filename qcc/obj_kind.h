#ifndef QUARTZ_OBJECT_KIND_H_
#define QUARTZ_OBJECT_KIND_H_

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_BINDED_METHOD,
    OBJ_CLOSED,
    OBJ_NATIVE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_ARRAY,
} ObjKind;

#define CLASS_CONSTRUCTOR_NAME "init"
#define CLASS_CONSTRUCTOR_LENGTH 4
#define CLASS_SELF_NAME "self"
#define CLASS_SELF_LENGTH 4

#endif
