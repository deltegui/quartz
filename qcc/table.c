#include "table.h"
#include <string.h>
#include "vm_memory.h"

#define LOAD_FACTOR 0.75

#define TABLE_SHOULD_GROW(table) (table->size + 1 > table->capacity * LOAD_FACTOR)
#define SHOULD_INTERCHANGE_ENTRY(table, index, dist) (table->entries[index].distance < dist)
#define KEY_EQUALS(table, index, other_key) (table->entries[index].key == other_key)
#define IS_TOMBSTONE(table, index) (table->entries[index].distance == -1)

static void insert(Table* table, ObjString* key, Value value);
static void adjust_capacity(Table* table, int capacity);
static Entry* find_entry(Table* table, ObjString* key);

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

static void insert(Table* table, ObjString* key, Value value) {
    uint32_t index = key->hash & (table->capacity - 1);
    Entry entry_insert = (Entry){
        .key = key,
        .value = value,
        .distance = 0,
    };
    uint32_t current_index = index;
    for (;;) {
        if (IS_ENTRY_EMPTY(table, current_index) || IS_TOMBSTONE(table, current_index)) {
            table->entries[current_index] = entry_insert;
            table->size++;
            return;
        }
        if (KEY_EQUALS(table, current_index, entry_insert.key)) {
            table->entries[current_index] = entry_insert;
            return;
        }
        if (SHOULD_INTERCHANGE_ENTRY(table, current_index, entry_insert.distance)) {
            Entry old_entry = table->entries[current_index];
            table->entries[current_index] = entry_insert;
            entry_insert = old_entry;
        }
        entry_insert.distance++;
        if (entry_insert.distance > table->max_distance) {
            table->max_distance = entry_insert.distance;
        }
        current_index = (current_index + 1) & (table->capacity - 1);
        // It's garanteed that there is at least one empty space. So, returning to
        // the start should never happen.
        assert(current_index != index);
    }
}

static void adjust_capacity(Table* table, int capacity) {
    Entry* old_entries = table->entries;
    int old_capacity = table->capacity;

    table->entries = ALLOC(Entry, capacity);
    table->capacity = capacity;
    table->size = 0;
    table->max_distance = 0;

    for (int i = 0; i < table->capacity; i++) {
        table->entries[i].key = NULL;
        table->entries[i].value = NIL_VALUE();
        table->entries[i].distance = 0;
    }

    for (int i = 0; i < old_capacity; i++) {
        if (old_entries[i].key != NULL) {
            insert(table, old_entries[i].key, old_entries[i].value);
        }
    }

    FREE_ARRAY(Entry, old_entries, old_capacity);
}

void table_set(Table* table, ObjString* key, Value value) {
    if (TABLE_SHOULD_GROW(table)) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }
    insert(table, key, value);
}

static Entry* find_entry(Table* table, ObjString* key) {
    if (table->size == 0) {
        return NULL;
    }
    uint32_t index = key->hash & (table->capacity - 1);
    int distance = 0;
    while (distance <= table->max_distance) {
        if (IS_ENTRY_EMPTY(table, index)) {
            break;
        }
        if ((! IS_TOMBSTONE(table, index)) && KEY_EQUALS(table, index, key)) {
            return &table->entries[index];
        }
        index = (index + 1) & (table->capacity - 1);
        distance++;
    }
    return NULL;
}

Value table_find(Table* table, ObjString* key) {
    Entry* entry = find_entry(table, key);
    if (entry == NULL) {
        return NIL_VALUE();
    }
    return entry->value;
}

bool table_delete(Table* table, ObjString* key) {
    Entry* entry = find_entry(table, key);
    if (entry == NULL) {
        return false;
    }
    entry->key = NULL;
    entry->value = NIL_VALUE();
    entry->distance = -1;
    table->size--;
    return true;
}

ObjString* table_find_string(Table* table, const char* chars, int length, uint32_t hash) {
    if (table->size == 0) {
        return NULL;
    }
    uint32_t index = hash & (table->capacity - 1);
    int distance = 0;
    while (distance <= table->max_distance) {
        if (IS_ENTRY_EMPTY(table, index)) {
            break;
        }
        if (! IS_TOMBSTONE(table, index)) {
            ObjString* current = table->entries[index].key;
            if (
                length == current->length &&
                hash == current->hash &&
                memcmp(current->chars, chars, length) == 0
            ) {
                return current;
            }
        }
        index = (index + 1) & (table->capacity - 1);
        distance++;
    }
    return NULL;
}
