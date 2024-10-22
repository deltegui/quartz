#ifndef QUARTZ_ARRAY_H
#define QUARTZ_ARRAY_H

#include "symbol.h"
#include "stmt.h"

#define ARRAY_CLASS_NAME "Array"
#define ARRAY_CLASS_LENGTH 5

void init_array();
void array_push_props(ValueArray* props);
NativeClassStmt array_register(ScopedSymbolTable* const table);
void mark_array();

#endif
