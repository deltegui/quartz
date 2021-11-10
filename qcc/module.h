#ifndef QUARTZ_MODULE_H_
#define QUARTZ_MODULE_H_

#include "common.h"

typedef struct {
    const char* path;
    int path_length;
    const char* source;
    bool is_already_loaded;
} Module;

void init_module_system();
void free_module_system();
Module module_read(const char* path, int length);

#endif