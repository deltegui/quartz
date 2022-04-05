#ifndef QUARTZ_STRING_H
#define QUARTZ_STRING_H

#include "symbol.h"
#include "stmt.h"

#define STRING_CLASS_NAME "String"
#define STRING_CLASS_LENGTH 6

void init_string();
void string_push_props(ValueArray* props);
NativeClassStmt string_register(ScopedSymbolTable* const table);
void mark_string();

#endif
