#ifndef QUARTZ_MODULE_H_
#define QUARTZ_MODULE_H_

#include "common.h"
#include "native.h"

typedef struct {
    const char* path;
    int path_length;
    const char* source;
    bool is_already_loaded;
} FileImport;

typedef struct {
    bool is_native;
    union {
        NativeImport native;
        FileImport file;
    };
} Import;

void init_module_system();
void free_module_system();
Import import(const char* path, int length);

#endif
