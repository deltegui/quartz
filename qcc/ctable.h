#ifndef QUARTZ_CTABLE_H_
#define QUARTZ_CTABLE_H_

#include "common.h"
#include "vector.h"

typedef struct {
    const char* str;
    int length;
    uint32_t hash;
} CTableKey;

CTableKey create_ctable_key(const char* str, int length);

typedef struct {
    Vector keys; // Vector<CTableKey>
    Vector entries; // Vector <void*>
    size_t element_size;
    int size;
    int capacity;
    int mask;
} CTable;

void init_ctable(CTable* const table, size_t element_size);
void free_ctable(CTable* const table);
void* ctable_find(CTable* const table, CTableKey* key);
void ctable_set(CTable* const table, CTableKey key, void* value);

#endif