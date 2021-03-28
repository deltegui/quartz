#ifndef QUARTZ_MEMORY_H
#define QUARTZ_MEMORY_H

#include "common.h"

void* qvm_realloc(void* ptr, size_t old_size, size_t size);
void free_objects();

#define ALLOC(type, count) (type*) qvm_realloc(NULL, 0, sizeof(type) * count)

#define GROW_CAPACITY(capacity) ((capacity < 8) ? 8 : (capacity) * 2)

#define FREE(type, pointer)\
    qvm_realloc(pointer, sizeof(type), 0)

#define GROW_ARRAY(type, ptr, old_count, count)\
    (type*) qvm_realloc(ptr, sizeof(type) * old_count, sizeof(type) * count)

#define FREE_ARRAY(type, ptr, count)\
    (type*) qvm_realloc(ptr, sizeof(type) * count, 0);

#endif