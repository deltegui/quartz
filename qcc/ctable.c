// This is CTable. CTable stands for "Compiler Table". The
// reason why is called "CTable" and not just "Table" is
// that there is another hash table in the compiler (in table.h
// and table.c, used in runtime). Its main feature is that
// it's a generic table.
//
// This hash table is implemented using just Open Addressing
// (it does not use other optimizations). But, since it's
// generic, it has been implemented in a "strange" way.
// First things first, it uses a Vector (vector.h and vector.c)
// to store data by value (so, it does not use void* to
// implement generic code). We don't want to store data outside
// the array. Data is just stored inside the vector, one after
// another.
// Secondly, to search by key, we use a custom array
// which stores keys and the positions of the data inside the
// vector. So, this implementation let us to iterate over the
// values of the hash table easily (just iterate over the vector,
// used heavily in symbol.c) and take advantage of a traditional
// hash table.

#include "ctable.h"
#include <string.h> // for memcmp
#include "object.h" // for hash function

#define LOAD_FACTOR 0.7

#define IS_EMPTY(entry) ((entry)->key.length == 0)
#define SHOULD_GROW(table) (table->size + 1 > table->capacity * LOAD_FACTOR)
#define S_GROW_CAPACITY(cap) (cap < 8 ? 8 : cap * 2)

#define CTABLE_KEY_EQUALS(first, second)\
    (first).hash == (second).hash &&\
    (first).length == (second).length &&\
    memcmp((first).start, (second).start, (second).length) == 0

static void reset_data(CTable* const table);
static void grow_symbol_table(CTable* const table);
static CTableEntry* find(CTable* const table, CTableKey* name);

CTableKey create_ctable_key(const char* start, int length) {
    assert(length != 0);
    assert(start != NULL);
    CTableKey name = (CTableKey){
        .start = start,
        .length = length,
        .hash = hash_string(start, length),
    };
    return name;
}

bool ctable_key_equals(CTableKey* first, CTableKey* second) {
    if (
        first->hash != second->hash ||
        first->length != second->length
    ) {
        return false;
    }
    return memcmp(first->start, second->start, first->length) == 0;
}

static void reset_data(CTable* const table) {
    table->size = 0;
    table->capacity = 0;
    table->mask = 1;
    table->entries = NULL;
}

void init_ctable(CTable* const table, size_t element_size) {
    reset_data(table);
    init_vector(&table->data, element_size);
}

void free_ctable(CTable* const table) {
    if (table->capacity == 0) {
        return;
    }
    free(table->entries);
    free_vector(&table->data);
    reset_data(table);
}

CTableEntry* ctable_find(CTable* const table, CTableKey* key) {
    if (table->capacity == 0) {
        return NULL;
    }
    CTableEntry* entry = find(table, key);
    if (IS_EMPTY(entry)) {
        return NULL;
    }
    return entry;
}

CTableEntry* ctable_next_add_position(CTable* const table, CTableKey key) {
    if (SHOULD_GROW(table)) {
        grow_symbol_table(table);
    }
    assert(table->size + 1 < table->capacity);

    CTableEntry* destination = (CTableEntry*) find(table, &key);
    destination->key = key;
    table->size++;

    return destination;
}

static void grow_symbol_table(CTable* const table) {
    CTableEntry* old_entries = table->entries;
    int old_capacity = table->capacity;

    table->capacity = S_GROW_CAPACITY(old_capacity);
    table->mask = table->capacity - 1;
    table->entries = (CTableEntry*) malloc(sizeof(CTableEntry) * table->capacity);

    for (uint32_t i = 0; i < table->capacity; i++) {
        CTableEntry* entry = (CTableEntry*) &table->entries[i];
        entry->key.length = 0;
    }

    for (int i = 0; i < old_capacity; i++) {
        if (IS_EMPTY(&old_entries[i])) {
            continue;
        }
        CTableEntry* entry = (CTableEntry*) find(table, &old_entries[i].key);
        *entry = old_entries[i];
    }

    // Just free the array if wasn't NULL. Do not free key str.
    // old_entries is NULL if is the first time that is initialized.
    if (old_entries != NULL) {
        free(old_entries);
    }
}

static CTableEntry* find(CTable* const table, CTableKey* key) {
    assert(table != NULL);
    assert(key != NULL);
    assert(key->start != NULL);
    assert(key->length != 0);
    int index = key->hash & table->mask;
    for (;;) {
        CTableEntry* current = &table->entries[index];
        if (IS_EMPTY(current)) {
            return current;
        }
        if (CTABLE_KEY_EQUALS(current->key, *key)) {
            return current;
        }
        index = index + 1 & table->mask;
    }
}

