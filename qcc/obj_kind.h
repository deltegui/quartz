#ifndef QUARTZ_OBJECT_KIND_H_
#define QUARTZ_OBJECT_KIND_H_

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSED,
    OBJ_NATIVE,
    OBJ_CLASS,
    OBJ_INSTANCE,
} ObjKind;

#endif
