#ifndef QUARTZ_TABLE_H_
#define QUARTZ_TABLE_H_

#include "values.h"
#include "object.h"

typedef struct {
    ObjString* key;
    Value value;
    int distance;
} Entry;

typedef struct {
    Entry* entries;
    int size;
    int capacity;
    int max_distance;
} Table;

#define IS_ENTRY_EMPTY(table, index) (table->entries[index].key == NULL && table->entries[index].distance != -1)

void init_table(Table* table);
void free_table(Table* table);
void table_set(Table* table, ObjString* key, Value value);
Value table_find(Table* table, ObjString* key);
bool table_delete(Table* table, ObjString* key);
ObjString* table_find_string(Table* table, const char* chars, int length, uint32_t hash);

#endif
