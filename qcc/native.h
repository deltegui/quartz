#ifndef QUARTZ_NATIVE_H_
#define QUARTZ_NATIVE_H_

#include "values.h"
#include "type.h"

typedef Value (*native_fn_t) (int argc, Value* argv);

typedef struct {
    const char* name;
    int length;
    native_fn_t function;
    Type* type;
} NativeFunction;

typedef struct {
    const char* name;
    int length;
    NativeFunction* functions;
    int functions_length;
} NativeImport;

#endif