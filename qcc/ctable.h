#ifndef QUARTZ_CTABLE_H_
#define QUARTZ_CTABLE_H_

#include "common.h"
#include "vector.h"

typedef struct {
    const char* start;
    int length;
    uint32_t hash;
} CTableKey;

CTableKey create_ctable_key(const char* start, int length);
bool ctable_key_equals(CTableKey* first, CTableKey* second);

typedef struct {
    CTableKey key;
    int vector_pos;
} CTableEntry;

typedef struct {
    Vector data; // Vector <void*>

    CTableEntry* entries;
    uint32_t size;
    uint32_t capacity;
    int mask;
} CTable;

void init_ctable(CTable* const table, size_t element_size);
void free_ctable(CTable* const table);
CTableEntry* ctable_find(CTable* const table, CTableKey* key);
CTableEntry* ctable_next_add_position(CTable* const table, CTableKey key);

#define CTABLE_FOREACH(table, type, ...) do {\
    type* elements = (type*) (table)->data.elements;\
    for (uint32_t i = 0; i < (table)->data.size; i++) {\
        __VA_ARGS__\
    }\
} while(false)

#define CTABLE_SET(table, key, value, type) do {\
    CTableEntry* destination = ctable_next_add_position((table), key);\
    uint32_t vector_pos = vector_next_add_position(&(table)->data);\
    type* elements = (type*) (table)->data.elements;\
    elements[vector_pos] = value;\
    destination->vector_pos = vector_pos;\
} while(false)

#define CTABLE_ENTRY_RESOLVE_AS(table, entry, dst, type) do {\
    type* elements = (type*) (table)->data.elements;\
    *(dst) = elements[entry->vector_pos];\
} while(false)

#define CTABLE_SIZE(table) ((table).data.size)
#define CTABLE_AS(table, type) VECTOR_AS(&(table).data, type)

#endif

