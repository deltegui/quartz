#ifndef QUARTZ_TYPECHECKER_H_
#define QUARTZ_TYPECHECKER_H_

#include "stmt.h"
#include "symbol.h"

bool typecheck(const char* source, Stmt* ast, ScopedSymbolTable* symbols);

#endif
