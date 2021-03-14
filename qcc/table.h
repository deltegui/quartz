#ifndef QUARTZ_TABLE_H
#define QUARTZ_TABLE_H

typedef struct {
    ObjString* key;
    Value value;
    int probe_sequence_length;
} Entry;

typedef struct {
    Entry* entires;
    int size;
    int capacity;
} Table;

#endif