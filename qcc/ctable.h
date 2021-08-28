#ifndef QUARTZ_CTABLE_H_
#define QUARTZ_CTABLE_H_

typedef struct {
    const char* str;
    int length;
    uint32_t hash;
} CTableKey;

CTableKey create_ctable_key(const char* str, int length);

typedef struct {
    CTableKey key;
    void[] value;
} CTableEntry;

typedef struct {
    CTableEntry* entries;
    size_t element_size;
    int size;
    int capacity;
    int mask;
} CTable;

void init_ctable(CTable* const table, size_t element_size);
void free_ctable(CTable* const table);
CTableEntry* ctable_find(CTable* const table, CTableKey* key);
void ctable_set(CTable* const table, CTableKey* key, void* value);

#endif