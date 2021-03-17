#include "table.h"
#include <string.h>
#include "vm_memory.h"

#define LOAD_FACTOR 0.7

#define TABLE_SHOULD_GROW(table) (table->size + 1 > table->capacity * LOAD_FACTOR)
#define IS_ENTRY_EMPTY(table, index) (table->entries[index].key == NULL)
#define SHOULD_INTERCHANGE_ENTRY(table, index, dist) (table->entries[index].distance < dist)

void init_table(Table* table) {
    table->entries = NULL;
    table->size = 0;
    table->capacity = 0;
    table->max_distance = 0;
}

void free_table(Table* table) {
    if (table->entries == NULL) {
        return;
    }
    FREE_ARRAY(Entry, table->entries, table->size);
    init_table(table);
}

static void table_insert(Table* table, ObjString* key, Value value) {
    uint32_t index = key->hash & (table->capacity - 1);
    Entry current_entry = (Entry){
        .key = key,
        .value = value,
        .distance = 0,
    };
    int current_index = index;
    for (;;) {
        if (IS_ENTRY_EMPTY(table, current_index)) {
            table->entries[current_index] = current_entry;
            table->size++;
            return;
        }
        if (SHOULD_INTERCHANGE_ENTRY(table, current_index, current_entry.distance)) {
            Entry old_entry = table->entries[current_index];
            table->entries[current_index] = current_entry;
            current_entry = old_entry;
        }
        current_entry.distance++;
        if (current_entry.distance > table->max_distance) {
            table->max_distance = current_entry.distance;
        }
        current_index = (current_index + 1) & (table->capacity - 1);
        // It's garateed that there is at least one empty space. So, returning to 
        // the start should never happen.
        assert(current_index != index);
    }
}

void table_set(Table* table, ObjString* key, Value value) {
    if (TABLE_SHOULD_GROW(table)) {
        int old_capacity = table->capacity;
        table->capacity = GROW_CAPACITY(table->capacity);
        table->entries = GROW_ARRAY(Entry, table->entries, old_capacity, table->capacity);
    }
    table_insert(table, key, value);
}

Value table_find(Table* table, ObjString* key) {
    uint32_t index = key->hash & (table->capacity - 1);
    int distance = 0;
    while (distance <= table->max_distance) {
        if (IS_ENTRY_EMPTY(table, index)) {
            break;
        }
        // @warning this line supposes that all strings in the language
        // are interned.
        if (key == table->entries[index].key) {
            return table->entries[index].value;
        }
        index = (index + 1) & (table->capacity - 1);
        distance++;
    }
    return NIL_VALUE();
}

ObjString* table_find_string(Table* table, const char* chars, int length, uint32_t hash) {
    uint32_t index = hash & (table->capacity - 1);
    int distance = 0;
    while (distance <= table->max_distance) {
        if (IS_ENTRY_EMPTY(table, index)) {
            break;
        }
        ObjString* current = table->entries[index].key;
        if (length == current->length && hash == current->hash) {
            if (memcmp(current->cstr, chars, length) == 0) {
                return current;
            }
        }
        index = (index + 1) & (table->capacity - 1);
        distance++;
    }
    return NULL;
}
