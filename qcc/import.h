#ifndef QUARTZ_MODULE_H_
#define QUARTZ_MODULE_H_

#include "common.h"
#include "native.h"
#include "token.h"

typedef struct {
    bool is_native;
    bool is_already_loaded;
    union {
        NativeImport native;
        FileImport file;
    };
} Import;

void init_module_system();
void free_module_system();
Import import(const char* path, int length);

#endif
