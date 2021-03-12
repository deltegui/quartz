#ifndef QUARTZ_TABLE_H
#define QUARTZ_TABLE_H

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    Entry* entires;
    int size;
    int capacity;
} Table;

#endif